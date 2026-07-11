/**
 * @file wav_writer.h
 * @brief WAV file writer API declaration.
 *
 * Provides the public interface for writing WAV/RIFF audio files from
 * in-memory WavMetadata, including all chunk data.
 */

#pragma once

#include <string>
#include "wav_types.h"

/**
 * @brief Writes a WAV file from in-memory metadata and PCM data.
 *
 * Constructs a valid RIFF/WAVE file with "fmt " and "data" chunks, plus any
 * extra chunks stored in meta.extraChunks. All data is written from the
 * WavMetadata struct — no re-reading of the input file is needed.
 *
 * @param outputPath Path to the output WAV file to create.
 * @param meta       Metadata containing all chunk data to write.
 *
 * @throws std::runtime_error If the output file cannot be opened.
 * @throws std::runtime_error If a write error occurs.
 *
 * @see readWavFile
 */
void writeWavFile(const std::string& outputPath, const WavMetadata& meta);
