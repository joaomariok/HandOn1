/**
 * @file sample_codec.cpp
 * @brief Implementation of PCM<->float sample decoding/encoding.
 *
 * Deliberately the simplest correct approach: no dithering, no noise shaping.
 * Not production audio DSP.
 */

#include "sample_codec.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

#include "wav_types.h"

namespace {

/**
 * @brief Decodes an 8-bit unsigned PCM sample to a normalized float.
 */
float decodeUnsigned8(uint8_t byte) {
    return (static_cast<float>(byte) - 128.0f) / 128.0f;
}

/**
 * @brief Encodes a normalized float to an 8-bit unsigned PCM sample, clamped to [0, 255].
 */
uint8_t encodeUnsigned8(float sample) {
    float clamped = std::clamp(sample, -1.0f, 1.0f);
    int quantized = static_cast<int>(std::lround(clamped * 127.0f)) + 128;
    return static_cast<uint8_t>(std::clamp(quantized, 0, 255));
}

/**
 * @brief Decodes a little-endian signed integer PCM sample of numBytes width to a
 *        normalized float. Sign-extends widths below 32 bits before scaling.
 */
float decodeSignedInt(const uint8_t* bytes, int numBytes) {
    uint32_t raw = 0;
    for (int i = 0; i < numBytes; ++i) {
        raw |= static_cast<uint32_t>(bytes[i]) << (8 * i);
    }

    int32_t value;
    if (numBytes >= 4) {
        value = static_cast<int32_t>(raw);
    } else {
        uint32_t signBit = 1u << (numBytes * 8 - 1);
        uint32_t signExtendMask = ~0u << (numBytes * 8);
        if (raw & signBit) {
            raw |= signExtendMask;
        }
        value = static_cast<int32_t>(raw);
    }

    double scale = static_cast<double>(1u << (numBytes * 8 - 1)); // 2^(bits-1)
    return static_cast<float>(static_cast<double>(value) / scale);
}

/**
 * @brief Encodes a normalized float to a little-endian signed integer PCM sample of
 *        numBytes width, clamped to the representable range.
 */
void encodeSignedInt(float sample, int numBytes, uint8_t* out) {
    double clamped = std::clamp(static_cast<double>(sample), -1.0, 1.0);
    double scale = static_cast<double>((1u << (numBytes * 8 - 1)) - 1); // 2^(bits-1)-1

    long long minVal = -(1LL << (numBytes * 8 - 1));
    long long maxVal = (1LL << (numBytes * 8 - 1)) - 1;
    long long quantized = static_cast<long long>(std::lround(clamped * scale));
    quantized = std::clamp(quantized, minVal, maxVal);

    uint32_t raw = static_cast<uint32_t>(quantized);
    for (int i = 0; i < numBytes; ++i) {
        out[i] = static_cast<uint8_t>((raw >> (8 * i)) & 0xFFu);
    }
}

/// PCM format tag: uncompressed integer samples.
constexpr uint16_t AUDIO_FORMAT_PCM = 1;
/// PCM format tag: IEEE float samples.
constexpr uint16_t AUDIO_FORMAT_IEEE_FLOAT = 3;

} // namespace

/** @copydoc decodeSamples */
std::vector<std::vector<float>> decodeSamples(const std::vector<uint8_t>& pcmBytes,
                                               uint16_t audioFormat,
                                               uint16_t bitsPerSample,
                                               uint16_t numChannels) {
    bool isFloat = false;
    if (audioFormat == AUDIO_FORMAT_PCM) {
        if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) {
            throw std::runtime_error("Unsupported bit depth for integer PCM decoding: " +
                                      std::to_string(bitsPerSample) + "-bit");
        }
    } else if (audioFormat == AUDIO_FORMAT_IEEE_FLOAT) {
        if (bitsPerSample != 32) {
            throw std::runtime_error("Unsupported bit depth for float PCM decoding: " +
                                      std::to_string(bitsPerSample) + "-bit (only 32-bit float supported)");
        }
        isFloat = true;
    } else {
        throw std::runtime_error("Unsupported audio format for decoding: audioFormat=" +
                                  std::to_string(audioFormat));
    }

    uint32_t bytesPerSample = bitsPerSample / BITS_PER_BYTE;
    uint32_t blockAlign = bytesPerSample * numChannels;
    size_t frameCount = (blockAlign > 0) ? pcmBytes.size() / blockAlign : 0;

    std::vector<std::vector<float>> channels(numChannels, std::vector<float>(frameCount));

    for (size_t frame = 0; frame < frameCount; ++frame) {
        const uint8_t* frameStart = pcmBytes.data() + frame * blockAlign;
        for (uint16_t ch = 0; ch < numChannels; ++ch) {
            const uint8_t* sampleBytes = frameStart + static_cast<size_t>(ch) * bytesPerSample;
            float sample;
            if (isFloat) {
                std::memcpy(&sample, sampleBytes, sizeof(float));
            } else if (bitsPerSample == 8) {
                sample = decodeUnsigned8(*sampleBytes);
            } else {
                sample = decodeSignedInt(sampleBytes, static_cast<int>(bytesPerSample));
            }
            channels[ch][frame] = sample;
        }
    }

    return channels;
}

/** @copydoc encodeSamples */
std::vector<uint8_t> encodeSamples(const std::vector<std::vector<float>>& channelSamples,
                                    uint16_t targetBits) {
    if (targetBits != 8 && targetBits != 16 && targetBits != 24 && targetBits != 32) {
        throw std::runtime_error("Unsupported target bit depth for encoding: " +
                                  std::to_string(targetBits) + "-bit");
    }

    uint16_t numChannels = static_cast<uint16_t>(channelSamples.size());
    size_t frameCount = (numChannels > 0) ? channelSamples[0].size() : 0;
    for (const auto& channel : channelSamples) {
        if (channel.size() != frameCount) {
            throw std::runtime_error("Channel sample count mismatch during encoding");
        }
    }

    uint32_t bytesPerSample = targetBits / BITS_PER_BYTE;
    uint32_t blockAlign = bytesPerSample * numChannels;
    std::vector<uint8_t> pcmBytes(frameCount * blockAlign);

    for (size_t frame = 0; frame < frameCount; ++frame) {
        uint8_t* frameStart = pcmBytes.data() + frame * blockAlign;
        for (uint16_t ch = 0; ch < numChannels; ++ch) {
            uint8_t* sampleBytes = frameStart + static_cast<size_t>(ch) * bytesPerSample;
            float sample = channelSamples[ch][frame];
            if (targetBits == 8) {
                *sampleBytes = encodeUnsigned8(sample);
            } else {
                encodeSignedInt(sample, static_cast<int>(bytesPerSample), sampleBytes);
            }
        }
    }

    return pcmBytes;
}
