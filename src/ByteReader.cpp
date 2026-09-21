#include "ByteReader.hpp"
#include "Errors.hpp"
#include <cerrno>
#include <cstring>
#include <optional>

ByteReader ByteReader::open(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        throw FileError(path, std::strerror(errno));
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
    ++offset;
    return static_cast<std::byte>(c);
}
