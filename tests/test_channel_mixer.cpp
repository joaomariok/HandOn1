/**
 * @file test_channel_mixer.cpp
 * @brief Tests for mono<->stereo channel conversion using Catch2.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "channel_mixer.h"

TEST_CASE("convertChannels averages stereo to mono", "[mixer][unit]") {
    std::vector<std::vector<float>> stereo = {{1.0f, -1.0f}, {0.0f, 1.0f}};
    auto mono = convertChannels(stereo, 1);

    REQUIRE(mono.size() == 1);
    REQUIRE_THAT(mono[0][0], Catch::Matchers::WithinAbs(0.5, 0.0001));
    REQUIRE_THAT(mono[0][1], Catch::Matchers::WithinAbs(0.0, 0.0001));
}

TEST_CASE("convertChannels duplicates mono to stereo", "[mixer][unit]") {
    std::vector<std::vector<float>> mono = {{0.3f, -0.7f}};
    auto stereo = convertChannels(mono, 2);

    REQUIRE(stereo.size() == 2);
    REQUIRE(stereo[0] == mono[0]);
    REQUIRE(stereo[1] == mono[0]);
}

TEST_CASE("convertChannels is a no-op when target equals source", "[mixer][unit]") {
    std::vector<std::vector<float>> stereo = {{0.1f, 0.2f}, {0.3f, 0.4f}};
    auto result = convertChannels(stereo, 2);

    REQUIRE(result == stereo);
}

TEST_CASE("convertChannels throws on unsupported channel counts", "[mixer][unit]") {
    std::vector<std::vector<float>> threeChannels = {{0.0f}, {0.0f}, {0.0f}};
    REQUIRE_THROWS_AS(convertChannels(threeChannels, 2), std::runtime_error);

    std::vector<std::vector<float>> mono = {{0.0f}};
    REQUIRE_THROWS_AS(convertChannels(mono, 3), std::runtime_error);
}
