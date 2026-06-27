/**
 * @file test_wav_writer.cpp
 * @brief Tests for the WAV writer using Catch2.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

#include "wav_reader.h"
#include "wav_writer.h"

#ifndef TEST_WAV_PATH
#error "TEST_WAV_PATH must be defined via CMake"
#endif

namespace {

const std::string OUTPUT_PATH = std::string(TEST_WAV_PATH) + ".out.wav";

struct CleanupGuard {
    ~CleanupGuard() { std::remove(OUTPUT_PATH.c_str()); }
};

} // namespace

TEST_CASE("writeWavFile produces a file with the same size as the input", "[writer]") {
    CleanupGuard cleanup;
    WavMetadata meta = readWavFile(TEST_WAV_PATH);
    writeWavFile(OUTPUT_PATH, meta);

    std::uintmax_t inputSize = std::filesystem::file_size(TEST_WAV_PATH);
    std::uintmax_t outputSize = std::filesystem::file_size(OUTPUT_PATH);
    REQUIRE(outputSize == inputSize);
}

TEST_CASE("writeWavFile output can be re-parsed with matching metadata", "[writer]") {
    CleanupGuard cleanup;
    WavMetadata original = readWavFile(TEST_WAV_PATH);
    writeWavFile(OUTPUT_PATH, original);

    WavMetadata reparsed = readWavFile(OUTPUT_PATH);
    REQUIRE(reparsed.fmt.sampleRate == original.fmt.sampleRate);
    REQUIRE(reparsed.fmt.bitsPerSample == original.fmt.bitsPerSample);
    REQUIRE(reparsed.fmt.numChannels == original.fmt.numChannels);
    REQUIRE(reparsed.data.header.chunkSize == original.data.header.chunkSize);
}

TEST_CASE("writeWavFile throws on invalid output path", "[writer][error]") {
    WavMetadata meta = readWavFile(TEST_WAV_PATH);
    REQUIRE_THROWS_AS(writeWavFile("Z:/nonexistent/dir/out.wav", meta), std::runtime_error);
}
