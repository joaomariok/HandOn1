/**
 * @file wav_reader.h
 * @brief WAV file parser API declaration.
 *
 * Provides the public interface for reading and parsing WAV/RIFF audio files.
 * The parser uses dynamic chunk traversal — it never relies on hardcoded byte offsets.
 */

#pragma once

#include <string>
#include "wav_types.h"

/**
 * @brief Parses a WAV/RIFF file and extracts its audio metadata.
 *
 * Opens the specified file in binary mode, validates the RIFF/WAVE container,
 * and walks the chunk list dynamically to locate the "fmt " and "data" sub-chunks.
 * No hardcoded byte offsets are used — each chunk is discovered by reading its
 * header (ID + size) and advancing accordingly.
 *
 * @param filePath Path to the WAV file to parse.
 * @return WavMetadata struct populated with sample rate, bit depth, channel count,
 *         and the absolute offset/size of the PCM data payload.
 *
 * @throws std::runtime_error If the file cannot be opened.
 * @throws std::runtime_error If the file is not a valid RIFF container.
 * @throws std::runtime_error If the RIFF form type is not "WAVE".
 * @throws std::runtime_error If the "fmt " sub-chunk is missing.
 * @throws std::runtime_error If the "data" sub-chunk is missing.
 * @throws std::runtime_error If an unexpected end-of-file is encountered during parsing.
 *
 * @note The returned WavMetadata includes dataOffset and dataSize, allowing callers
 *       to reopen the file and seek directly to the PCM payload for reading or resampling.
 *
 * @see WavMetadata
 */
WavMetadata readWavFile(const std::string& filePath);
