/**
 * @file wav_converter.cpp
 * @brief Implementation of WAV property conversion orchestration.
 *
 * Pipeline order: decode -> channel conversion -> sample rate conversion ->
 * encode, running only the stages whose target differs from the source. The
 * fmt and RIFF envelopes are always regenerated on this path.
 */

#include "wav_converter.h"

#include <cstring>
#include <stdexcept>
#include <string>

#include "channel_mixer.h"
#include "resampler.h"
#include "sample_codec.h"

namespace {

constexpr uint16_t AUDIO_FORMAT_PCM = 1;
constexpr uint16_t AUDIO_FORMAT_IEEE_FLOAT = 3;

/**
 * @brief Validates that the source WAV's format is supported for conversion.
 *
 * @throws std::runtime_error If audioFormat/bitsPerSample/numChannels are not
 *         one of the supported combinations.
 */
void validateSourceFormat(const FmtChunk& fmt) {
    if (fmt.audioFormat == AUDIO_FORMAT_PCM) {
        if (fmt.bitsPerSample != 8 && fmt.bitsPerSample != 16 &&
            fmt.bitsPerSample != 24 && fmt.bitsPerSample != 32) {
            throw std::runtime_error("Unsupported source bit depth for conversion: " +
                                      std::to_string(fmt.bitsPerSample) +
                                      "-bit (only 8/16/24/32-bit PCM supported)");
        }
    } else if (fmt.audioFormat == AUDIO_FORMAT_IEEE_FLOAT) {
        if (fmt.bitsPerSample != 32) {
            throw std::runtime_error(
                "Unsupported source format for conversion: audioFormat=" +
                std::to_string(fmt.audioFormat) + ", " + std::to_string(fmt.bitsPerSample) +
                "-bit (supported: 8/16/24/32-bit integer PCM, or 32-bit float PCM)");
        }
    } else {
        throw std::runtime_error(
            "Unsupported source format for conversion: audioFormat=" +
            std::to_string(fmt.audioFormat) + ", " + std::to_string(fmt.bitsPerSample) +
            "-bit (supported: 8/16/24/32-bit integer PCM, or 32-bit float PCM)");
    }

    if (fmt.numChannels != 1 && fmt.numChannels != 2) {
        throw std::runtime_error("Unsupported source channel count for conversion: " +
                                  std::to_string(fmt.numChannels) +
                                  " (only mono/stereo supported)");
    }
}

/**
 * @brief Builds a fresh standard 16-byte PCM fmt chunk for the given target properties.
 */
FmtChunk buildFreshFmt(uint32_t rate, uint16_t channels, uint16_t bits) {
    FmtChunk fmt{};
    fmt.audioFormat = AUDIO_FORMAT_PCM;
    fmt.numChannels = channels;
    fmt.sampleRate = rate;
    fmt.bitsPerSample = bits;
    fmt.blockAlign = static_cast<uint16_t>(channels * (bits / BITS_PER_BYTE));
    fmt.byteRate = rate * fmt.blockAlign;
    fmt.chunkSize = FMT_BASE_SIZE;
    fmt.extraData.clear();
    return fmt;
}

} // namespace

/** @copydoc convertWav */
WavMetadata convertWav(const WavMetadata& input, const ConversionOptions& opts) {
    validateSourceFormat(input.fmt);

    uint32_t targetRate = opts.hasRate ? opts.rate : input.fmt.sampleRate;
    uint16_t targetChannels = opts.hasChannels ? opts.channels : input.fmt.numChannels;
    uint16_t targetBits = opts.hasBits ? opts.bits : input.fmt.bitsPerSample;

    bool needChannelConv = targetChannels != input.fmt.numChannels;
    bool needRateConv = targetRate != input.fmt.sampleRate;
    bool needBitsConv = targetBits != input.fmt.bitsPerSample;
    bool needsAudioFormatConv = input.fmt.audioFormat != AUDIO_FORMAT_PCM;

    WavMetadata output = input;
    output.fmt = buildFreshFmt(targetRate, targetChannels, targetBits);

    if (needChannelConv || needRateConv || needBitsConv || needsAudioFormatConv) {
        std::vector<std::vector<float>> channelSamples =
            decodeSamples(input.data.data, input.fmt.audioFormat, input.fmt.bitsPerSample,
                          input.fmt.numChannels);

        if (needChannelConv) {
            channelSamples = convertChannels(channelSamples, targetChannels);
        }
        if (needRateConv) {
            channelSamples = resample(channelSamples, input.fmt.sampleRate, targetRate);
        }

        std::vector<uint8_t> newData = encodeSamples(channelSamples, targetBits);

        std::memcpy(output.data.header.chunkId, "data", CHUNK_ID_SIZE);
        output.data.header.chunkSize = static_cast<uint32_t>(newData.size());
        output.data.data = std::move(newData);
    }

    std::memcpy(output.riff.chunkId, "RIFF", CHUNK_ID_SIZE);
    std::memcpy(output.riff.formType, "WAVE", CHUNK_ID_SIZE);

    uint32_t extraTotal = 0;
    for (const auto& chunk : output.extraChunks) {
        extraTotal += chunk.totalSize();
    }
    output.riff.chunkSize = CHUNK_ID_SIZE + output.fmt.totalSize() + output.data.totalSize() + extraTotal;

    return output;
}
