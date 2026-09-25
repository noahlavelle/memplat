#pragma once

#include <cstddef>
#include <optional>

class ByteReader {
  public:
    static std::optional<ByteReader> open(const char *path);

    ~ByteReader();

    ByteReader(const ByteReader &) = delete;
    ByteReader &operator=(const ByteReader &) = delete;
    ByteReader(ByteReader &&other) : data(other.data), size(other.size), pos(other.pos) {
        other.data = nullptr;
        other.size = 0;
    }

    std::optional<std::byte> peek() const;
    std::optional<std::byte> at(size_t offset) const;
    std::optional<std::byte> consume();
    void rewind();
    void seek(size_t p);
    bool inBounds(size_t offset) const;

  private:
    explicit ByteReader(const std::byte *data, size_t size);

    const std::byte *data;
    size_t size;
    size_t pos = 0;
};
