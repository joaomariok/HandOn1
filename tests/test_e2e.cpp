/**
 * @file test_e2e.cpp
 * @brief End-to-end tests: spawns the built wav_tool.exe and asserts on its
 *        stdout/stderr/exit code and any output file it writes.
 */

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

#include "test_helpers.h"
#include "wav_reader.h"

TEST_CASE("wav_tool with no arguments prints usage and exits 1", "[e2e]") {
    ProcessResult result = runWavTool({});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Usage: wav_tool") != std::string::npos);
}

TEST_CASE("wav_tool with one arg prints metadata table and exits 0", "[e2e]") {
    ProcessResult result = runWavTool({fixturePath("reference-sample-44100hz-16bit-2ch.wav")});

    REQUIRE(result.exitCode == 0);
    REQUIRE(result.stdOut.find("Sample Rate") != std::string::npos);
    REQUIRE(result.stdOut.find("44100 Hz") != std::string::npos);
    REQUIRE(result.stdOut.find("16 bit") != std::string::npos);
    REQUIRE(result.stdErr.empty());
}

TEST_CASE("wav_tool with two args writes an output file and prints confirmation", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_copy.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");

    ProcessResult result = runWavTool({input, cleanup.path});

    REQUIRE(result.exitCode == 0);
    REQUIRE(result.stdOut.find("Output written:") != std::string::npos);
    REQUIRE(std::filesystem::exists(cleanup.path));
    REQUIRE(std::filesystem::file_size(cleanup.path) == std::filesystem::file_size(input));
}

TEST_CASE("wav_tool with --rate --channels --bits converts and prints both tables", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_converted.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");

    ProcessResult result =
        runWavTool({input, cleanup.path, "--rate", "22050", "--channels", "1", "--bits", "8"});

    REQUIRE(result.exitCode == 0);
    REQUIRE(result.stdOut.find("Output:") != std::string::npos);
    REQUIRE(result.stdOut.find("22050 Hz / 8 bit / 1 ch") != std::string::npos);

    WavMetadata output = readWavFile(cleanup.path);
    REQUIRE(output.fmt.sampleRate == 22050);
    REQUIRE(output.fmt.bitsPerSample == 8);
    REQUIRE(output.fmt.numChannels == 1);
}

TEST_CASE("wav_tool with a single conversion flag only converts that property", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_single_flag.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");
    WavMetadata original = readWavFile(input);

    ProcessResult result = runWavTool({input, cleanup.path, "--bits", "8"});

    REQUIRE(result.exitCode == 0);
    WavMetadata output = readWavFile(cleanup.path);
    REQUIRE(output.fmt.bitsPerSample == 8);
    REQUIRE(output.fmt.sampleRate == original.fmt.sampleRate);
    REQUIRE(output.fmt.numChannels == original.fmt.numChannels);
}

TEST_CASE("wav_tool rejects nonexistent input file", "[e2e]") {
    ProcessResult result = runWavTool({scratchPath("does_not_exist.wav")});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Error: ") != std::string::npos);
    REQUIRE(result.stdErr.find("Could not open file") != std::string::npos);
}

TEST_CASE("wav_tool rejects malformed input file", "[e2e]") {
    ProcessResult result = runWavTool({fixturePath("glass-malformed.wav")});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("RIFF size mismatch") != std::string::npos);
}

TEST_CASE("wav_tool rejects unknown conversion flag", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_unknown_flag.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");

    ProcessResult result = runWavTool({input, cleanup.path, "--foo", "1"});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Unknown flag:") != std::string::npos);
}

TEST_CASE("wav_tool rejects flag with missing value", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_missing_value.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");

    ProcessResult result = runWavTool({input, cleanup.path, "--rate"});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Missing value for flag") != std::string::npos);
}

TEST_CASE("wav_tool rejects invalid --rate value", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_invalid_rate.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");

    ProcessResult result = runWavTool({input, cleanup.path, "--rate", "abc"});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Invalid --rate value") != std::string::npos);
}

TEST_CASE("wav_tool rejects invalid --channels value", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_invalid_channels.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");

    ProcessResult result = runWavTool({input, cleanup.path, "--channels", "3"});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Invalid --channels value") != std::string::npos);
}

TEST_CASE("wav_tool rejects invalid --bits value", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_invalid_bits.wav")};
    std::string input = fixturePath("reference-sample-44100hz-16bit-2ch.wav");

    ProcessResult result = runWavTool({input, cleanup.path, "--bits", "17"});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Invalid --bits value") != std::string::npos);
}

TEST_CASE("wav_tool rejects unsupported source format at convert stage", "[e2e]") {
    CleanupGuard cleanup{scratchPath("e2e_unsupported_source.wav")};
    std::string input = fixturePath("8000hz-12bit-2ch-pcm.wav");

    ProcessResult result = runWavTool({input, cleanup.path, "--bits", "16"});

    REQUIRE(result.exitCode == 1);
    REQUIRE(result.stdErr.find("Unsupported source bit depth") != std::string::npos);
}
