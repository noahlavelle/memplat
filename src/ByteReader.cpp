#include "ByteReader.hpp"
#include <cstdio>
#include <optional>

std::optional<ByteReader> ByteReader::open(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return std::nullopt;
    }
    return ByteReader(f);
}

std::optional<std::byte> ByteReader::peek() const {
    int c = fgetc(stream);
    if (c == EOF) {
        return std::nullopt;
    }
    ungetc(c, stream);
    return static_cast<std::byte>(c);
}
std::optional<std::byte> ByteReader::consume() {
    int c = fgetc(stream);
    if (c == EOF) {
        return std::nullopt;
    }
    return static_cast<std::byte>(c);
}

void ByteReader::rewind() { std::rewind(stream); }
