/**
 * @file channel_mixer.cpp
 * @brief Implementation of mono<->stereo channel conversion.
 *
 * Deliberately the simplest possible conversion: averaging and duplication.
 * No stereo-width/panning logic; not production DSP.
 */

#include "channel_mixer.h"

#include <stdexcept>
#include <string>

/** @copydoc convertChannels */
std::vector<std::vector<float>> convertChannels(const std::vector<std::vector<float>>& channelSamples,
                                                 uint16_t targetChannels) {
    uint16_t sourceChannels = static_cast<uint16_t>(channelSamples.size());

    if (sourceChannels != 1 && sourceChannels != 2) {
        throw std::runtime_error("Unsupported source channel count for mixing: " +
                                  std::to_string(sourceChannels));
    }
    if (targetChannels != 1 && targetChannels != 2) {
        throw std::runtime_error("Unsupported target channel count for mixing: " +
                                  std::to_string(targetChannels));
    }

    if (sourceChannels == targetChannels) {
        return channelSamples;
    }

    size_t frameCount = channelSamples[0].size();

    if (sourceChannels == 2 && targetChannels == 1) {
        std::vector<float> mono(frameCount);
        for (size_t i = 0; i < frameCount; ++i) {
            mono[i] = (channelSamples[0][i] + channelSamples[1][i]) * 0.5f;
        }
        return {mono};
    }

    // sourceChannels == 1 && targetChannels == 2
    return {channelSamples[0], channelSamples[0]};
}
