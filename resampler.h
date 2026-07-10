/**
 * @file resampler.h
 * @brief Linear-interpolation sample rate conversion API declaration.
 *
 * Linear interpolation only, deliberately: this is a didactic exercise, not a
 * production resampler. No windowed sinc, no anti-alias filter, no dithering.
 */

#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief Resamples per-channel float audio using linear interpolation.
 *
 * For each output sample at time t_out, computes the corresponding source
 * position t_out * srcRate / dstRate and linearly interpolates between the two
 * neighboring source samples. The last source sample is repeated for
 * interpolation past the end of the buffer. Applied identically for both
 * upsampling and downsampling.
 *
 * @note This aliases on downsampling since no anti-alias/lowpass filter is
 *       applied — a deliberate, documented tradeoff for this didactic
 *       implementation, not a production bandlimited resampler.
 *
 * @param channelSamples Per-channel float samples at srcRate.
 * @param srcRate Source sample rate in Hz (> 0).
 * @param dstRate Destination sample rate in Hz (> 0).
 * @return Per-channel float samples at dstRate. If srcRate == dstRate, returns
 *         channelSamples unchanged.
 */
std::vector<std::vector<float>> resample(const std::vector<std::vector<float>>& channelSamples,
                                          uint32_t srcRate, uint32_t dstRate);
