/**
 * @file resampler.cpp
 * @brief Implementation of linear-interpolation sample rate conversion.
 *
 * Linear interpolation only, deliberately: this is a didactic exercise, not a
 * production resampler. No windowed sinc, no anti-alias filter, no dithering.
 * Downsampling with this method WILL alias — an accepted, documented tradeoff.
 */

#include "resampler.h"

#include <cmath>

/** @copydoc resample */
std::vector<std::vector<float>> resample(const std::vector<std::vector<float>>& channelSamples,
                                          uint32_t srcRate, uint32_t dstRate) {
    if (srcRate == dstRate) {
        return channelSamples;
    }

    size_t inputLength = channelSamples.empty() ? 0 : channelSamples[0].size();
    size_t outputLength = static_cast<size_t>(std::llround(
        static_cast<double>(inputLength) * static_cast<double>(dstRate) / static_cast<double>(srcRate)));

    std::vector<std::vector<float>> result(channelSamples.size(), std::vector<float>(outputLength));

    if (inputLength == 0) {
        return result;
    }

    for (size_t ch = 0; ch < channelSamples.size(); ++ch) {
        const std::vector<float>& input = channelSamples[ch];
        std::vector<float>& output = result[ch];

        for (size_t i = 0; i < outputLength; ++i) {
            double srcPos = static_cast<double>(i) * srcRate / static_cast<double>(dstRate);
            size_t i0 = static_cast<size_t>(srcPos);
            if (i0 >= inputLength) {
                i0 = inputLength - 1;
            }
            size_t i1 = (i0 + 1 < inputLength) ? i0 + 1 : i0;
            double frac = srcPos - static_cast<double>(i0);

            output[i] = static_cast<float>(input[i0] * (1.0 - frac) + input[i1] * frac);
        }
    }

    return result;
}
