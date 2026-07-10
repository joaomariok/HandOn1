# WavTool — Claude Code Project Context

## IMPORTANT: Update this file after every implementation change.
## IMPORTANT: Update README.md after every implementation change to keep it consistent with this file.
## IMPORTANT: After every implementation, consider adding or updating tests to cover the new or changed functionality.

## Project Overview

WavTool is a C++20 console application that parses WAV/RIFF audio files, displays metadata, copies WAV files preserving all properties, and converts sample rate / channel count / bit depth (phase 3, final, of a 3-phase project — see README.md for the phase roadmap and CLI usage). No external dependencies, no audio libraries — only C++ standard library headers.

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

Run: `build\vs2022\Release\wav_tool.exe <input.wav> [output.wav] [--rate N] [--channels N] [--bits N]`

CI uses a second preset, `ci` (Ninja generator, no VS install needed) — see CMake Structure below.

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
- **Algorithm**: writes RIFF header from `meta.riff`, fmt chunk field-by-field (including any extension bytes), data chunk via `writeRawChunk`, then extra chunks via `writeRawChunk` (unknown chunks like LIST/id3 are re-emitted to preserve output file size parity with the source). No input file re-reading needed.
- **Errors**: throws `std::runtime_error` for: cannot open output, write errors

### `sample_codec.h` / `sample_codec.cpp` — PCM<->float sample decoding/encoding
- **Public API**: `decodeSamples(pcmBytes, audioFormat, bitsPerSample, numChannels) -> vector<vector<float>>` (one inner vector per channel, normalized to [-1.0, 1.0]); `encodeSamples(channelSamples, targetBits) -> vector<uint8_t>` (always integer PCM output)
- Supports decode of 8/16/24/32-bit integer PCM (`audioFormat==1`) and 32-bit float PCM (`audioFormat==3`, direct passthrough, no scaling); encode is always integer PCM (8-bit unsigned, 16/24/32-bit signed LE)
- 16/24/32-bit signed decode/encode share one generic byte-count-parameterized helper; only 8-bit (unsigned) is a special case
- **Errors**: throws `std::runtime_error` for unsupported (audioFormat, bitsPerSample) combinations or unsupported target bit depths
- Top-of-file comment states the simplicity (no dithering/noise shaping) is deliberate

### `resampler.h` / `resampler.cpp` — Linear-interpolation sample rate conversion
- **Public API**: `resample(channelSamples, srcRate, dstRate) -> vector<vector<float>>`
- Linear interpolation only, per-channel independent; identity when `srcRate == dstRate`
- **Note**: aliases on downsampling (no anti-alias filter) — a deliberate, documented tradeoff for this didactic exercise, not a production bandlimited resampler

### `channel_mixer.h` / `channel_mixer.cpp` — Mono<->stereo channel conversion
- **Public API**: `convertChannels(channelSamples, targetChannels) -> vector<vector<float>>`
- Stereo→mono averages channels; mono→stereo duplicates; only 1↔2 supported
- **Errors**: throws `std::runtime_error` for unsupported channel counts (source or target)

### `wav_converter.h` / `wav_converter.cpp` — Phase 3 pipeline orchestration
- **Public API**: `ConversionOptions` struct (`hasRate`/`rate`, `hasChannels`/`channels`, `hasBits`/`bits`, `any()`); `convertWav(input, opts) -> WavMetadata` — pure in-memory, no I/O
- **Algorithm**: validates source format (integer PCM 8/16/24/32-bit, or 32-bit float PCM; mono/stereo only) → resolves each target property (flag value or source fallback) → always rebuilds a fresh standard 16-byte PCM `FmtChunk` → runs decode → (channel conversion if needed) → (rate conversion if needed) → encode, skipped entirely if source is already integer PCM and every resolved target equals the source → recomputes `data` header and `riff.chunkSize`
- Mirrors the `wav_reader`/`wav_writer` split: keeps the graded conversion logic (duration preservation, exact property matching) testable without file I/O or `main()`
- **Errors**: throws `std::runtime_error` for unsupported source format/bit depth/channel count

### `main.cpp` — CLI entry point + display + flag parsing
- `void printMetadataTable(const WavMetadata&)` — renders UTF-8 box-drawing table to stdout (accesses fields via `meta.fmt.*`)
- `ConversionOptions parseConversionFlags(int argc, char* argv[])` — parses `--rate`/`--channels`/`--bits` pairs starting at `argv[3]`, validates each value, throws `std::runtime_error` with a specific message on any error (unknown flag, missing value, invalid value). Kept inline in `main.cpp` (not its own module) since CLI-boundary parsing has never been unit-tested in this project and `wav_tests` doesn't link `main.cpp`
- `int main(int argc, char* argv[])` — validates `argc >= 2`, calls `readWavFile()`, prints input table:
  - `argc == 2`: phase 1, read-only, unchanged
  - `argc == 3`: phase 2, `writeWavFile()` exact copy, unchanged
  - `argc > 3`: phase 3, `parseConversionFlags()` → `convertWav()` → `writeWavFile()`, prints a second labeled "Output:" table, then a confirmation line with size/rate/bits/channels

