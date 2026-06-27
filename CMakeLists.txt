# Generate the VS solution: cmake --preset windows-vs2022
cmake_minimum_required(VERSION 3.28)
project(WavTool LANGUAGES CXX)

add_executable(wav_tool
    main.cpp
    wav_reader.cpp
    wav_writer.cpp
)

target_compile_features(wav_tool PUBLIC cxx_std_20)

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
    wav_reader.cpp
    wav_writer.cpp
)

target_compile_features(wav_tests PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(wav_tests PRIVATE /W4 /WX)
else()
    target_compile_options(wav_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
target_include_directories(wav_tests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(wav_tests PRIVATE Catch2::Catch2WithMain)
target_compile_definitions(wav_tests PRIVATE TEST_WAV_PATH="${CMAKE_SOURCE_DIR}/tests/test_file.wav")

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(wav_tests)
