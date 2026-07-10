# WavTool

A C++20 command-line utility that reads WAV audio files, displays their metadata, copies WAV files preserving all original properties, and converts sample rate / channel count / bit depth.

> **IMPORTANT:** Update this file after every implementation change.
> **IMPORTANT:** After every implementation, consider adding or updating tests to cover the new or changed functionality.

## Project Roadmap

| Phase | Description | Status |
|-------|-------------|--------|
| Hands-on 1 | WAV file reading (metadata extraction) | Done |
| Hands-on 2 | WAV file writing | Done |
| Hands-on 3 | Sample rate / channel / bit depth conversion | Done |

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

# Rebuild with parallel jobs (faster)
cmake --build build/vs2022 --config Release --clean-first --parallel
```

Useful flags: `--config <Debug|Release|RelWithDebInfo|MinSizeRel>`, `--clean-first` (rebuild), `--parallel <N>` (parallel jobs). Full flag reference in [CLAUDE.md](CLAUDE.md#build-instructions).

If `cmake` is not on your PATH, use the full path from your VS2022 installation:
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
```

## Usage

```bash
# Read and display metadata only
wav_tool.exe <input.wav>

# Read, display, and copy to a new file (exact copy, all properties preserved)
wav_tool.exe <input.wav> <output.wav>

# Read, display, and convert to new properties (omitted flags keep the source value)
wav_tool.exe <input.wav> <output.wav> [--rate N] [--channels N] [--bits N]
```

`--rate` accepts any positive integer (Hz). `--channels` accepts `1` or `2`. `--bits`
accepts `8`, `16`, `24`, or `32` (integer PCM output only). The source file must be
integer PCM (8/16/24/32-bit) or 32-bit float PCM — other formats (ADPCM, A-law,
μ-law, etc.), unsupported channel counts, or invalid/incomplete flags are rejected
with an error message on stderr and a non-zero exit code.

### Conversion Example

```bash
wav_tool.exe input.wav output.wav --rate 16000 --channels 1 --bits 8
```

```
┌─────────────────────────┬───────────────┐
│ Property                │ Value         │
├─────────────────────────┼───────────────┤
│ Sample Rate             │ 44100 Hz      │
│ Bit Depth               │ 16 bit        │
│ Channels                │ 2             │
│ Duration                │ 29.98 s       │
└─────────────────────────┴───────────────┘

Output:
┌─────────────────────────┬───────────────┐
│ Property                │ Value         │
├─────────────────────────┼───────────────┤
│ Sample Rate             │ 16000 Hz      │
│ Bit Depth               │ 8 bit         │
│ Channels                │ 1             │
│ Duration                │ 29.98 s       │
└─────────────────────────┴───────────────┘
Output written: output.wav (479680 bytes, 16000 Hz / 8 bit / 1 ch)
```

