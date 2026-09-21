// ByteReader.h
#ifndef BYTEREADER_H
#define BYTEREADER_H

#include <cstddef>
#include <cstdio>
#include <optional>

class ByteReader {
  public:
    // Throws FileError if the file cannot be opened.
    static ByteReader open(const char *path);

    ~ByteReader() {
        if (stream) {
            fclose(stream);
        }
    }

    ByteReader(const ByteReader &) = delete;
    ByteReader &operator=(const ByteReader &) = delete;
    ByteReader(ByteReader &&other) noexcept : stream(other.stream), offset(other.offset) {
        other.stream = nullptr;
    }

    std::optional<std::byte> peek() const;
    std::optional<std::byte> consume();

    // Number of bytes consumed so far; useful for error context.
    std::size_t position() const { return offset; }

  private:
    explicit ByteReader(FILE *stream) : stream(stream) {}
    FILE *stream;
    std::size_t offset = 0;
};

#endif // BYTEREADER_H
