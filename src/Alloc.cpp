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

namespace std {

[[noreturn]] void __throw_length_error(const char *) { std::abort(); }
[[noreturn]] void __throw_out_of_range(const char *) { std::abort(); }
[[noreturn]] void __throw_out_of_range_fmt(const char *, ...) { std::abort(); }
[[noreturn]] void __throw_logic_error(const char *) { std::abort(); }
[[noreturn]] void __throw_invalid_argument(const char *) { std::abort(); }
[[noreturn]] void __throw_domain_error(const char *) { std::abort(); }
[[noreturn]] void __throw_range_error(const char *) { std::abort(); }
[[noreturn]] void __throw_overflow_error(const char *) { std::abort(); }
[[noreturn]] void __throw_underflow_error(const char *) { std::abort(); }
[[noreturn]] void __throw_bad_alloc() { std::abort(); }
[[noreturn]] void __throw_bad_cast() { std::abort(); }
[[noreturn]] void __throw_bad_typeid() { std::abort(); }
[[noreturn]] void __throw_bad_exception() { std::abort(); }

} // namespace std
