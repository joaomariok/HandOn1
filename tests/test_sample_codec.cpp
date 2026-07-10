/**
 * @file test_sample_codec.cpp
 * @brief Tests for PCM<->float sample decoding/encoding using Catch2.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstring>

#include "sample_codec.h"

namespace {
constexpr uint16_t AUDIO_FORMAT_PCM = 1;
constexpr uint16_t AUDIO_FORMAT_IEEE_FLOAT = 3;
} // namespace

TEST_CASE("decodeSamples handles 8-bit unsigned PCM", "[codec][unit]") {
    std::vector<uint8_t> bytes = {0, 128, 255};
    auto channels = decodeSamples(bytes, AUDIO_FORMAT_PCM, 8, 1);

    REQUIRE(channels.size() == 1);
    REQUIRE_THAT(channels[0][0], Catch::Matchers::WithinAbs(-1.0, 0.01));
    REQUIRE_THAT(channels[0][1], Catch::Matchers::WithinAbs(0.0, 0.01));
    REQUIRE_THAT(channels[0][2], Catch::Matchers::WithinAbs(0.992, 0.01));
}

TEST_CASE("decodeSamples handles 16-bit signed PCM boundary values", "[codec][unit]") {
    std::vector<uint8_t> bytes = {0x00, 0x00, 0xFF, 0x7F, 0x00, 0x80};
    auto channels = decodeSamples(bytes, AUDIO_FORMAT_PCM, 16, 1);

    REQUIRE_THAT(channels[0][0], Catch::Matchers::WithinAbs(0.0, 0.0001));
    REQUIRE_THAT(channels[0][1], Catch::Matchers::WithinAbs(1.0, 0.001));
    REQUIRE_THAT(channels[0][2], Catch::Matchers::WithinAbs(-1.0, 0.0001));
}

TEST_CASE("decodeSamples handles 24-bit signed PCM boundary values", "[codec][unit]") {
    std::vector<uint8_t> bytes = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x80};
    auto channels = decodeSamples(bytes, AUDIO_FORMAT_PCM, 24, 1);

    REQUIRE_THAT(channels[0][0], Catch::Matchers::WithinAbs(0.0, 0.0001));
    REQUIRE_THAT(channels[0][1], Catch::Matchers::WithinAbs(1.0, 0.001));
    REQUIRE_THAT(channels[0][2], Catch::Matchers::WithinAbs(-1.0, 0.0001));
}

TEST_CASE("decodeSamples de-interleaves 2-channel 16-bit PCM", "[codec][unit]") {
    // Frame 0: L=0x7FFF, R=0x8000. Frame 1: L=0x0000, R=0x0000.
    std::vector<uint8_t> bytes = {0xFF, 0x7F, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00};
    auto channels = decodeSamples(bytes, AUDIO_FORMAT_PCM, 16, 2);

    REQUIRE(channels.size() == 2);
    REQUIRE_THAT(channels[0][0], Catch::Matchers::WithinAbs(1.0, 0.001));
    REQUIRE_THAT(channels[1][0], Catch::Matchers::WithinAbs(-1.0, 0.0001));
    REQUIRE_THAT(channels[0][1], Catch::Matchers::WithinAbs(0.0, 0.0001));
    REQUIRE_THAT(channels[1][1], Catch::Matchers::WithinAbs(0.0, 0.0001));
}

TEST_CASE("decodeSamples passes through 32-bit float PCM directly", "[codec][unit]") {
    std::vector<float> values = {0.5f, -1.0f, 0.0f};
    std::vector<uint8_t> bytes(values.size() * sizeof(float));
    std::memcpy(bytes.data(), values.data(), bytes.size());

    auto channels = decodeSamples(bytes, AUDIO_FORMAT_IEEE_FLOAT, 32, 1);

    REQUIRE_THAT(channels[0][0], Catch::Matchers::WithinAbs(0.5, 0.0001));
    REQUIRE_THAT(channels[0][1], Catch::Matchers::WithinAbs(-1.0, 0.0001));
    REQUIRE_THAT(channels[0][2], Catch::Matchers::WithinAbs(0.0, 0.0001));
}

TEST_CASE("decodeSamples throws on unsupported format", "[codec][unit]") {
    std::vector<uint8_t> bytes = {0, 0};
    REQUIRE_THROWS_AS(decodeSamples(bytes, AUDIO_FORMAT_IEEE_FLOAT, 16, 1), std::runtime_error);
    REQUIRE_THROWS_AS(decodeSamples(bytes, 2, 16, 1), std::runtime_error);
}

TEST_CASE("encodeSamples clamps out-of-range floats", "[codec][unit]") {
    std::vector<std::vector<float>> channels = {{1.5f, -1.5f}};
    auto bytes = encodeSamples(channels, 16);

    int16_t maxVal;
    int16_t minVal;
    std::memcpy(&maxVal, bytes.data(), sizeof(int16_t));
    std::memcpy(&minVal, bytes.data() + 2, sizeof(int16_t));
    REQUIRE(maxVal == 32767);
    REQUIRE(minVal == -32767);
}

TEST_CASE("encodeSamples throws on unsupported bit depth", "[codec][unit]") {
    std::vector<std::vector<float>> channels = {{0.0f}};
    REQUIRE_THROWS_AS(encodeSamples(channels, 12), std::runtime_error);
}

TEST_CASE("decode/encode 16-bit round-trip reproduces representative values", "[codec][unit]") {
    std::vector<uint8_t> original = {0x00, 0x00, 0xFF, 0x7F, 0x00, 0x80, 0x34, 0x12};
    auto channels = decodeSamples(original, AUDIO_FORMAT_PCM, 16, 1);
    auto roundTripped = encodeSamples(channels, 16);

    REQUIRE(roundTripped.size() == original.size());
    for (size_t i = 0; i < original.size(); i += 2) {
        int16_t a;
        int16_t b;
        std::memcpy(&a, original.data() + i, sizeof(int16_t));
        std::memcpy(&b, roundTripped.data() + i, sizeof(int16_t));
        REQUIRE(std::abs(static_cast<int>(a) - static_cast<int>(b)) <= 1);
    }
}
