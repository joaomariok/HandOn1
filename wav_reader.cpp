/**
 * @file wav_reader.cpp
 * @brief Implementation of the WAV/RIFF file parser.
 *
 * Implements chunk-walking logic to parse WAV files without hardcoded byte offsets.
 * Reads the RIFF container envelope, then iterates through sub-chunks to extract
 * format metadata and locate the PCM data payload.
 */

#include "wav_reader.h"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace {

/**
 * @brief Reads a little-endian integer value of type T from a binary file stream.
 *
 * Reads exactly sizeof(T) bytes from the current file position and interprets
 * them as a little-endian integer using std::memcpy.
 *
 * @tparam T The integer type to read (e.g., uint16_t, uint32_t).
 * @param file The input binary file stream positioned at the bytes to read.
 * @return The value read from the stream, interpreted as little-endian.
 *
 * @throws std::runtime_error If fewer than sizeof(T) bytes are available (EOF).
 *
 * @note Assumes a little-endian host (x64 Windows). On a big-endian host,
 *       byte-swapping would be required after memcpy.
 */
template <typename T>
T readLE(std::ifstream& file) {
    char buf[sizeof(T)];
    if (!file.read(buf, sizeof(T))) {
        throw std::runtime_error("Unexpected end of file");
    }
    T value;
    std::memcpy(&value, buf, sizeof(T));
    return value;
}

/**
 * @brief Compares a chunk ID against a null-terminated string tag.
 *
 * @param id  The chunk ID read from the RIFF file (CHUNK_ID_SIZE bytes, not null-terminated).
 * @param tag The expected chunk ID as a null-terminated C string (e.g., "fmt ", "data").
 * @return true if the first CHUNK_ID_SIZE bytes of id match tag, false otherwise.
 */
bool idEquals(const char id[CHUNK_ID_SIZE], const char* tag) {
    return std::memcmp(id, tag, CHUNK_ID_SIZE) == 0;
}

} // namespace

/** @copydoc readWavFile */
WavMetadata readWavFile(const std::string& filePath) {
    constexpr const char* RIFF_ID       = "RIFF";
    constexpr const char* WAVE_ID       = "WAVE";
    constexpr const char* FMT_CHUNK_ID  = "fmt ";
    constexpr const char* DATA_CHUNK_ID = "data";

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    // --- RIFF container envelope ---
    RiffChunkHeader riffHeader;
    riffHeader.chunkSize = 0;
    file.read(riffHeader.chunkId, CHUNK_ID_SIZE);
    riffHeader.chunkSize = readLE<uint32_t>(file);

    if (!idEquals(riffHeader.chunkId, RIFF_ID)) {
        throw std::runtime_error("Not a RIFF file");
    }

    char formType[CHUNK_ID_SIZE];
    file.read(formType, CHUNK_ID_SIZE);
    if (!idEquals(formType, WAVE_ID)) {
        throw std::runtime_error("Not a WAVE file");
    }

    // --- Walk sub-chunks ---
    bool foundFmt = false;
    bool foundData = false;
    WavMetadata meta{};

    while (file && !foundData) {
        RiffChunkHeader subHeader;
        file.read(subHeader.chunkId, CHUNK_ID_SIZE);
        subHeader.chunkSize = readLE<uint32_t>(file);

        if (!file) {
            break;
        }

        if (idEquals(subHeader.chunkId, FMT_CHUNK_ID)) {
            if (subHeader.chunkSize < sizeof(FmtChunk)) {
                throw std::runtime_error("fmt chunk too small");
            }

            FmtChunk fmt;
            fmt.audioFormat  = readLE<uint16_t>(file);
            fmt.numChannels  = readLE<uint16_t>(file);
            fmt.sampleRate   = readLE<uint32_t>(file);
            fmt.byteRate     = readLE<uint32_t>(file);
            fmt.blockAlign   = readLE<uint16_t>(file);
            fmt.bitsPerSample = readLE<uint16_t>(file);

            meta.sampleRate    = fmt.sampleRate;
            meta.bitsPerSample = fmt.bitsPerSample;
            meta.numChannels   = fmt.numChannels;
            foundFmt = true;

            // Skip any extra bytes in the fmt chunk beyond FMT_BASE_SIZE
            uint32_t extraBytes = subHeader.chunkSize - FMT_BASE_SIZE;
            if (extraBytes > 0) {
                file.seekg(extraBytes, std::ios::cur);
            }
        } else if (idEquals(subHeader.chunkId, DATA_CHUNK_ID)) {
            meta.dataOffset = static_cast<uint32_t>(file.tellg());
            meta.dataSize   = subHeader.chunkSize;
            foundData = true;
        } else {
            // Skip unknown chunk; RIFF pads odd-sized chunks to even boundary
            uint32_t skipSize = (subHeader.chunkSize + 1) & ~1u;
            file.seekg(skipSize, std::ios::cur);
        }
    }

    if (!foundFmt) {
        throw std::runtime_error("Missing fmt chunk");
    }
    if (!foundData) {
        throw std::runtime_error("Missing data chunk");
    }

    return meta;
}
