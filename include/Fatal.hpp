// Fatal.h
#ifndef FATAL_H
#define FATAL_H

#include <cstdio>
#include <cstdlib>

// prints msg to stderr and aborts; unlike assert(), never compiled out by NDEBUG. Use for
// data-validation failures that must always be fatal (e.g. malformed level files), as opposed
// to assert()'s programmer-invariant checks.
[[noreturn]] inline void fatal(const char *msg) {
    fprintf(stderr, "fatal: %s\n", msg);
    abort();
}

#endif // FATAL_H
