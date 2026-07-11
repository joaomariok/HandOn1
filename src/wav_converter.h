/**
 * @file wav_converter.h
 * @brief WAV property conversion orchestration API declaration.
 *
 * Encapsulates the phase 3 pipeline (decode -> channel conversion -> sample
 * rate conversion -> encode) plus fmt/RIFF chunk regeneration, operating
 * purely on in-memory WavMetadata. No file I/O — writeWavFile() is called
 * separately by the caller with the result.
 */

#pragma once

#include <cstdint>

#include "wav_types.h"

/**
 * @brief Requested output properties for a WAV conversion, as parsed from CLI flags.
 *
 * A `has*` flag of false means "keep the source file's value for this property".
 */
struct ConversionOptions {
    /// Whether --rate was given.
    bool hasRate = false;
    /// Requested output sample rate in Hz, valid only if hasRate is true.
    uint32_t rate = 0;
    /// Whether --channels was given.
    bool hasChannels = false;
    /// Requested output channel count (1 or 2), valid only if hasChannels is true.
    uint16_t channels = 0;
    /// Whether --bits was given.
    bool hasBits = false;
    /// Requested output bit depth (8, 16, 24, or 32), valid only if hasBits is true.
    uint16_t bits = 0;

    /// @return true if any of the three flags were given.
    bool any() const { return hasRate || hasChannels || hasBits; }
};

/**
 * @brief Converts a parsed WAV file's sample rate, channel count, and/or bit
 *        depth according to the given options.
 *
 * Validates that the source is convertible (integer PCM at 8/16/24/32-bit, or
 * 32-bit float PCM; mono or stereo), resolves each target property (source
 * value if the corresponding flag was not given), and runs only the pipeline
 * stages whose target differs from the source. Always rebuilds a fresh
 * standard 16-byte PCM fmt chunk (never reuses the source's fmt bytes) since
 * this function is only meant to be called when opts.any() is true.
 *
 * @param input Parsed source WAV metadata (as returned by readWavFile).
 * @param opts Requested output properties.
 * @return A new WavMetadata with the requested properties applied, ready to
 *         pass to writeWavFile().
 *
 * @throws std::runtime_error If the source format is unsupported for conversion
 *         (not integer PCM at 8/16/24/32-bit or 32-bit float PCM, or channel
 *         count other than 1 or 2).
 */
WavMetadata convertWav(const WavMetadata& input, const ConversionOptions& opts);
