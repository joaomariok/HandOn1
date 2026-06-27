# WavTool — Claude Code Project Context

## IMPORTANT: Update this file after every implementation change.
## IMPORTANT: Update README.md after every implementation change to keep it consistent with this file.
## IMPORTANT: After every implementation, consider adding or updating tests to cover the new or changed functionality.

## Project Overview

WavTool is a C++20 console application that parses WAV/RIFF audio files, displays metadata, and copies WAV files preserving all properties. This is **phase 2 of a 3-phase project**:

1. **Hands-on 1 (done)** — WAV file reading: parse RIFF chunks, extract and display metadata
2. **Hands-on 2 (current)** — WAV file writing: write WAV files with the same properties
3. **Hands-on 3** — Downsampling/resampling to 16 kHz / 16-bit + rewrite

No external dependencies. No audio libraries. Only C++ standard library headers.

## Build Instructions

Target: Visual Studio 2022, x64. CMake 3.28+ required.

CMake is bundled with VS2022 at:
```
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
```

Commands (use full cmake path if not on PATH):
```bash
# Configure
cmake --preset windows-vs2022

# Build (Debug)
cmake --build build/vs2022 --config Debug

# Build (Release)
cmake --build build/vs2022 --config Release

# Rebuild with parallel jobs (faster)
cmake --build build/vs2022 --config Release --clean-first --parallel
```

Useful build flags:
- `--config <cfg>` — `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`
- `--clean-first` — clean before building (rebuild)
- `--target <name>` — build a specific target (e.g. `wav_tool`, `wav_tests`, `clean`)
- `--parallel <N>` / `-j <N>` — parallel build jobs
- `--verbose` / `-v` — show compiler commands
- `cmake --preset windows-vs2022 --fresh` — delete cache and re-configure from scratch

Run: `build\vs2022\Release\wav_tool.exe <input.wav> [output.wav]`

## Architecture

### `wav_types.h` — Type definitions (structs with encapsulated size calculations)
- `CHUNK_ID_SIZE` — constexpr, size of a RIFF chunk ID in bytes (4)
- `FMT_BASE_SIZE` — constexpr, size of the base FmtChunk payload in bytes (16)
- `RiffChunkHeader` — `chunkId[CHUNK_ID_SIZE]` + `chunkSize` (generic 8-byte header)
- `RiffEnvelope` — RIFF container: `chunkId`, `chunkSize`, `formType`; `totalSize()` returns file size
- `FmtChunk` — audio format fields + `chunkSize` + `extraData` (extension bytes); `totalSize()` returns 8 + chunkSize
- `RawChunk` — generic chunk: `RiffChunkHeader header` + `vector<uint8_t> data`; `totalSize()` returns 8 + padded chunkSize. Used for both "data" (PCM samples) and extra chunks (LIST, id3, etc.)
- `WavMetadata` — aggregates all envelopes: `riff`, `fmt`, `data`, `extraChunks`
  - `extraChunksSize()` — sum of extra chunk totalSize() (available for external use)
  - `fileSize()` — delegates to `riff.totalSize()`
  - `duration()` — `data.header.chunkSize / (sampleRate * numChannels * bytesPerSample)`

### `wav_reader.h` / `wav_reader.cpp` — WAV parser
- **Public API**: `WavMetadata readWavFile(const std::string& filePath)`
- **Internal helpers** (anonymous namespace):
  - `template<typename T> T readLE(std::ifstream&)` — reads sizeof(T) bytes as little-endian value
  - `bool idEquals(const char id[CHUNK_ID_SIZE], const char* tag)` — chunk ID comparison
- **Algorithm**: reads RIFF envelope into `meta.riff`, then loops bounded by `riffEnd` reading chunk headers. Populates `meta.fmt`, reads PCM samples into `meta.data.data`, captures unknown chunks into `meta.extraChunks`. File is read once — all data stored in memory.
- **Errors**: throws `std::runtime_error` for: file not found, not RIFF, not WAVE, missing fmt, missing data, unexpected EOF

### `wav_writer.h` / `wav_writer.cpp` — WAV writer
- **Public API**: `void writeWavFile(const std::string& outputPath, const WavMetadata& meta)`
- **Internal helpers** (anonymous namespace):
  - `template<typename T> void writeLE(std::ofstream&, T)` — writes sizeof(T) bytes as little-endian value
  - `void writeRawChunk(std::ofstream&, const RawChunk&)` — writes header + data + padding
