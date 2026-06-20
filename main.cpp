/**
 * @file main.cpp
 * @brief CLI entry point and formatted metadata display for the WavTool application.
 *
 * Handles command-line argument parsing, invokes the WAV parser, and renders
 * the extracted audio metadata in a Unicode box-drawing table to stdout.
 */

#include "wav_reader.h"

#include <iomanip>
#include <iostream>
#include <sstream>
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

    row("Sample Rate", std::to_string(meta.sampleRate) + " Hz");
    row("Bit Depth",   std::to_string(meta.bitsPerSample) + " bit");
    row("Channels",    std::to_string(meta.numChannels));

    std::ostringstream durStr;
    durStr << std::fixed << std::setprecision(2) << meta.duration() << " s";
    row("Duration", durStr.str());

    hLine(BOX_BOTTOM_LEFT, BOX_T_UP, BOX_BOTTOM_RIGHT);
}

/**
 * @brief Application entry point. Parses a WAV file and displays its metadata.
 *
 * Usage: wav_tool <path_to_file.wav>
 *
 * @param argc Argument count. Must be exactly 2.
 * @param argv Argument vector. argv[1] is the path to the WAV file.
 * @return 0 on success, 1 on error (wrong arguments, file not found, parse failure).
 */
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc != 2) {
        std::cerr << "Usage: wav_tool <path_to_file.wav>\n";
        return 1;
    }

    try {
        WavMetadata meta = readWavFile(argv[1]);
        printMetadataTable(meta);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
