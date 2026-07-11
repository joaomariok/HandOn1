/**
 * @file channel_mixer.h
 * @brief Mono<->stereo channel conversion API declaration.
 *
 * Deliberately the simplest possible conversion: averaging and duplication.
 * No stereo-width/panning logic; not production DSP.
 */

#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief Converts between mono and stereo per-channel float sample buffers.
 *
 * Stereo to mono averages the two channels sample-by-sample. Mono to stereo
 * duplicates the single channel into both output channels.
 *
 * @param channelSamples Input per-channel samples; must have exactly 1 or 2 channels.
 * @param targetChannels Target channel count; must be 1 or 2.
 * @return Per-channel samples with result.size() == targetChannels. If
 *         channelSamples.size() == targetChannels, returns the input unchanged.
 *
 * @throws std::runtime_error If channelSamples.size() or targetChannels is not 1 or 2.
 */
std::vector<std::vector<float>> convertChannels(const std::vector<std::vector<float>>& channelSamples,
                                                 uint16_t targetChannels);
