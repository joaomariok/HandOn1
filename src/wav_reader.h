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
 *         and the full PCM data payload (loaded into meta.data.data).
 *
 * @throws std::runtime_error If the file cannot be opened.
 * @throws std::runtime_error If the file is not a valid RIFF container.
 * @throws std::runtime_error If the RIFF form type is not "WAVE".
 * @throws std::runtime_error If the header-declared RIFF size doesn't match the actual file size.
 * @throws std::runtime_error If the "fmt " sub-chunk is present but smaller than FMT_BASE_SIZE.
 * @throws std::runtime_error If the "fmt " sub-chunk is missing.
 * @throws std::runtime_error If the "data" sub-chunk is missing.
 * @throws std::runtime_error If an unexpected end-of-file is encountered during parsing.
 *
 * @note The file is read once: PCM samples and any unknown chunks are copied into
 *       memory (WavMetadata::data and WavMetadata::extraChunks), so callers never
 *       need to reopen or re-seek the input file.
 *
 * @see WavMetadata
 */
WavMetadata readWavFile(const std::string& filePath);
