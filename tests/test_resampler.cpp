/**
 * @file test_resampler.cpp
 * @brief Tests for linear-interpolation sample rate conversion using Catch2.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "resampler.h"

TEST_CASE("resample returns input unchanged when rates are equal", "[resampler][unit]") {
    std::vector<std::vector<float>> input = {{0.0f, 0.25f, 0.5f, 1.0f}};
    auto output = resample(input, 44100, 44100);

    REQUIRE(output == input);
}

TEST_CASE("resample upsamples and interpolates midpoints", "[resampler][unit]") {
    std::vector<std::vector<float>> input = {{0.0f, 1.0f, 0.0f, -1.0f}};
    auto output = resample(input, 8000, 16000);

    REQUIRE(output[0].size() == 8);
    REQUIRE_THAT(output[0][0], Catch::Matchers::WithinAbs(0.0, 0.01));
    REQUIRE_THAT(output[0][1], Catch::Matchers::WithinAbs(0.5, 0.01));
    REQUIRE_THAT(output[0][2], Catch::Matchers::WithinAbs(1.0, 0.01));
}

TEST_CASE("resample downsamples to the expected length", "[resampler][unit]") {
    // Downsampling with linear interpolation aliases (no anti-alias filter) —
    // this test only checks length/endpoint correctness, not spectral fidelity.
    std::vector<std::vector<float>> input = {{0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f}};
    auto output = resample(input, 8000, 4000);

    REQUIRE(output[0].size() == 4);
    REQUIRE_THAT(output[0][0], Catch::Matchers::WithinAbs(0.0, 0.01));
}

TEST_CASE("resample preserves per-channel independence", "[resampler][unit]") {
    std::vector<std::vector<float>> input = {{0.0f, 1.0f}, {1.0f, 0.0f}};
    auto output = resample(input, 8000, 16000);

    REQUIRE(output.size() == 2);
    REQUIRE(output[0].size() == output[1].size());
    REQUIRE_THAT(output[0][0], Catch::Matchers::WithinAbs(0.0, 0.01));
    REQUIRE_THAT(output[1][0], Catch::Matchers::WithinAbs(1.0, 0.01));
}
