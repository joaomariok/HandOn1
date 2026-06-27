# WavTool

A C++20 command-line utility that reads WAV audio files, displays their metadata, and copies WAV files preserving all original properties.

> **IMPORTANT:** Update this file after every implementation change.
> **IMPORTANT:** After every implementation, consider adding or updating tests to cover the new or changed functionality.

## Project Roadmap

| Phase | Description | Status |
|-------|-------------|--------|
| Hands-on 1 | WAV file reading (metadata extraction) | Done |
| Hands-on 2 | WAV file writing | Done |
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
# Read and display metadata only
wav_tool.exe <input.wav>

# Read, display, and copy to a new file
wav_tool.exe <input.wav> <output.wav>
```

### Example Output

```
┌─────────────────────────┬───────────────┐
│ Property                │ Value         │
├─────────────────────────┼───────────────┤
│ Sample Rate             │ 44100 Hz      │
│ Bit Depth               │ 16 bit        │
│ Channels                │ 2             │
│ Duration                │ 29.98 s       │
└─────────────────────────┴───────────────┘
Output written: output.wav (5289194 bytes)
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
├── wav_types.h                 # Data structures: RiffEnvelope, FmtChunk, RawChunk, WavMetadata
├── wav_reader.h                # WAV parser API declaration
├── wav_reader.cpp              # WAV parser implementation (RIFF chunk walking)
├── wav_writer.h                # WAV writer API declaration
├── wav_writer.cpp              # WAV writer implementation (RIFF chunk construction)
├── main.cpp                    # CLI entry point + formatted table output
├── tests/
│   ├── test_file.wav           # Test WAV file (44100 Hz, 16-bit, stereo)
│   ├── test_wav_reader.cpp     # Catch2 sanity tests for the WAV parser
│   └── test_wav_writer.cpp     # Catch2 tests for the WAV writer
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

Test categories: `[sanity]` (parser vs. test_file.wav), `[error]` (exception handling), `[unit]` (duration calculation), `[writer]` (WAV writer output validation).

## CI/CD

A GitHub Actions workflow (`.github/workflows/build.yml`) runs on every push to `main`:

1. Configures and builds the project in Release mode
2. Runs all Catch2 tests
3. Packages source files + `wav_tool.exe` into a downloadable `WavTool-Release` artifact

Artifacts are available in the **Actions** tab of the GitHub repository.

## Documentation

All source code is documented using **Doxygen** style comments (`/** */` with `@brief`, `@param`, `@return`, `@throws`, etc.). Struct fields use `///` inline comments. File-level `@file` headers are present in every source file. Explicit types are preferred over `auto` unless the type is a lambda or excessively complex.

## How It Works

### Reading

The parser implements RIFF chunk traversal — it does **not** use hardcoded byte offsets to locate WAV sub-chunks.

1. Opens the file in binary mode
2. Reads the 12-byte RIFF container envelope and validates the "RIFF" and "WAVE" signatures
3. Enters a loop bounded by the RIFF payload size, reading each sub-chunk header (4-byte ID + 4-byte size):
   - **"fmt " chunk** — extracts audio format, channels, sample rate, bit depth, plus any extension bytes
   - **"data" chunk** — reads the PCM audio samples into memory
   - **Unknown chunks** — captures them into memory for preservation in the output file
4. Computes duration: `dataSize / (sampleRate × numChannels × (bitsPerSample / 8))`
5. Displays results in a Unicode box-drawing table

### Writing

When an output path is provided, the writer constructs a new WAV file from the in-memory structs (no re-reading of the input file):

1. Writes the RIFF header from the stored envelope
2. Writes the "fmt " chunk field-by-field, including any extension bytes
3. Writes the "data" chunk with the PCM samples
4. Writes any captured extra chunks (LIST, id3, etc.) to preserve file size parity

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

- **Hands-on 3** will add audio downsampling/resampling to 16 kHz / 16-bit, processing PCM data from `WavMetadata::data` and writing the result with updated fmt parameters