### Copy Example

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
├── .gitignore            # Git ignore rules (build artifacts, IDE, OS files)
├── .vscode/
│   ├── c_cpp_properties.json  # IntelliSense config (delegates to CMake Tools)
│   ├── tasks.json             # Build tasks: CMake configure + build Debug/Release
│   └── launch.json            # Debug configs for wav_tool.exe / wav_tests.exe (cppvsdbg)
├── CMakeLists.txt        # Build configuration (project WavTool, targets wav_tool/wav_tests)
├── CMakePresets.json     # windows-vs2022 (local) + ci (GitHub Actions) presets
├── wav_types.h           # Data structures: RiffEnvelope, FmtChunk, RawChunk, WavMetadata
├── wav_reader.h          # WAV parser API declaration
├── wav_reader.cpp        # WAV parser implementation (RIFF chunk walking)
├── wav_writer.h          # WAV writer API declaration
├── wav_writer.cpp        # WAV writer implementation (RIFF chunk construction)
├── sample_codec.h/.cpp   # PCM<->normalized float decode/encode (8/16/24/32-bit int, 32-bit float)
├── resampler.h/.cpp      # Linear-interpolation sample rate conversion
├── channel_mixer.h/.cpp  # Mono<->stereo channel conversion
├── wav_converter.h/.cpp  # Phase 3 pipeline orchestration + fmt/RIFF regeneration
├── main.cpp              # CLI entry point + formatted table output + flag parsing
├── tests/
│   ├── files/                   # WAV fixtures, named <rate>hz-<bits>bit-<channels>ch[-<codec>][-N].wav
│   │   ├── reference-sample-44100hz-16bit-2ch.wav  # CI-wired fixture (TEST_WAV_PATH)
│   │   └── ...                  # additional fixtures covering other rates/bit depths/channel counts/codecs
│   ├── test_helpers.h          # Shared fixture-path/scratch-dir/wav_tool.exe-runner helpers (header-only)
│   ├── test_wav_reader.cpp     # Catch2 sanity tests for the WAV parser
│   ├── test_wav_writer.cpp     # Catch2 tests for the WAV writer
│   ├── test_sample_codec.cpp   # Catch2 tests for PCM<->float decode/encode
│   ├── test_resampler.cpp      # Catch2 tests for sample rate conversion
│   ├── test_channel_mixer.cpp  # Catch2 tests for mono<->stereo conversion
│   ├── test_wav_converter.cpp  # Catch2 tests for the phase 3 pipeline
│   ├── test_integration.cpp    # In-process multi-module pipeline tests across the fixture set
│   └── test_e2e.cpp            # Spawns wav_tool.exe, asserts on stdout/stderr/exit code/output file
├── CLAUDE.md              # Claude Code agent context
└── README.md              # This file
```

## Running Tests

Tests use **Catch2** (v3.7.1, auto-fetched via CMake FetchContent — no manual install needed).

```bash
# Build (tests are built alongside the main target)
cmake --build build/vs2022 --config Release

# Run all tests
ctest --test-dir build/vs2022 -C Release --output-on-failure

# Run just one file's tests
ctest --test-dir build/vs2022 -C Release -L reader --output-on-failure

# Run the whole unit-test superset (everything except integration/e2e)
ctest --test-dir build/vs2022 -C Release -LE "integration|e2e" --output-on-failure

# Run one broader suite
ctest --test-dir build/vs2022 -C Release -L integration --output-on-failure
ctest --test-dir build/vs2022 -C Release -L e2e --output-on-failure
```

Useful flags: `--output-on-failure`, `-R <regex>` (filter by name), `-L <label>` (filter by label, one per source file: `reader`, `writer`, `codec`, `resampler`, `mixer`, `converter`, `integration`, `e2e`), `-LE <regex>` (exclude by label).

Each of the 8 test source files owns one CTest label and a matching Catch2 tag (e.g. `test_wav_reader.cpp` → `-L reader`). Test-label mechanics and the fixture/tagging scheme are documented in [CLAUDE.md](CLAUDE.md#testing).

## CI/CD

A GitHub Actions workflow (`.github/workflows/build.yml`) runs on every push to `main`:

1. Configures and builds the project in Release mode
2. Runs all Catch2 tests
3. Packages source files + `wav_tool.exe` into a downloadable `WavTool-Release` artifact

Artifacts are available in the **Actions** tab of the GitHub repository.

## Error Handling

The tool reports errors to stderr and exits with code 1 for: missing/incorrect CLI
arguments, a file that isn't a valid RIFF/WAVE container or is missing a required
chunk, unexpected EOF while parsing, invalid or incomplete `--rate`/`--channels`/`--bits`
flags, and unsupported source formats for conversion (not integer PCM at 8/16/24/32-bit,
not 32-bit float PCM, or a channel count other than mono/stereo).

## Implementation Details

For architecture, algorithms, design rules, and coding conventions, see [CLAUDE.md](CLAUDE.md).
