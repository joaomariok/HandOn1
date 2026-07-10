/**
 * @file test_wav_converter.cpp
 * @brief Tests for WAV property conversion orchestration using Catch2.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cstring>

#include "wav_converter.h"

namespace {

WavMetadata makeFixture() {
    WavMetadata meta{};
    std::memcpy(meta.riff.chunkId, "RIFF", CHUNK_ID_SIZE);
    std::memcpy(meta.riff.formType, "WAVE", CHUNK_ID_SIZE);

    meta.fmt.audioFormat = 1;
    meta.fmt.numChannels = 2;
    meta.fmt.sampleRate = 44100;
    meta.fmt.bitsPerSample = 16;
    meta.fmt.blockAlign = 4;
    meta.fmt.byteRate = 44100 * 4;
    meta.fmt.chunkSize = 16;

    // 4 stereo frames of 16-bit PCM.
    meta.data.data = {
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0x7F, 0x00, 0x80,
        0x00, 0x40, 0x00, 0xC0,
        0x00, 0x00, 0xFF, 0x7F,
    };
    std::memcpy(meta.data.header.chunkId, "data", CHUNK_ID_SIZE);
    meta.data.header.chunkSize = static_cast<uint32_t>(meta.data.data.size());

    return meta;
}

} // namespace

TEST_CASE("convertWav changes only sample rate when only --rate is given", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    ConversionOptions opts;
    opts.hasRate = true;
    opts.rate = 22050;

    WavMetadata result = convertWav(fixture, opts);

    REQUIRE(result.fmt.sampleRate == 22050);
    REQUIRE(result.fmt.bitsPerSample == 16);
    REQUIRE(result.fmt.numChannels == 2);
    REQUIRE(result.fmt.chunkSize == 16);
    REQUIRE(result.fmt.extraData.empty());
}

TEST_CASE("convertWav mixes down to mono when only --channels is given", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    ConversionOptions opts;
    opts.hasChannels = true;
    opts.channels = 1;

    WavMetadata result = convertWav(fixture, opts);

    REQUIRE(result.fmt.numChannels == 1);
    REQUIRE(result.fmt.sampleRate == 44100);
    REQUIRE(result.fmt.bitsPerSample == 16);
    REQUIRE(result.data.header.chunkSize == fixture.data.header.chunkSize / 2);
}

TEST_CASE("convertWav requantizes to 8-bit when only --bits is given", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    ConversionOptions opts;
    opts.hasBits = true;
    opts.bits = 8;

    WavMetadata result = convertWav(fixture, opts);

    REQUIRE(result.fmt.bitsPerSample == 8);
    REQUIRE(result.fmt.sampleRate == 44100);
    REQUIRE(result.fmt.numChannels == 2);
    REQUIRE(result.data.header.chunkSize == fixture.data.header.chunkSize / 2);
}

TEST_CASE("convertWav applies all three properties together", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    ConversionOptions opts;
    opts.hasRate = true;
    opts.rate = 16000;
    opts.hasChannels = true;
    opts.channels = 1;
    opts.hasBits = true;
    opts.bits = 8;

    WavMetadata result = convertWav(fixture, opts);

    REQUIRE(result.fmt.sampleRate == 16000);
    REQUIRE(result.fmt.numChannels == 1);
    REQUIRE(result.fmt.bitsPerSample == 8);
    REQUIRE(result.fmt.chunkSize == 16);
    REQUIRE(result.fmt.extraData.empty());
}

TEST_CASE("convertWav rebuilds a fresh fmt even when resolved values equal the source", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    ConversionOptions opts;
    opts.hasRate = true;
    opts.rate = fixture.fmt.sampleRate;
    opts.hasChannels = true;
    opts.channels = fixture.fmt.numChannels;
    opts.hasBits = true;
    opts.bits = fixture.fmt.bitsPerSample;

    WavMetadata result = convertWav(fixture, opts);

    REQUIRE(result.fmt.chunkSize == 16);
    REQUIRE(result.fmt.extraData.empty());
    REQUIRE(result.data.data == fixture.data.data);
}

TEST_CASE("convertWav converts 32-bit float PCM input to integer output", "[converter][unit]") {
    WavMetadata fixture{};
    fixture.fmt.audioFormat = 3;
    fixture.fmt.numChannels = 1;
    fixture.fmt.sampleRate = 44100;
    fixture.fmt.bitsPerSample = 32;
    fixture.fmt.blockAlign = 4;
    fixture.fmt.byteRate = 44100 * 4;
    fixture.fmt.chunkSize = 16;

    std::vector<float> samples = {0.0f, 0.5f, -0.5f};
    fixture.data.data.resize(samples.size() * sizeof(float));
    std::memcpy(fixture.data.data.data(), samples.data(), fixture.data.data.size());
    fixture.data.header.chunkSize = static_cast<uint32_t>(fixture.data.data.size());

    ConversionOptions opts;
    opts.hasBits = true;
    opts.bits = 16;

    WavMetadata result = convertWav(fixture, opts);

    REQUIRE(result.fmt.audioFormat == 1);
    REQUIRE(result.fmt.bitsPerSample == 16);
    REQUIRE(result.data.header.chunkSize == samples.size() * 2);
}

TEST_CASE("convertWav throws on unsupported source format", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    fixture.fmt.audioFormat = 3;
    fixture.fmt.bitsPerSample = 16; // float PCM only supported at 32-bit

    ConversionOptions opts;
    opts.hasBits = true;
    opts.bits = 16;

    REQUIRE_THROWS_WITH(convertWav(fixture, opts),
                        Catch::Matchers::ContainsSubstring("Unsupported source format"));
}

TEST_CASE("convertWav throws on unsupported source bit depth", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    fixture.fmt.bitsPerSample = 12;

    ConversionOptions opts;
    opts.hasBits = true;
    opts.bits = 16;

    REQUIRE_THROWS_WITH(convertWav(fixture, opts),
                        Catch::Matchers::ContainsSubstring("Unsupported source bit depth"));
}

TEST_CASE("convertWav throws on unsupported source channel count", "[converter][unit]") {
    WavMetadata fixture = makeFixture();
    fixture.fmt.numChannels = 4;

    ConversionOptions opts;
    opts.hasBits = true;
    opts.bits = 16;

    REQUIRE_THROWS_WITH(convertWav(fixture, opts),
                        Catch::Matchers::ContainsSubstring("Unsupported source channel count"));
}