- **Algorithm**: writes RIFF header from `meta.riff`, fmt chunk field-by-field, data chunk via `writeRawChunk`, then extra chunks via `writeRawChunk`. No input file re-reading needed.
- **Errors**: throws `std::runtime_error` for: cannot open output, write errors

### `main.cpp` — CLI entry point + display
- `void printMetadataTable(const WavMetadata&)` — renders UTF-8 box-drawing table to stdout (accesses fields via `meta.fmt.*`)
- `int main(int argc, char* argv[])` — validates args (1 or 2), calls `readWavFile()`, prints table; if output path given, calls `writeWavFile()` and prints confirmation with file size

## Key Design Rules

1. **NO hardcoded byte offsets** — the parser walks the RIFF chunk list dynamically. Never use `seekg` to a magic offset like 12 or 36.
2. **RIFF chunk padding** — odd-sized chunks are padded to even boundary. Skip size: `(chunkSize + 1) & ~1u`
3. **Little-endian assumption** — WAV is LE; code uses `memcpy` into native types. This is correct on x64 Windows. Noted in source for future portability.
4. **No external dependencies** — only `<fstream>`, `<cstdint>`, `<cstring>`, `<string>`, `<stdexcept>`, `<iostream>`, `<iomanip>`, `<sstream>`, `<vector>`, `<filesystem>`
5. **Extensibility** — Each RIFF envelope has its own struct with `totalSize()`. `WavMetadata` aggregates all envelopes and provides `fileSize()`, `duration()`. `printMetadataTable()` is a standalone function for reuse.
6. **Read once** — The reader loads all data into memory (PCM samples, extra chunks). The writer writes from in-memory structs without re-reading the input file.

## Coding Conventions

- C++20, enforced via `target_compile_features`
- `#pragma once` for header guards
- Doxygen documentation on all structs, functions, templates, and member variables (`/** */` blocks with `@brief`, `@param`, `@return`, `@throws`, `@note`, `@see`; `///` for struct fields)
- Prefer explicit types over `auto` unless the type is a lambda or excessively complex
- Inline comments only when the WHY is non-obvious
- Minimal error handling — only at system boundaries (file I/O, CLI args)
- Errors to stderr, non-zero exit code on failure

## CMake Structure

- Project name: `WavTool`
- Target: `wav_tool`
- Presets defined in `CMakePresets.json`:
  - Configure: `windows-vs2022` (VS 17 2022, x64, build dir: `build/vs2022`)
  - Build: `windows-vs2022-debug`, `windows-vs2022-release`

## Testing

- Framework: **Catch2 v3.7.1** (auto-fetched via CMake FetchContent)
- Test target: `wav_tests` (links `wav_reader.cpp`, `wav_writer.cpp` + `tests/test_wav_reader.cpp`, `tests/test_wav_writer.cpp`)
- Test WAV path passed via compile definition `TEST_WAV_PATH`
- Test discovery via `catch_discover_tests()`
- Run tests: `ctest --test-dir build/vs2022 -C Release --output-on-failure`
- Useful ctest flags: `--output-on-failure`, `-R <regex>` (filter tests), `-V` (verbose)

Test categories:
- `[sanity]` — validates parser output against known `test_file.wav` metadata
- `[error]` — verifies exception handling for invalid inputs
- `[unit]` — tests `WavMetadata::duration()` with synthetic values
- `[writer]` — validates WAV writer output: file size, PCM data integrity, re-parsability

## CI/CD

- GitHub Actions workflow: `.github/workflows/build.yml`
- Triggers on every push to `main`
- Runs on `windows-latest` with MSVC via `ilammy/msvc-dev-cmd@v1`
- Uses the `ci` CMake preset (Ninja generator) — the VS2022 generator is not available on GitHub runners
- Steps: configure → build Release → run tests → package artifact
- Artifact `WavTool-Release`: contains source files, tests, and `wav_tool.exe`

## VS Code Configuration

- `.vscode/c_cpp_properties.json` — IntelliSense config: delegates to CMake Tools extension (`ms-vscode.cmake-tools`) for include paths, defines, and compiler settings.
