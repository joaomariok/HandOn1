/**
 * @file sample_codec.h
 * @brief PCM sample decoding/encoding API declaration.
 *
 * Deliberately the simplest correct approach to PCM<->float conversion for this
 * didactic exercise: no dithering, no noise shaping, no fancy quantization.
 */

#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief Decodes interleaved raw PCM bytes into per-channel normalized float samples.
 *
 * Supports 8-bit unsigned integer PCM, 16/24/32-bit signed integer PCM, and
 * 32-bit IEEE float PCM. Samples are normalized to the range [-1.0, 1.0]
 * (float PCM input is assumed already normalized and is passed through as-is).
 *
 * @param pcmBytes Interleaved raw PCM byte buffer (as read from a "data" chunk).
 * @param audioFormat Source format tag: 1 (integer PCM) or 3 (IEEE float PCM).
 * @param bitsPerSample Source bit depth. Must be 8, 16, 24, or 32 for audioFormat 1;
 *                      must be 32 for audioFormat 3.
 * @param numChannels Source channel count (must be >= 1).
 * @return One vector<float> per channel, each of length pcmBytes.size() / blockAlign.
 *
 * @throws std::runtime_error If the (audioFormat, bitsPerSample) combination is
 *         not one of the supported cases listed above.
 */
std::vector<std::vector<float>> decodeSamples(const std::vector<uint8_t>& pcmBytes,
                                               uint16_t audioFormat,
                                               uint16_t bitsPerSample,
                                               uint16_t numChannels);

/**
 * @brief Encodes per-channel normalized float samples into interleaved integer PCM bytes.
 *
 * Values are clamped to [-1.0, 1.0] before quantization to avoid overflow wraparound.
 * Output is always integer PCM (8-bit unsigned, or 16/24/32-bit signed little-endian).
 *
 * @param channelSamples Per-channel float samples (all channels must have equal length).
 * @param targetBits Target bit depth (must be 8, 16, 24, or 32).
 * @return Interleaved raw PCM byte buffer.
 *
 * @throws std::runtime_error If targetBits is not one of 8/16/24/32.
 */
std::vector<uint8_t> encodeSamples(const std::vector<std::vector<float>>& channelSamples,
                                    uint16_t targetBits);
