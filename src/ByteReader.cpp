#include "ByteReader.hpp"
#include "Fatal.hpp"
#include <cstddef>
#include <fcntl.h>
#include <optional>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

std::optional<ByteReader> ByteReader::open(const char *path) {
    int fd = ::open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        return std::nullopt;
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1 || sb.st_size == 0) {
        return std::nullopt;
    }

    void *mapped = mmap(nullptr, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    if (mapped == MAP_FAILED) {
        return std::nullopt;
    }
    return ByteReader(static_cast<const std::byte *>(mapped), sb.st_size);
}

ByteReader::ByteReader(const std::byte *data, size_t size) : data(data), size(size) {}

ByteReader::~ByteReader() {
    if (data) {
        munmap(const_cast<std::byte *>(data), size);
    }
}

std::optional<std::byte> ByteReader::peek() const {
    if (pos >= size) {
        return std::nullopt;
    }
    return data[pos];
}

std::optional<std::byte> ByteReader::at(size_t offset) const {
    if (offset >= size) {
        fatal("out of bounds");
    }
    return data[offset];
}

std::optional<std::byte> ByteReader::consume() {
    if (pos >= size) {
        return std::nullopt;
    }
    return data[pos++];
}

void ByteReader::rewind() { pos = 0; }

void ByteReader::seek(size_t offset) {
    if (offset >= size) {
        fatal("out of bounds");
    }
    pos = offset;
}

bool ByteReader::inBounds(size_t offset) const { return offset < size; }
