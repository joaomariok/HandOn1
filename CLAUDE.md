# WavTool — Claude Code Project Context

## IMPORTANT: Update this file after every implementation change.
## IMPORTANT: After every implementation, consider adding or updating tests to cover the new or changed functionality.

## Project Overview

WavTool is a C++20 console application that parses WAV/RIFF audio files and displays metadata. This is **phase 1 of a 3-phase project**:

1. **Hands-on 1 (current)** — WAV file reading: parse RIFF chunks, extract and display metadata
2. **Hands-on 2** — WAV file writing: write WAV files with the same properties
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
```

Run: `build\vs2022\Release\wav_tool.exe <path_to_file.wav>`

## Architecture

### `wav_types.h` — Type definitions (POD structs, no logic except `duration()`)
- `RiffChunkHeader` — `chunkId[4]` + `chunkSize` (used to walk RIFF chunk list)
- `FmtChunk` — full fmt sub-chunk fields: `audioFormat`, `numChannels`, `sampleRate`, `byteRate`, `blockAlign`, `bitsPerSample`
- `WavMetadata` — aggregated result: `sampleRate`, `bitsPerSample`, `numChannels`, `dataOffset`, `dataSize`
  - `double duration()` — computes `dataSize / (sampleRate * numChannels * (bitsPerSample / 8))`
  - `dataOffset` and `dataSize` exist so future phases can seek directly to PCM data without re-parsing

### `wav_reader.h` / `wav_reader.cpp` — WAV parser
- **Public API**: `WavMetadata readWavFile(const std::string& filePath)`
- **Internal helpers** (anonymous namespace):
  - `template<typename T> T readLE(std::ifstream&)` — reads sizeof(T) bytes as little-endian value
  - `bool idEquals(const char id[4], const char* tag)` — 4-byte chunk ID comparison
- **Algorithm**: reads RIFF envelope (12 bytes: "RIFF" + size + "WAVE"), then loops reading chunk headers (ID + size). Processes "fmt " and "data", skips unknown chunks with RIFF padding.
- **Errors**: throws `std::runtime_error` for: file not found, not RIFF, not WAVE, missing fmt, missing data, unexpected EOF

### `main.cpp` — CLI entry point + display
- `void printMetadataTable(const WavMetadata&)` — reusable function, renders UTF-8 box-drawing table to stdout (23-char label column + 13-char value column)
- `int main(int argc, char* argv[])` — validates args, calls `readWavFile()`, prints table or error to stderr

## Key Design Rules

1. **NO hardcoded byte offsets** — the parser walks the RIFF chunk list dynamically. Never use `seekg` to a magic offset like 12 or 36.
2. **RIFF chunk padding** — odd-sized chunks are padded to even boundary. Skip size: `(chunkSize + 1) & ~1u`
3. **Little-endian assumption** — WAV is LE; code uses `memcpy` into native types. This is correct on x64 Windows. Noted in source for future portability.
4. **No external dependencies** — only `<fstream>`, `<cstdint>`, `<cstring>`, `<string>`, `<stdexcept>`, `<iostream>`, `<iomanip>`, `<sstream>`
5. **Extensibility** — `WavMetadata` carries `dataOffset`/`dataSize` for phase 2 PCM access. `printMetadataTable()` is a standalone function for reuse.

## Coding Conventions

- C++20, enforced via `target_compile_features`
- `#pragma once` for header guards
- Doxygen documentation on all structs, functions, templates, and member variables (`/** */` blocks with `@brief`, `@param`, `@return`, `@throws`, `@note`, `@see`; `///` for struct fields)
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
- Test target: `wav_tests` (links `wav_reader.cpp` + `tests/test_wav_reader.cpp`)
- Test WAV path passed via compile definition `TEST_WAV_PATH`
- Test discovery via `catch_discover_tests()`
- Run tests: `ctest --test-dir build/vs2022 -C Release --output-on-failure`

Test categories:
- `[sanity]` — validates parser output against known `test_file.wav` metadata
- `[error]` — verifies exception handling for invalid inputs
- `[unit]` — tests `WavMetadata::duration()` with synthetic values

## CI/CD

- GitHub Actions workflow: `.github/workflows/build.yml`
- Triggers on every push to `main`
- Runs on `windows-latest` with MSVC via `ilammy/msvc-dev-cmd@v1`
- Uses the `ci` CMake preset (Ninja generator) — the VS2022 generator is not available on GitHub runners
- Steps: configure → build Release → run tests → package artifact
- Artifact `WavTool-Release`: contains source files, tests, and `wav_tool.exe`

## VS Code Configuration

- `.vscode/c_cpp_properties.json` — IntelliSense config: delegates to CMake Tools extension (`ms-vscode.cmake-tools`) for include paths, defines, and compiler settings.