## Key Design Rules

1. **NO hardcoded byte offsets** — the parser walks the RIFF chunk list dynamically. Never use `seekg` to a magic offset like 12 or 36.
2. **RIFF chunk padding** — odd-sized chunks are padded to even boundary. Skip size: `(chunkSize + 1) & ~1u`
3. **Little-endian assumption** — WAV is LE; code uses `memcpy` into native types. This is correct on x64 Windows. Noted in source for future portability.
4. **No external dependencies** — only `<fstream>`, `<cstdint>`, `<cstring>`, `<string>`, `<stdexcept>`, `<iostream>`, `<iomanip>`, `<sstream>`, `<vector>`, `<filesystem>`
5. **Extensibility** — Each RIFF envelope has its own struct with `totalSize()`. `WavMetadata` aggregates all envelopes and provides `fileSize()`, `duration()`. `printMetadataTable()` is a standalone function for reuse.
6. **Read once** — The reader loads all data into memory (PCM samples, extra chunks). The writer writes from in-memory structs without re-reading the input file.
7. **No fancy DSP** — `sample_codec`, `resampler`, and `channel_mixer` deliberately use the simplest correct approach (no windowed sinc, no anti-aliasing filter, no dithering/noise shaping). This is a didactic exercise, not a production audio pipeline; each file states this in a top-of-file comment.
8. **Output is always integer PCM** — `convertWav()` always encodes to integer PCM (per `--bits`'s valid values: 8/16/24/32), even when the source is 32-bit float PCM. `writeWavFile()` doesn't need to know or care about this: it just serializes whatever bytes sit in `WavMetadata.data.data`, integer or float alike.

## Coding Conventions

- C++20, enforced via `target_compile_features`
- `#pragma once` for header guards
- Doxygen documentation on all structs, functions, templates, and member variables (`/** */` blocks with `@brief`, `@param`, `@return`, `@throws`, `@note`, `@see`; `///` for struct fields)
- Prefer explicit types over `auto` unless the type is a lambda or excessively complex
- Inline comments only when the WHY is non-obvious
- Minimal error handling — only at system boundaries (file I/O, CLI args)
- Errors to stderr, non-zero exit code on failure
- All warnings treated as errors: `/W4 /WX` (MSVC) or `-Wall -Wextra -Wpedantic -Werror` (else), applied to both `wav_tool` and `wav_tests`

## CMake Structure

- Project name: `WavTool`
- Target: `wav_tool`
- Presets defined in `CMakePresets.json`:
  - Configure: `windows-vs2022` (VS 17 2022, x64, build dir: `build/vs2022`) — local dev
  - Configure: `ci` (Ninja generator, `CMAKE_BUILD_TYPE=Release` baked in, build dir: `build/ci`) — GitHub Actions only, no VS install needed
  - Build: `windows-vs2022-debug`, `windows-vs2022-release`, `ci-release`
- `wav_tests` depends on `wav_tool` via `add_dependencies` (needed for the `[e2e]` tests, which spawn the built executable)

## Testing

- Framework: **Catch2 v3.7.1** (auto-fetched via CMake FetchContent)
- Test target: `wav_tests` (links `wav_reader.cpp`, `wav_writer.cpp`, `sample_codec.cpp`, `resampler.cpp`, `channel_mixer.cpp`, `wav_converter.cpp` + their `tests/test_*.cpp` files, plus `tests/test_integration.cpp` and `tests/test_e2e.cpp`; `tests/test_helpers.h` is a header-only shared helper, not compiled separately)
- `wav_tests` has an explicit `add_dependencies(wav_tests wav_tool)` so `wav_tool.exe` is always built before the e2e tests that spawn it
- Test WAV path passed via compile definition `TEST_WAV_PATH`; `tests/test_integration.cpp`/`tests/test_e2e.cpp` additionally use `TEST_FILES_DIR` (path to `tests/files/`), `WAV_TOOL_EXE` (path to the built `wav_tool.exe`, via `$<TARGET_FILE:wav_tool>`), and `TEST_SCRATCH_DIR` (build-tree scratch directory for generated test output, never committed)
- Test fixture WAV files live in `tests/files/`, named `<rate>hz-<bits>bit-<channels>ch[-<codec>][-N].wav` (pure fmt-chunk info; malformed fixtures used for error-path testing are named `<descriptive-name>-malformed.wav` instead, since no fmt is readable; `reference-sample-44100hz-16bit-2ch.wav` is the original CI-wired fixture)
- Test tags follow a clean two-axis scheme: a **type** tag (`[unit]`, `[integration]`, or `[e2e]`) and, for the six unit-suite files only, a **module** tag (`[reader]`, `[writer]`, `[codec]`, `[resampler]`, `[mixer]`, `[converter]`) — e.g. `test_wav_reader.cpp`'s `TEST_CASE`s are all tagged `"[reader][unit]"`. `test_integration.cpp`/`test_e2e.cpp` only need their type tag (`[integration]`/`[e2e]`) since there's one file each, so type and module coincide
- Test discovery via one `catch_discover_tests()` call **per source file**, each filtered by that file's Catch2 tag via `TEST_SPEC`, with a `TEST_PREFIX` (`"[unit][reader]."`, ..., `"[integration]."`, `"[e2e]."`) and a single-value CTest `LABELS` matching the module/file name (`reader`, `writer`, `codec`, `resampler`, `mixer`, `converter`, `integration`, `e2e`)
- **Gotcha — no combined `unit` CTest label:** `catch_discover_tests()`'s `PROPERTIES` only reliably forwards **single-value** labels; a multi-value list like `LABELS "unit;reader"` gets flattened into broken tokens by its internal `-D`/child-script property-forwarding pipeline (confirmed via the generated `_tests.cmake`, which wrote `LABELS unit reader` unquoted — `set_tests_properties` then misparses that as extra dangling property names). So the six unit-suite files each keep only their own module label; filter that superset via label-exclude instead: `ctest -LE "integration|e2e"`. The `[unit]` **Catch2 tag** itself is still applied to all of them and works for direct-binary filtering (`wav_tests.exe "[unit]"`), since that's a Catch2-level tag match, not a CTest `PROPERTIES` value
- Run tests: `ctest --test-dir build/vs2022 -C Release --output-on-failure`
- Run one file's tests: `ctest --test-dir build/vs2022 -C Release -L reader --output-on-failure` (or `-L writer`, `-L codec`, `-L resampler`, `-L mixer`, `-L converter`, `-L integration`, `-L e2e`)
- Run the whole unit-test superset: `ctest --test-dir build/vs2022 -C Release -LE "integration|e2e" --output-on-failure`
- Useful ctest flags: `--output-on-failure`, `-R <regex>` (filter tests), `-L <label>` (filter by one file's label), `-LE <regex>` (exclude by label regex), `--print-labels` (list labels), `-V` (verbose)

Test files (56 tests total):
- `test_wav_reader.cpp` (5, `[reader][unit]`) — parses the reference fixture, validates its metadata, exception handling for missing files, `WavMetadata::duration()` with synthetic values
- `test_wav_writer.cpp` (3, `[writer][unit]`) — file size, PCM data integrity, re-parsability, invalid output path
- `test_sample_codec.cpp` (9, `[codec][unit]`) — PCM<->float decode/encode at each bit depth, including 32-bit float passthrough, unsupported format/bit-depth errors
- `test_resampler.cpp` (4, `[resampler][unit]`) — linear-interpolation sample rate conversion (identity, upsample, downsample, per-channel independence)
- `test_channel_mixer.cpp` (4, `[mixer][unit]`) — mono<->stereo channel conversion, unsupported channel count errors
- `test_wav_converter.cpp` (9, `[converter][unit]`) — phase 3 pipeline orchestration (`convertWav()`), including fmt/RIFF regeneration and error cases
- `test_integration.cpp` (9, `[integration]`) — in-process multi-module pipelines (read→convert→write→re-read) driven by Catch2 `GENERATE` across the `tests/files/` fixture set; covers all 3 phases at the library-API level
- `test_e2e.cpp` (13, `[e2e]`) — spawns the built `wav_tool.exe` via `std::system()` (see `tests/test_helpers.h`'s `runWavTool()`), asserts on captured stdout/stderr/exit code and any output file written; covers CLI argv parsing and process-boundary error wiring not exercised at the unit level (e.g. `parseConversionFlags` in `main.cpp`, which is intentionally not unit-tested)

## CI/CD

- GitHub Actions workflow: `.github/workflows/build.yml`
- Triggers on every push to `main`
- Runs on `windows-latest` with MSVC set up via `egor-tensin/vs-shell@v2`
- Uses the `ci` CMake preset (Ninja generator) — the VS2022 generator is not available on GitHub runners
- Steps: configure → build Release → run tests → package artifact
- Artifact `WavTool-Release`: contains source files, tests, and `wav_tool.exe`

## VS Code Configuration

- `.vscode/c_cpp_properties.json` — IntelliSense config: delegates to CMake Tools extension (`ms-vscode.cmake-tools`) for include paths, defines, and compiler settings.
- `.vscode/tasks.json` — 3 build tasks: "CMake: Configure" (`windows-vs2022` preset), "CMake: Build Debug" (default build task), "CMake: Build Release"; both build tasks depend on Configure.
- `.vscode/launch.json` — 8 `cppvsdbg` debug configurations: run `wav_tool.exe` (Debug/Release) against the reference fixture, against a picklist of every fixture in `tests/files/`, against a typed-in custom path, or against the active editor file; run `wav_tests.exe` (Debug/Release); attach to a running process.
