#pragma once

#include <cstdio>
#include <cstdlib>

// prints msg to stderr and aborts; not removed out by NDEBUG. Use for validation failures that must
// always be fatal (e.g. malformed level files)
[[noreturn]] inline void fatal(const char *msg) {
    fprintf(stderr, "fatal: %s\n", msg);
    abort();
}
