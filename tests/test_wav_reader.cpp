/**
 * @file test_wav_reader.cpp
 * @brief Sanity tests for the WAV/RIFF parser using Catch2 and reference-sample-44100hz-16bit-2ch.wav.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "wav_reader.h"
#include "wav_types.h"

#ifndef TEST_WAV_PATH
#error "TEST_WAV_PATH must be defined via CMake"
#endif

// --- Sanity tests against reference-sample-44100hz-16bit-2ch.wav ---

TEST_CASE("readWavFile parses reference-sample-44100hz-16bit-2ch.wav without throwing", "[reader][unit]") {
    REQUIRE_NOTHROW(readWavFile(TEST_WAV_PATH));
}

TEST_CASE("reference-sample-44100hz-16bit-2ch.wav has expected metadata", "[reader][unit]") {
    WavMetadata meta = readWavFile(TEST_WAV_PATH);

    SECTION("sample rate is 44100 Hz") {
        REQUIRE(meta.fmt.sampleRate == 44100);
    }

    SECTION("bit depth is 16 bits") {
        REQUIRE(meta.fmt.bitsPerSample == 16);
    }

    SECTION("channel count is 2 (stereo)") {
        REQUIRE(meta.fmt.numChannels == 2);
    }

    SECTION("data chunk has samples") {
        REQUIRE(!meta.data.data.empty());
    }

    SECTION("data size is non-zero") {
        REQUIRE(meta.data.header.chunkSize > 0);
    }

    SECTION("duration is approximately 29.98 seconds") {
        REQUIRE_THAT(meta.duration(),
                     Catch::Matchers::WithinAbs(29.98, 0.01));
    }
}

// --- Error handling tests ---

TEST_CASE("readWavFile throws on non-existent file", "[reader][unit]") {
    REQUIRE_THROWS_AS(readWavFile("nonexistent_file.wav"), std::runtime_error);
}

TEST_CASE("readWavFile error message contains file path", "[reader][unit]") {
    REQUIRE_THROWS_WITH(readWavFile("nonexistent_file.wav"),
                        Catch::Matchers::ContainsSubstring("Could not open file"));
}

// --- WavMetadata::duration() unit tests ---

TEST_CASE("WavMetadata::duration() computes correctly", "[reader][unit]") {
    SECTION("1 second of 16kHz mono 16-bit audio") {
        WavMetadata meta{};
        meta.fmt.sampleRate = 16000;
        meta.fmt.bitsPerSample = 16;
        meta.fmt.numChannels = 1;
        meta.data.header.chunkSize = 32000; // 16000 * 1 * 2 = 32000 bytes/sec
        REQUIRE_THAT(meta.duration(), Catch::Matchers::WithinAbs(1.0, 0.0001));
    }

    SECTION("2.5 seconds of 44100Hz stereo 16-bit audio") {
        WavMetadata meta{};
        meta.fmt.sampleRate = 44100;
        meta.fmt.bitsPerSample = 16;
        meta.fmt.numChannels = 2;
        meta.data.header.chunkSize = 441000; // 44100 * 2 * 2 * 2.5 = 441000
        REQUIRE_THAT(meta.duration(), Catch::Matchers::WithinAbs(2.5, 0.0001));
    }
}
