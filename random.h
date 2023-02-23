#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

uint64_t random_u64(int urandom_fd);
char random_punc(int urandom_fd);

#endif
