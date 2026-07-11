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
    WavMetadata meta{};

    file.read(meta.riff.chunkId, CHUNK_ID_SIZE);
    meta.riff.chunkSize = readLE<uint32_t>(file);

    if (!idEquals(meta.riff.chunkId, RIFF_ID)) {
        throw std::runtime_error("Not a RIFF file");
    }

    file.read(meta.riff.formType, CHUNK_ID_SIZE);
    if (!idEquals(meta.riff.formType, WAVE_ID)) {
        throw std::runtime_error("Not a WAVE file");
    }

    // --- Walk sub-chunks ---
    bool foundFmt = false;
    bool foundData = false;

    std::streamoff riffEnd = static_cast<std::streamoff>(meta.fileSize());

    std::streamoff chunksStart = file.tellg();
    file.seekg(0, std::ios::end);
    std::streamoff fileSize = file.tellg();
    if (fileSize != riffEnd) {
        throw std::runtime_error("RIFF size mismatch: header says " +
            std::to_string(riffEnd) + " bytes but file is " +
            std::to_string(fileSize) + " bytes");
    }
    file.seekg(chunksStart);

    while (file.tellg() < riffEnd) {
        RiffChunkHeader subHeader;
        file.read(subHeader.chunkId, CHUNK_ID_SIZE);
        subHeader.chunkSize = readLE<uint32_t>(file);

        if (idEquals(subHeader.chunkId, FMT_CHUNK_ID)) {
            if (subHeader.chunkSize < FMT_BASE_SIZE) {
                throw std::runtime_error("fmt chunk too small");
            }

            meta.fmt.audioFormat   = readLE<uint16_t>(file);
            meta.fmt.numChannels   = readLE<uint16_t>(file);
            meta.fmt.sampleRate    = readLE<uint32_t>(file);
            meta.fmt.byteRate      = readLE<uint32_t>(file);
            meta.fmt.blockAlign    = readLE<uint16_t>(file);
            meta.fmt.bitsPerSample = readLE<uint16_t>(file);
            meta.fmt.chunkSize     = subHeader.chunkSize;
            foundFmt = true;

            uint32_t extraBytes = subHeader.chunkSize - FMT_BASE_SIZE;
            if (extraBytes > 0) {
                meta.fmt.extraData.resize(extraBytes);
                if (!file.read(reinterpret_cast<char*>(meta.fmt.extraData.data()), extraBytes)) {
                    throw std::runtime_error("Unexpected end of file in fmt chunk");
                }
            }
        } else if (idEquals(subHeader.chunkId, DATA_CHUNK_ID)) {
            meta.data.header = subHeader;
            meta.data.data.resize(subHeader.chunkSize);
            if (!file.read(reinterpret_cast<char*>(meta.data.data.data()), subHeader.chunkSize)) {
                throw std::runtime_error("Unexpected end of file in data chunk");
            }
            foundData = true;

            if (subHeader.chunkSize & 1u) {
                file.seekg(1, std::ios::cur);
            }
        } else {
            RawChunk extra;
            extra.header = subHeader;
            extra.data.resize(subHeader.chunkSize);
            if (!file.read(reinterpret_cast<char*>(extra.data.data()), subHeader.chunkSize)) {
                throw std::runtime_error("Unexpected end of file in extra chunk");
            }
            meta.extraChunks.push_back(std::move(extra));

            if (subHeader.chunkSize & 1u) {
                file.seekg(1, std::ios::cur);
            }
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
