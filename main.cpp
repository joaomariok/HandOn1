/**
 * @file main.cpp
 * @brief CLI entry point and formatted metadata display for the WavTool application.
 *
 * Handles command-line argument parsing, invokes the WAV parser, and renders
 * the extracted audio metadata in a Unicode box-drawing table to stdout.
 */

#include "wav_converter.h"
#include "wav_reader.h"
#include "wav_writer.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

/// @name Box-drawing characters (UTF-8 encoded)
/// @{
constexpr const char* BOX_HORIZONTAL   = "\xe2\x94\x80"; // ─
constexpr const char* BOX_VERTICAL     = "\xe2\x94\x82"; // │
constexpr const char* BOX_TOP_LEFT     = "\xe2\x94\x8c"; // ┌
constexpr const char* BOX_TOP_RIGHT    = "\xe2\x94\x90"; // ┐
constexpr const char* BOX_BOTTOM_LEFT  = "\xe2\x94\x94"; // └
constexpr const char* BOX_BOTTOM_RIGHT = "\xe2\x94\x98"; // ┘
constexpr const char* BOX_T_DOWN       = "\xe2\x94\xac"; // ┬
constexpr const char* BOX_T_UP         = "\xe2\x94\xb4"; // ┴
constexpr const char* BOX_T_RIGHT      = "\xe2\x94\x9c"; // ├
constexpr const char* BOX_T_LEFT       = "\xe2\x94\xa4"; // ┤
constexpr const char* BOX_CROSS        = "\xe2\x94\xbc"; // ┼
/// @}

/// One space on each side of cell content.
constexpr int COL_PADDING = 2;

/**
 * @brief Prints WAV audio metadata in a formatted Unicode box-drawing table.
 *
 * Renders a two-column table to stdout displaying sample rate, bit depth,
 * number of channels, and computed duration. Uses UTF-8 box-drawing characters
 * (U+2500 series) for the table borders.
 *
 * Column widths: 23 characters (label) + 13 characters (value).
 *
 * @param meta The WavMetadata struct containing the audio properties to display.
 *
 * @note This function is designed as a standalone utility for reuse in future
 *       project phases (WAV writing, resampling) that may also need to display metadata.
 */
void printMetadataTable(const WavMetadata& meta) {
    constexpr int colLeft  = 23;
    constexpr int colRight = 13;

    auto hLine = [&](const char* left, const char* mid, const char* right) {
        std::cout << left;
        for (int i = 0; i < colLeft + COL_PADDING; ++i) std::cout << BOX_HORIZONTAL;
        std::cout << mid;
        for (int i = 0; i < colRight + COL_PADDING; ++i) std::cout << BOX_HORIZONTAL;
        std::cout << right << "\n";
    };

    auto row = [&](const std::string& label, const std::string& value) {
        std::cout << BOX_VERTICAL << " "
                  << std::left << std::setw(colLeft) << label
                  << " " << BOX_VERTICAL << " "
                  << std::left << std::setw(colRight) << value
                  << " " << BOX_VERTICAL << "\n";
    };

    hLine(BOX_TOP_LEFT, BOX_T_DOWN, BOX_TOP_RIGHT);
    row("Property", "Value");
    hLine(BOX_T_RIGHT, BOX_CROSS, BOX_T_LEFT);

    row("Sample Rate", std::to_string(meta.fmt.sampleRate) + " Hz");
    row("Bit Depth",   std::to_string(meta.fmt.bitsPerSample) + " bit");
    row("Channels",    std::to_string(meta.fmt.numChannels));

    std::ostringstream durStr;
    durStr << std::fixed << std::setprecision(2) << meta.duration() << " s";
    row("Duration", durStr.str());

    hLine(BOX_BOTTOM_LEFT, BOX_T_UP, BOX_BOTTOM_RIGHT);
}

/**
 * @brief Parses the trailing --rate/--channels/--bits flags from argv.
 *
 * Flags are read in pairs starting at argv[3] (argv[1] is the input path,
 * argv[2] is the output path). Each flag's value is validated; an omitted
 * flag leaves the corresponding ConversionOptions field unset.
 *
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @return Parsed and validated ConversionOptions.
 *
 * @throws std::runtime_error If a flag is unknown, missing its value, or has
 *         an invalid value (--rate not a positive integer, --channels not 1
 *         or 2, --bits not 8/16/24/32).
 */
ConversionOptions parseConversionFlags(int argc, char* argv[]) {
    ConversionOptions opts;

    if ((argc - 3) % 2 != 0) {
        throw std::runtime_error(std::string("Missing value for flag '") + argv[argc - 1] + "'");
    }

    for (int i = 3; i < argc; i += 2) {
        std::string flag = argv[i];
        std::string value = argv[i + 1];

        if (flag == "--rate") {
            char* end = nullptr;
            long parsed = std::strtol(value.c_str(), &end, 10);
            bool isValid = !value.empty() && end != nullptr && *end == '\0' &&
                            parsed > 0 && std::to_string(parsed) == value;
            if (!isValid) {
                throw std::runtime_error("Invalid --rate value: '" + value +
                                          "' (must be a positive integer)");
            }
            opts.hasRate = true;
            opts.rate = static_cast<uint32_t>(parsed);
        } else if (flag == "--channels") {
            if (value != "1" && value != "2") {
                throw std::runtime_error("Invalid --channels value: '" + value +
                                          "' (must be 1 or 2)");
            }
            opts.hasChannels = true;
            opts.channels = static_cast<uint16_t>(std::stoi(value));
        } else if (flag == "--bits") {
            if (value != "8" && value != "16" && value != "24" && value != "32") {
                throw std::runtime_error("Invalid --bits value: '" + value +
                                          "' (must be 8, 16, 24, or 32)");
            }
            opts.hasBits = true;
            opts.bits = static_cast<uint16_t>(std::stoi(value));
        } else {
            throw std::runtime_error("Unknown flag: '" + flag + "'");
        }
    }

    return opts;
}

/**
 * @brief Application entry point. Parses a WAV file, displays its metadata,
 *        and optionally writes an (optionally converted) output file.
 *
 * Usage: wav_tool <input.wav> [output.wav] [--rate N] [--channels N] [--bits N]
 *
 * @param argc Argument count.
 * @param argv Argument vector. argv[1] is the input path; argv[2] (if present)
 *             is the output path; any remaining arguments are conversion flags.
 * @return 0 on success, 1 on error (wrong arguments, invalid flags, parse/write failure).
 */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2) {
        std::cerr << "Usage: wav_tool <input.wav> [output.wav] [--rate N] [--channels N] [--bits N]\n";
        return 1;
    }

    try {
        WavMetadata meta = readWavFile(argv[1]);
        printMetadataTable(meta);

        if (argc == 2) {
            return 0;
        }

        if (argc == 3) {
            writeWavFile(argv[2], meta);
            std::uintmax_t size = std::filesystem::file_size(argv[2]);
            std::cout << "Output written: " << argv[2] << " (" << size << " bytes)\n";
            return 0;
        }

        ConversionOptions opts = parseConversionFlags(argc, argv);
        WavMetadata converted = convertWav(meta, opts);
        writeWavFile(argv[2], converted);

        std::uintmax_t size = std::filesystem::file_size(argv[2]);
        std::cout << "\nOutput:\n";
        printMetadataTable(converted);
        std::cout << "Output written: " << argv[2] << " (" << size << " bytes, "
                  << converted.fmt.sampleRate << " Hz / " << converted.fmt.bitsPerSample
                  << " bit / " << converted.fmt.numChannels << " ch)\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
