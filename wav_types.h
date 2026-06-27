/**
 * @file wav_types.h
 * @brief Data structures for WAV/RIFF file parsing, writing, and metadata representation.
 *
 * Each RIFF envelope (RIFF, fmt, data, extra) is represented by its own struct
 * with encapsulated size calculations. WavMetadata aggregates all envelopes.
 */

#pragma once

#include <cstdint>
#include <vector>

/// Size of a RIFF chunk ID in bytes.
constexpr int CHUNK_ID_SIZE = 4;

/// Size of the base FmtChunk payload in bytes (PCM format, no extensions).
constexpr uint32_t FMT_BASE_SIZE = 16;

/// Size of a RIFF chunk header in bytes (4-byte ID + 4-byte size field).
constexpr uint32_t CHUNK_HEADER_SIZE = CHUNK_ID_SIZE + sizeof(uint32_t);

/// Number of bits in one byte, used for bits-to-bytes conversions.
constexpr uint32_t BITS_PER_BYTE = 8;

/**
 * @brief Generic RIFF chunk header used during chunk-list traversal.
 *
 * Every RIFF sub-chunk begins with this 8-byte header. The parser reads it
 * to identify the chunk type and determine how many bytes to read or skip.
 */
struct RiffChunkHeader {
    /// ASCII identifier (e.g., "fmt ", "data", "LIST"). Not null-terminated.
    char chunkId[CHUNK_ID_SIZE];
    /// Size of the chunk payload in bytes. Excludes the 8-byte header itself.
    uint32_t chunkSize;
};

/**
 * @brief Represents the RIFF container envelope.
 *
 * Stores the RIFF chunk ID, payload size, and WAVE form type as read from
 * the file header.
 */
struct RiffEnvelope {
    /// RIFF chunk identifier ("RIFF"). Not null-terminated.
    char chunkId[CHUNK_ID_SIZE]{};
    /// RIFF payload size as read from the file (file size minus 8).
    uint32_t chunkSize{};
    /// RIFF form type ("WAVE"). Not null-terminated.
    char formType[CHUNK_ID_SIZE]{};

    /**
     * @brief Total file size implied by this RIFF envelope.
     *
     * @return 8 (RIFF header) + chunkSize.
     */
    uint32_t totalSize() const { return CHUNK_HEADER_SIZE + chunkSize; }
};

/**
 * @brief Represents the "fmt " sub-chunk: audio format parameters and any extensions.
 *
 * Contains the core 16-byte PCM format fields plus any extension bytes beyond
 * FMT_BASE_SIZE.
 */
struct FmtChunk {
    /// Audio format code. 1 = PCM (uncompressed). Other values indicate compression.
    uint16_t audioFormat{};
    /// Number of audio channels (1 = mono, 2 = stereo, etc.).
    uint16_t numChannels{};
    /// Sample rate in Hz (e.g., 44100, 48000, 16000).
    uint32_t sampleRate{};
    /// Average bytes per second: sampleRate * numChannels * (bitsPerSample / 8).
    uint32_t byteRate{};
    /// Block alignment in bytes: numChannels * (bitsPerSample / 8). Size of one complete sample frame.
    uint16_t blockAlign{};
    /// Bits per sample per channel (e.g., 8, 16, 24, 32).
    uint16_t bitsPerSample{};
    /// Original "fmt " chunk payload size in bytes (>= FMT_BASE_SIZE).
    uint32_t chunkSize{};
    /// Extension bytes beyond FMT_BASE_SIZE (e.g. cbSize + codec-specific data).
    std::vector<uint8_t> extraData;

    /**
     * @brief Total size of this chunk in the RIFF file, including the 8-byte header.
     *
     * @return 8 (header) + chunkSize.
     */
    uint32_t totalSize() const { return CHUNK_HEADER_SIZE + ((chunkSize + 1) & ~1u); }
};

/**
 * @brief Generic RIFF sub-chunk storing its header and raw payload bytes.
 *
 * Used for both the "data" chunk (PCM samples) and any unknown chunks
 * (LIST, id3, etc.) that need to be preserved in the output.
 */
struct RawChunk {
    /// Chunk header as read from the file.
    RiffChunkHeader header{};
    /// Raw payload bytes (PCM samples for "data", opaque bytes for others).
    std::vector<uint8_t> data;

    /**
     * @brief Total size of this chunk in the RIFF file, including header and padding.
     *
     * @return 8 (header) + header.chunkSize rounded up to even boundary.
     */
    uint32_t totalSize() const {
        return CHUNK_HEADER_SIZE + ((header.chunkSize + 1) & ~1u);
    }
};

/**
 * @brief Aggregated metadata extracted from a parsed WAV file.
 *
 * Contains all RIFF envelopes: riff, fmt, data, and any extra chunks. Provides
 * convenience methods for computing file size and audio duration.
 */
struct WavMetadata {
    /// RIFF container envelope.
    RiffEnvelope riff;
    /// Parsed "fmt " chunk with audio format parameters and extensions.
    FmtChunk fmt;
    /// "data" chunk with PCM audio payload.
    RawChunk data;
    /// Non-fmt/non-data RIFF sub-chunks preserved for size parity.
    std::vector<RawChunk> extraChunks;

    /**
     * @brief Total output file size, derived from the RIFF envelope.
     *
     * @return riff.totalSize() (8 + riff.chunkSize).
     */
    uint32_t fileSize() const { return riff.totalSize(); }

    /**
     * @brief Computes the audio duration in seconds.
     *
     * @return data payload size / (sampleRate * numChannels * bytesPerSample).
     */
    double duration() const {
        uint32_t bytesPerSample = fmt.bitsPerSample / BITS_PER_BYTE;
        uint32_t bytesPerSecond = fmt.sampleRate * fmt.numChannels * bytesPerSample;
        return static_cast<double>(data.header.chunkSize) / static_cast<double>(bytesPerSecond);
    }
};
