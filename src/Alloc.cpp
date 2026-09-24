#include <cstddef>
#include <cstdlib>

// replace pretty printed throws on libstdc++ defaults like std::bad_alloc with aborts, as we
// compile with -fno-exceptions and never throw; avoids pulling in the whole exception path and
// system demangler and significantly drops binary size

void *operator new(std::size_t size) {
    void *p = std::malloc(size ? size : 1);
    if (!p) {
        std::abort();
    }
    return p;
}

void *operator new[](std::size_t size) { return operator new(size); }

void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

extern "C" char *__cxa_demangle(const char *, char *, std::size_t *, int *status) {
    if (status) {
        *status = -1;
    }
    return nullptr;
}
