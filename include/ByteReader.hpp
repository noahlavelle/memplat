// ByteReader.h
#ifndef BYTEREADER_H
#define BYTEREADER_H

#include <cstddef>
#include <cstdio>
#include <optional>

class ByteReader {
  public:
    // returns std::nullopt if the file cannot be opened
    static std::optional<ByteReader> open(const char *path);

    ~ByteReader() {
        if (stream) {
            fclose(stream);
        }
    }

    ByteReader(const ByteReader &) = delete;
    ByteReader &operator=(const ByteReader &) = delete;
    ByteReader(ByteReader &&other) : stream(other.stream) { other.stream = nullptr; }

    std::optional<std::byte> peek() const;
    std::optional<std::byte> consume();

  private:
    explicit ByteReader(FILE *stream) : stream(stream) {}
    FILE *stream;
};

#endif // BYTEREADER_H
