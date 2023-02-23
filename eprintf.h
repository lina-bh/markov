#ifndef EPRINT_H
#define EPRINT_H

#include <stdio.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#if 0
#define debug(fn, fmt, ...) fprintf(stderr, fn ": " fmt, __VA_ARGS__)
#else
#define debug(...) ;
#endif

#endif // EPRINT_H
