/**
 * @file wav_writer.cpp
 * @brief Implementation of the WAV/RIFF file writer.
 *
 * Writes a valid WAV file by constructing RIFF chunk headers explicitly from
 * in-memory WavMetadata. No input file re-reading is needed.
 */

#include "wav_writer.h"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace {

/**
 * @brief Writes a little-endian integer value of type T to a binary file stream.
 *
 * @tparam T The integer type to write (e.g., uint16_t, uint32_t).
 * @param file The output binary file stream.
 * @param value The value to write in little-endian byte order.
 *
 * @throws std::runtime_error If the write fails.
 *
 * @note Assumes a little-endian host (x64 Windows).
 */
template <typename T>
void writeLE(std::ofstream& file, T value) {
    char buf[sizeof(T)];
    std::memcpy(buf, &value, sizeof(T));
    if (!file.write(buf, sizeof(T))) {
        throw std::runtime_error("Write error");
    }
}

/**
 * @brief Writes a RawChunk (header + data + padding) to the output stream.
 *
 * @param file The output binary file stream.
 * @param chunk The chunk to write.
 *
 * @throws std::runtime_error If a write error occurs.
 */
void writeRawChunk(std::ofstream& file, const RawChunk& chunk) {
    file.write(chunk.header.chunkId, CHUNK_ID_SIZE);
    writeLE<uint32_t>(file, chunk.header.chunkSize);
    if (!chunk.data.empty()) {
        if (!file.write(reinterpret_cast<const char*>(chunk.data.data()),
                        static_cast<std::streamsize>(chunk.data.size()))) {
            throw std::runtime_error("Write error on chunk data");
        }
    }
    if (chunk.header.chunkSize & 1u) {
        char pad = 0;
        file.write(&pad, 1);
    }
}

} // namespace

/** @copydoc writeWavFile */
void writeWavFile(const std::string& outputPath, const WavMetadata& meta) {
    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Could not open output file: " + outputPath);
    }

    // --- RIFF header ---
    output.write(meta.riff.chunkId, CHUNK_ID_SIZE);
    writeLE<uint32_t>(output, meta.riff.chunkSize);
    output.write(meta.riff.formType, CHUNK_ID_SIZE);

    // --- fmt chunk ---
    output.write("fmt ", CHUNK_ID_SIZE);
    writeLE<uint32_t>(output, meta.fmt.chunkSize);
    writeLE<uint16_t>(output, meta.fmt.audioFormat);
    writeLE<uint16_t>(output, meta.fmt.numChannels);
    writeLE<uint32_t>(output, meta.fmt.sampleRate);
    writeLE<uint32_t>(output, meta.fmt.byteRate);
    writeLE<uint16_t>(output, meta.fmt.blockAlign);
    writeLE<uint16_t>(output, meta.fmt.bitsPerSample);
    if (!meta.fmt.extraData.empty()) {
        if (!output.write(reinterpret_cast<const char*>(meta.fmt.extraData.data()),
                          static_cast<std::streamsize>(meta.fmt.extraData.size()))) {
            throw std::runtime_error("Write error on fmt extension data");
        }
    }

    // --- data chunk ---
    writeRawChunk(output, meta.data);

    // --- Extra chunks ---
    for (const auto& chunk : meta.extraChunks) {
        writeRawChunk(output, chunk);
    }
}
