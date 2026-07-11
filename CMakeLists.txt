# Generate the VS solution: cmake --preset windows-vs2022
cmake_minimum_required(VERSION 3.28)
project(WavTool LANGUAGES CXX)

add_executable(wav_tool
    main.cpp
    src/wav_reader.cpp
    src/wav_writer.cpp
    src/sample_codec.cpp
    src/resampler.cpp
    src/channel_mixer.cpp
    src/wav_converter.cpp
)

target_compile_features(wav_tool PUBLIC cxx_std_20)
target_include_directories(wav_tool PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(MSVC)
    target_compile_options(wav_tool PRIVATE /W4 /WX)
else()
    target_compile_options(wav_tool PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# --- Tests ---
include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

enable_testing()

add_executable(wav_tests
    tests/test_wav_reader.cpp
    tests/test_wav_writer.cpp
    tests/test_sample_codec.cpp
    tests/test_resampler.cpp
    tests/test_channel_mixer.cpp
    tests/test_wav_converter.cpp
    tests/test_integration.cpp
    tests/test_e2e.cpp
    src/wav_reader.cpp
    src/wav_writer.cpp
    src/sample_codec.cpp
    src/resampler.cpp
    src/channel_mixer.cpp
    src/wav_converter.cpp
)

target_compile_features(wav_tests PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(wav_tests PRIVATE /W4 /WX)
else()
    target_compile_options(wav_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
target_include_directories(wav_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(wav_tests PRIVATE Catch2::Catch2WithMain)
target_compile_definitions(wav_tests PRIVATE TEST_WAV_PATH="${CMAKE_SOURCE_DIR}/tests/files/reference-sample-44100hz-16bit-2ch.wav")
target_compile_definitions(wav_tests PRIVATE TEST_FILES_DIR="${CMAKE_SOURCE_DIR}/tests/files")
target_compile_definitions(wav_tests PRIVATE WAV_TOOL_EXE="$<TARGET_FILE:wav_tool>")
target_compile_definitions(wav_tests PRIVATE TEST_SCRATCH_DIR="${CMAKE_BINARY_DIR}/tests_scratch")

add_dependencies(wav_tests wav_tool)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# One catch_discover_tests() call per source file, each carrying a single
# file-level CTest label (`ctest -L reader`, etc). See CLAUDE.md's Testing
# section for why there's no combined "unit" label across the six files below.
catch_discover_tests(wav_tests
    TEST_SPEC "[reader]"
    TEST_PREFIX "[unit][reader]"
    PROPERTIES LABELS "reader"
)
catch_discover_tests(wav_tests
    TEST_SPEC "[writer]"
    TEST_PREFIX "[unit][writer]"
    PROPERTIES LABELS "writer"
)
catch_discover_tests(wav_tests
    TEST_SPEC "[codec]"
    TEST_PREFIX "[unit][codec]"
    PROPERTIES LABELS "codec"
)
catch_discover_tests(wav_tests
    TEST_SPEC "[resampler]"
    TEST_PREFIX "[unit][resampler]"
    PROPERTIES LABELS "resampler"
)
catch_discover_tests(wav_tests
    TEST_SPEC "[mixer]"
    TEST_PREFIX "[unit][mixer]"
    PROPERTIES LABELS "mixer"
)
catch_discover_tests(wav_tests
    TEST_SPEC "[converter]"
    TEST_PREFIX "[unit][converter]"
    PROPERTIES LABELS "converter"
)
catch_discover_tests(wav_tests
    TEST_SPEC "[integration]"
    TEST_PREFIX "[integration]"
    PROPERTIES LABELS "integration"
)
catch_discover_tests(wav_tests
    TEST_SPEC "[e2e]"
    TEST_PREFIX "[e2e]"
    PROPERTIES LABELS "e2e"
)
