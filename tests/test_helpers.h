/**
 * @file test_helpers.h
 * @brief Shared helpers for the integration and e2e test suites: fixture-path
 *        resolution, scratch-directory management, and wav_tool.exe invocation.
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef TEST_FILES_DIR
#error "TEST_FILES_DIR must be defined via CMake"
#endif
#ifndef TEST_SCRATCH_DIR
#error "TEST_SCRATCH_DIR must be defined via CMake"
#endif
#ifndef WAV_TOOL_EXE
#error "WAV_TOOL_EXE must be defined via CMake"
#endif

/// @return Full path to a fixture file under tests/files/, given its filename.
inline std::string fixturePath(const std::string& name) {
    return std::string(TEST_FILES_DIR) + "/" + name;
}

/// Ensures the build-tree scratch directory exists. Safe to call from every TEST_CASE.
inline std::string scratchDir() {
    std::filesystem::create_directories(TEST_SCRATCH_DIR);
    return TEST_SCRATCH_DIR;
}

/// @return Full path to a scratch file under the build-tree scratch directory.
inline std::string scratchPath(const std::string& name) {
    return scratchDir() + "/" + name;
}

/// Deletes the file at `path` when it goes out of scope. Construct with a
/// scratchPath() result to guarantee test-generated output files don't linger.
struct CleanupGuard {
    std::string path;
    ~CleanupGuard() { std::remove(path.c_str()); }
};

/// Captured result of running wav_tool.exe as a subprocess.
struct ProcessResult {
    int exitCode;
    std::string stdOut;
    std::string stdErr;
};

/**
 * @brief Runs wav_tool.exe with the given arguments, capturing stdout/stderr.
 *
 * Uses std::system() with shell redirection to scratch files (no platform-specific
 * process APIs). Every token is individually double-quoted, and the whole command
 * is wrapped in one more pair of quotes, since cmd.exe strips the outermost quote
 * pair when the first token of the command is itself quoted; this also neutralizes
 * '&' characters that may appear in the repo's own path.
 */
inline ProcessResult runWavTool(const std::vector<std::string>& args) {
    static int counter = 0;
    std::string tag = std::to_string(counter++);
    CleanupGuard outFile{scratchPath("e2e_stdout_" + tag + ".txt")};
    CleanupGuard errFile{scratchPath("e2e_stderr_" + tag + ".txt")};

    std::ostringstream inner;
    inner << "\"" << WAV_TOOL_EXE << "\"";
    for (const auto& arg : args) {
        inner << " \"" << arg << "\"";
    }
    inner << " > \"" << outFile.path << "\" 2> \"" << errFile.path << "\"";

    std::string command = "\"" + inner.str() + "\"";
    int exitCode = std::system(command.c_str());

    auto slurp = [](const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };

    return {exitCode, slurp(outFile.path), slurp(errFile.path)};
}
