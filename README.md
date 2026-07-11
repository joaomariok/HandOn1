# WavTool

A C++20 command-line utility that reads WAV audio files, displays their metadata, copies WAV files preserving all original properties, and converts sample rate / channel count / bit depth (all three phases fully implemented).

## Prerequisites

- **Visual Studio 2022** (Community, Professional, or Enterprise)
- **CMake 3.28+** (bundled with VS2022, or install separately)
- **Windows 10/11** x64

## Build Instructions

```bash
# Configure
cmake --preset windows-vs2022

# Build
cmake --build build/vs2022 --config Debug    # or --config Release
```

If `cmake` is not on your PATH, use the full path from your VS2022 installation:
```
"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
```

Additional build flags (parallel builds, clean rebuilds, specific targets, CI preset) in [CLAUDE.md](CLAUDE.md#build-instructions).

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

`wav_tool.exe input.wav output.wav` (no flags) prints the input table once, then `Output written: output.wav (5289194 bytes)` — an exact copy, so there's no output table or property diff.

## Project Structure

```
HandOn1/
├── .github/
│   └── workflows/
│       └── build.yml
├── .vscode/
├── CMakeLists.txt
├── CMakePresets.json
├── wav_types.h
├── wav_reader.h / wav_reader.cpp
├── wav_writer.h / wav_writer.cpp
├── sample_codec.h / sample_codec.cpp
├── resampler.h / resampler.cpp
├── channel_mixer.h / channel_mixer.cpp
├── wav_converter.h / wav_converter.cpp
├── main.cpp
├── tests/
│   ├── files/              # WAV fixtures, named <rate>hz-<bits>bit-<channels>ch[-<codec>][-N].wav
│   ├── test_helpers.h
│   └── test_*.cpp          # one file per module, plus test_integration.cpp and test_e2e.cpp
├── CLAUDE.md
└── README.md
```

What each module does is documented in [CLAUDE.md](CLAUDE.md#architecture).

## Running Tests

Tests use **Catch2** (v3.7.1, auto-fetched via CMake FetchContent — no manual install needed) and are built alongside the main target.

```bash
cmake --build build/vs2022 --config Release
ctest --test-dir build/vs2022 -C Release --output-on-failure
```

To run a subset, filter by label: `ctest --test-dir build/vs2022 -C Release -L reader --output-on-failure` (labels: `reader`, `writer`, `codec`, `resampler`, `mixer`, `converter`, `integration`, `e2e`). Label/tag mechanics and the fixture-naming scheme are documented in [CLAUDE.md](CLAUDE.md#testing).

## CI/CD

Every push to `main` builds, tests, and packages a `WavTool-Release` artifact via GitHub Actions, available in the **Actions** tab. Workflow/runner/preset details in [CLAUDE.md](CLAUDE.md#cicd).

## Error Handling

The tool reports errors to stderr and exits with code 1 for: missing/incorrect CLI
arguments, a file that isn't a valid RIFF/WAVE container or is missing a required
chunk, unexpected EOF while parsing, invalid or incomplete `--rate`/`--channels`/`--bits`
flags, and unsupported source formats for conversion (not integer PCM at 8/16/24/32-bit,
not 32-bit float PCM, or a channel count other than mono/stereo).
