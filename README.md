# WavTool

A C++20 command-line utility that reads WAV audio files and displays their metadata in a formatted table.

> **IMPORTANT:** Update this file after every implementation change.
> **IMPORTANT:** After every implementation, consider adding or updating tests to cover the new or changed functionality.

## Project Roadmap

| Phase | Description | Status |
|-------|-------------|--------|
| Hands-on 1 | WAV file reading (metadata extraction) | Done |
| Hands-on 2 | WAV file writing | Pending |
| Hands-on 3 | Downsampling/resampling (16 kHz / 16-bit) | Pending |

## Prerequisites

- **Visual Studio 2022** (Community, Professional, or Enterprise)
- **CMake 3.28+** (bundled with VS2022, or install separately)
- **Windows 10/11** x64

## Build Instructions

### 1. Configure

```bash
cmake --preset windows-vs2022
```

### 2. Build

```bash
# Debug
cmake --build build/vs2022 --config Debug

# Release
cmake --build build/vs2022 --config Release
```

If `cmake` is not on your PATH, use the full path from your VS2022 installation:
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
```

## Usage

```bash
wav_tool.exe <path_to_file.wav>
```

### Example Output

```
┌─────────────────────────┬───────────────┐
│ Property                │ Value         │
├─────────────────────────┼───────────────┤
│ Sample Rate             │ 16000 Hz      │
│ Bit Depth               │ 16 bit        │
│ Channels                │ 2             │
│ Duration                │ 33.53 s       │
└─────────────────────────┴───────────────┘
```

## Project Structure

```
HandOn1/
├── .github/
│   └── workflows/
│       └── build.yml           # GitHub Actions: build, test, and package on push to main
├── .gitignore                  # Git ignore rules (build artifacts, IDE, OS files)
├── .vscode/
│   └── c_cpp_properties.json  # IntelliSense config (delegates to CMake Tools)
├── CMakeLists.txt             # Build configuration (project WavTool, target wav_tool)
├── CMakePresets.json           # VS2022 configure + build presets
├── wav_types.h                 # Data structures: RiffChunkHeader, FmtChunk, WavMetadata
├── wav_reader.h                # WAV parser API declaration
├── wav_reader.cpp              # WAV parser implementation (RIFF chunk walking)
├── main.cpp                    # CLI entry point + formatted table output
├── tests/
│   ├── test_file.wav           # Test WAV file (44100 Hz, 16-bit, stereo)
│   └── test_wav_reader.cpp     # Catch2 sanity tests for the WAV parser
├── CLAUDE.md                   # Claude Code agent context
└── README.md                   # This file
```

## Running Tests

Tests use **Catch2** (v3.7.1, auto-fetched via CMake FetchContent — no manual install needed).

```bash
# Build (tests are built alongside the main target)
cmake --build build/vs2022 --config Release

# Run all tests
ctest --test-dir build/vs2022 -C Release --output-on-failure
```

Test categories: `[sanity]` (parser vs. test_file.wav), `[error]` (exception handling), `[unit]` (duration calculation).

## CI/CD

A GitHub Actions workflow (`.github/workflows/build.yml`) runs on every push to `main`:

1. Configures and builds the project in Release mode
2. Runs all Catch2 tests
3. Packages source files + `wav_tool.exe` into a downloadable `WavTool-Release` artifact

Artifacts are available in the **Actions** tab of the GitHub repository.

## Documentation

All source code is documented using **Doxygen** style comments (`/** */` with `@brief`, `@param`, `@return`, `@throws`, etc.). Struct fields use `///` inline comments. File-level `@file` headers are present in every source file.

## How It Works

The parser implements RIFF chunk traversal — it does **not** use hardcoded byte offsets to locate WAV sub-chunks.

1. Opens the file in binary mode
2. Reads the 12-byte RIFF container envelope and validates the "RIFF" and "WAVE" signatures
3. Enters a loop, reading each sub-chunk header (4-byte ID + 4-byte size):
   - **"fmt " chunk** — extracts audio format, channels, sample rate, bit depth
   - **"data" chunk** — records the byte offset and size of the PCM audio data
   - **Unknown chunks** — skips forward by the chunk size (with RIFF even-boundary padding)
4. Computes duration: `dataSize / (sampleRate × numChannels × (bitsPerSample / 8))`
5. Displays results in a Unicode box-drawing table

## Error Handling

The tool reports errors to stderr and exits with code 1 for:

- Missing or incorrect command-line arguments (prints usage)
- File not found or cannot be opened
- File is not a valid RIFF container
- File is RIFF but not WAVE format
- Missing required "fmt " chunk
- Missing required "data" chunk
- Unexpected end of file during parsing

## Future Phases

- **Hands-on 2** will add WAV file writing capabilities, reusing `WavMetadata` and the existing type definitions
- **Hands-on 3** will add audio downsampling/resampling to 16 kHz / 16-bit, reading PCM data via `dataOffset`/`dataSize` from `WavMetadata`
