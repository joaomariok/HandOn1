/**
 * @file test_integration.cpp
 * @brief Integration tests: in-process multi-module pipelines (read -> convert
 *        -> write -> re-read) exercised across the tests/files/ fixture set.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstdio>
#include <cstring>

#include "sample_codec.h"
#include "test_helpers.h"
#include "wav_converter.h"
#include "wav_reader.h"
#include "wav_writer.h"

namespace {

/// One fixture's expected source properties, used to drive GENERATE-based loops.
struct FixtureCase {
    std::string filename;
    uint32_t sourceRate;
    uint16_t sourceBits;
    uint16_t sourceChannels;
};

/// All 22 fixtures readWavFile can parse (excludes the 2 malformed fixtures).
const std::vector<FixtureCase>& readableFixtures() {
    static const std::vector<FixtureCase> fixtures = {
        {"16000hz-16bit-1ch-pcm.wav", 16000, 16, 1},
        {"22050hz-16bit-1ch-pcm.wav", 22050, 16, 1},
        {"22050hz-16bit-2ch-pcm.wav", 22050, 16, 2},
        {"44100hz-16bit-2ch-pcm-1.wav", 44100, 16, 2},
        {"44100hz-16bit-2ch-pcm-2.wav", 44100, 16, 2},
        {"44100hz-16bit-2ch-pcm-3.wav", 44100, 16, 2},
        {"44100hz-32bit-1ch-pcm.wav", 44100, 32, 1},
        {"48000hz-16bit-1ch-pcm.wav", 48000, 16, 1},
        {"48000hz-24bit-1ch-pcm.wav", 48000, 24, 1},
        {"8000hz-12bit-2ch-pcm.wav", 8000, 12, 2},
        {"8000hz-16bit-1ch-pcm.wav", 8000, 16, 1},
        {"8000hz-16bit-2ch-pcm.wav", 8000, 16, 2},
        {"8000hz-24bit-2ch-pcm.wav", 8000, 24, 2},
        {"8000hz-32bit-2ch-float.wav", 8000, 32, 2},
        {"8000hz-32bit-2ch-pcm.wav", 8000, 32, 2},
        {"8000hz-64bit-2ch-float.wav", 8000, 64, 2},
        {"8000hz-8bit-1ch-pcm.wav", 8000, 8, 1},
        {"8000hz-8bit-2ch-alaw.wav", 8000, 8, 2},
        {"8000hz-8bit-2ch-mulaw.wav", 8000, 8, 2},
        {"8000hz-8bit-2ch-pcm.wav", 8000, 8, 2},
        {"96000hz-16bit-1ch-pcm.wav", 96000, 16, 1},
        {"reference-sample-44100hz-16bit-2ch.wav", 44100, 16, 2},
    };
    return fixtures;
}

/// The 18 fixtures convertWav accepts (excludes 12-bit PCM, 64-bit float, A-law, mu-law).
std::vector<FixtureCase> convertibleFixtures() {
    std::vector<FixtureCase> result;
    for (const FixtureCase& f : readableFixtures()) {
        if (f.filename == "8000hz-12bit-2ch-pcm.wav" || f.filename == "8000hz-64bit-2ch-float.wav" ||
            f.filename == "8000hz-8bit-2ch-alaw.wav" || f.filename == "8000hz-8bit-2ch-mulaw.wav") {
            continue;
        }
        result.push_back(f);
    }
    return result;
}

std::vector<std::string> convertRejectedFixtures() {
    return {"8000hz-12bit-2ch-pcm.wav", "8000hz-64bit-2ch-float.wav", "8000hz-8bit-2ch-alaw.wav",
            "8000hz-8bit-2ch-mulaw.wav"};
}

std::vector<std::string> malformedFixtures() {
    return {"glass-malformed.wav", "ptjunk-malformed.wav"};
}

/// Picks a target differing from the source in every property, always a valid
/// convertWav target (rate is always positive, bits is one of 8/16/24/32, channels is 1 or 2).
ConversionOptions differingTarget(uint32_t sourceRate, uint16_t sourceBits, uint16_t sourceChannels) {
    ConversionOptions opts;
    opts.hasRate = true;
    opts.rate = (sourceRate == 44100) ? 22050 : 44100;
    opts.hasBits = true;
    opts.bits = (sourceBits == 16) ? 8 : 16;
    opts.hasChannels = true;
    opts.channels = (sourceChannels == 1) ? 2 : 1;
    return opts;
}

WavMetadata makeSilenceFixture(uint32_t rate, uint16_t bits, uint16_t channels, size_t frameCount) {
    WavMetadata meta{};
    std::memcpy(meta.riff.chunkId, "RIFF", CHUNK_ID_SIZE);
    std::memcpy(meta.riff.formType, "WAVE", CHUNK_ID_SIZE);

    meta.fmt.audioFormat = 1;
    meta.fmt.numChannels = channels;
    meta.fmt.sampleRate = rate;
    meta.fmt.bitsPerSample = bits;
    meta.fmt.blockAlign = static_cast<uint16_t>(channels * (bits / BITS_PER_BYTE));
    meta.fmt.byteRate = rate * meta.fmt.blockAlign;
    meta.fmt.chunkSize = FMT_BASE_SIZE;

    // 8-bit PCM is unsigned with 0x80 as the zero point; all other supported
    // depths are signed with 0x00 as the zero point.
    uint8_t zeroByte = (bits == 8) ? 0x80 : 0x00;
    meta.data.data.assign(frameCount * meta.fmt.blockAlign, zeroByte);
    std::memcpy(meta.data.header.chunkId, "data", CHUNK_ID_SIZE);
    meta.data.header.chunkSize = static_cast<uint32_t>(meta.data.data.size());

    return meta;
}

} // namespace

TEST_CASE("readWavFile + writeWavFile round trip preserves metadata across all readable fixtures",
          "[integration]") {
    FixtureCase fixture = GENERATE(from_range(readableFixtures()));
    CAPTURE(fixture.filename);

    CleanupGuard cleanup{scratchPath(fixture.filename + ".copy.wav")};
    WavMetadata original = readWavFile(fixturePath(fixture.filename));

    REQUIRE(original.fmt.sampleRate == fixture.sourceRate);
    REQUIRE(original.fmt.bitsPerSample == fixture.sourceBits);
    REQUIRE(original.fmt.numChannels == fixture.sourceChannels);

    writeWavFile(cleanup.path, original);
    WavMetadata copy = readWavFile(cleanup.path);

    REQUIRE(copy.fmt.sampleRate == original.fmt.sampleRate);
    REQUIRE(copy.fmt.bitsPerSample == original.fmt.bitsPerSample);
    REQUIRE(copy.fmt.numChannels == original.fmt.numChannels);
    REQUIRE(copy.data.header.chunkSize == original.data.header.chunkSize);
    REQUIRE(copy.data.data == original.data.data);
}

TEST_CASE("convertWav produces requested properties across all convertible fixtures",
          "[integration]") {
    FixtureCase fixture = GENERATE(from_range(convertibleFixtures()));
    CAPTURE(fixture.filename);

    CleanupGuard cleanup{scratchPath(fixture.filename + ".converted.wav")};
    WavMetadata original = readWavFile(fixturePath(fixture.filename));
    double originalDuration = original.duration();

    ConversionOptions opts = differingTarget(fixture.sourceRate, fixture.sourceBits, fixture.sourceChannels);
    WavMetadata converted = convertWav(original, opts);

    REQUIRE(converted.fmt.sampleRate == opts.rate);
    REQUIRE(converted.fmt.bitsPerSample == opts.bits);
    REQUIRE(converted.fmt.numChannels == opts.channels);
    REQUIRE_THAT(converted.duration(), Catch::Matchers::WithinAbs(originalDuration, 0.01));

    writeWavFile(cleanup.path, converted);
    WavMetadata reread = readWavFile(cleanup.path);

    REQUIRE(reread.fmt.sampleRate == opts.rate);
    REQUIRE(reread.fmt.bitsPerSample == opts.bits);
    REQUIRE(reread.fmt.numChannels == opts.channels);
}

TEST_CASE("convertWav with only --rate changes rate and preserves bits/channels",
          "[integration]") {
    WavMetadata original = readWavFile(TEST_WAV_PATH);
    ConversionOptions opts;
    opts.hasRate = true;
    opts.rate = 22050;

    WavMetadata result = convertWav(original, opts);

    REQUIRE(result.fmt.sampleRate == 22050);
    REQUIRE(result.fmt.bitsPerSample == original.fmt.bitsPerSample);
    REQUIRE(result.fmt.numChannels == original.fmt.numChannels);
}

TEST_CASE("convertWav with only --channels changes channels and preserves rate/bits",
          "[integration]") {
    WavMetadata original = readWavFile(TEST_WAV_PATH);
    ConversionOptions opts;
    opts.hasChannels = true;
    opts.channels = 1;

    WavMetadata result = convertWav(original, opts);

    REQUIRE(result.fmt.numChannels == 1);
    REQUIRE(result.fmt.sampleRate == original.fmt.sampleRate);
    REQUIRE(result.fmt.bitsPerSample == original.fmt.bitsPerSample);
}

TEST_CASE("convertWav with only --bits changes bits and preserves rate/channels",
          "[integration]") {
    WavMetadata original = readWavFile(TEST_WAV_PATH);
    ConversionOptions opts;
    opts.hasBits = true;
    opts.bits = 8;

    WavMetadata result = convertWav(original, opts);

    REQUIRE(result.fmt.bitsPerSample == 8);
    REQUIRE(result.fmt.sampleRate == original.fmt.sampleRate);
    REQUIRE(result.fmt.numChannels == original.fmt.numChannels);
}

TEST_CASE("convertWav identity conversion round-trips byte-for-byte",
          "[integration]") {
    WavMetadata original = readWavFile(TEST_WAV_PATH);
    ConversionOptions opts;
    opts.hasRate = true;
    opts.rate = original.fmt.sampleRate;
    opts.hasChannels = true;
    opts.channels = original.fmt.numChannels;
    opts.hasBits = true;
    opts.bits = original.fmt.bitsPerSample;

    WavMetadata result = convertWav(original, opts);

    REQUIRE(result.data.data == original.data.data);
}

TEST_CASE("convertWav rejects the fixtures with unsupported source formats",
          "[integration]") {
    std::string filename = GENERATE(from_range(convertRejectedFixtures()));
    CAPTURE(filename);

    WavMetadata source = readWavFile(fixturePath(filename));
    ConversionOptions opts;
    opts.hasBits = true;
    opts.bits = 16;

    REQUIRE_THROWS_AS(convertWav(source, opts), std::runtime_error);
}

TEST_CASE("readWavFile rejects malformed fixtures before any pipeline stage runs",
          "[integration]") {
    std::string filename = GENERATE(from_range(malformedFixtures()));
    CAPTURE(filename);

    REQUIRE_THROWS_AS(readWavFile(fixturePath(filename)), std::runtime_error);
}

TEST_CASE("converting a silent buffer through rate/bits/channels change stays silent",
          "[integration]") {
    WavMetadata silence = makeSilenceFixture(44100, 16, 2, 100);

    ConversionOptions opts;
    opts.hasRate = true;
    opts.rate = 22050;
    opts.hasBits = true;
    opts.bits = 8;
    opts.hasChannels = true;
    opts.channels = 1;

    WavMetadata converted = convertWav(silence, opts);
    std::vector<std::vector<float>> decoded = decodeSamples(
        converted.data.data, converted.fmt.audioFormat, converted.fmt.bitsPerSample, converted.fmt.numChannels);

    REQUIRE(decoded.size() == 1);
    for (float sample : decoded[0]) {
        REQUIRE_THAT(sample, Catch::Matchers::WithinAbs(0.0, 0.01));
    }
}
