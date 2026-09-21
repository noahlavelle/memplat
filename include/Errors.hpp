// Errors.h
#ifndef ERRORS_H
#define ERRORS_H

#include <cstddef>
#include <stdexcept>
#include <string>

// Thrown when a file cannot be opened; carries the path and the OS-reported reason.
class FileError : public std::runtime_error {
  public:
    FileError(const std::string &path, const std::string &reason)
        : std::runtime_error("failed to open '" + path + "': " + reason), path(path) {}

    std::string path;
};

// Thrown when level data is malformed or truncated; carries the byte offset it was found at.
class LevelParseError : public std::runtime_error {
  public:
    LevelParseError(const std::string &message, std::size_t offset)
        : std::runtime_error(message + " (at byte offset " + std::to_string(offset) + ")"),
          offset(offset) {}

    std::size_t offset;
};

#endif // ERRORS_H
