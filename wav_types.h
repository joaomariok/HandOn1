/**
 * @file wav_types.h
 * @brief Data structures for WAV/RIFF file parsing and metadata representation.
 *
 * Defines POD structs used throughout the WavTool project for reading, and
 * in future phases, writing and resampling WAV audio files.
 */

#pragma once

#include <cstdint>

/// Size of a RIFF chunk ID in bytes.
constexpr int CHUNK_ID_SIZE = 4;

/// Size of the base FmtChunk payload in bytes (PCM format, no extensions).
constexpr uint32_t FMT_BASE_SIZE = 16;

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
 * @brief Represents the payload of the "fmt " sub-chunk in a WAV file.
 *
 * Contains the core audio format parameters as defined by the WAV specification.
 * This struct maps directly to the first 16 bytes of the "fmt " chunk payload
 * (PCM format). Extended format chunks may have additional bytes beyond this.
 */
struct FmtChunk {
    /// Audio format code. 1 = PCM (uncompressed). Other values indicate compression.
    uint16_t audioFormat;
    /// Number of audio channels (1 = mono, 2 = stereo, etc.).
    uint16_t numChannels;
    /// Sample rate in Hz (e.g., 44100, 48000, 16000).
    uint32_t sampleRate;
    /// Average bytes per second: sampleRate * numChannels * (bitsPerSample / 8).
    uint32_t byteRate;
    /// Block alignment in bytes: numChannels * (bitsPerSample / 8). Size of one complete sample frame.
    uint16_t blockAlign;
    /// Bits per sample per channel (e.g., 8, 16, 24, 32).
    uint16_t bitsPerSample;
};

/**
 * @brief Aggregated metadata extracted from a parsed WAV file.
 *
 * Holds the essential audio properties and the location of PCM data within the file.
 * Designed for reuse across all project phases: reading (phase 1), writing (phase 2),
 * and resampling (phase 3). The dataOffset and dataSize fields allow downstream code
 * to seek directly to the PCM payload without re-parsing the file.
 */
struct WavMetadata {
    /// Sample rate in Hz (e.g., 44100, 16000).
    uint32_t sampleRate;
    /// Bits per sample per channel (e.g., 16, 24, 32).
    uint16_t bitsPerSample;
    /// Number of audio channels (1 = mono, 2 = stereo).
    uint16_t numChannels;
    /// Absolute byte offset of the first PCM sample in the file.
    uint32_t dataOffset;
    /// Total size of the PCM data payload in bytes.
    uint32_t dataSize;

    /**
     * @brief Computes the audio duration in seconds from the metadata fields.
     *
     * Formula: dataSize / (sampleRate * numChannels * (bitsPerSample / 8))
     *
     * @return Duration of the audio in seconds as a floating-point value.
     */
    double duration() const {
        uint32_t bytesPerSample = bitsPerSample / 8;
        uint32_t bytesPerSecond = sampleRate * numChannels * bytesPerSample;
        return static_cast<double>(dataSize) / static_cast<double>(bytesPerSecond);
    }
};
