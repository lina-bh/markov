#include "random.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

#include "eprintf.h"

#define RANDOM_PUNC ".!?,"

uint64_t random_u64(int urandom_fd) {
	uint8_t buf[sizeof(uint64_t) / sizeof(uint8_t)] = { 0 };
	if (read(urandom_fd, buf, sizeof(buf)) != sizeof(buf)) {
		eprintf("random_u64: can't read /dev/urandom\n");
		abort();
	}
	uint64_t retval = buf[0];
	for (unsigned int i = 1; i < sizeof(buf); i++) {
		retval += ((uint64_t)buf[i] << 8 * i);
	}
	return retval;
}

char random_punc(int urandom_fd) {
	char b = 0;
	if (read(urandom_fd, &b, 1) != 1) {
		eprintf("random_punc: can't read /dev/urandom\n");
		abort();
	}
	static_assert(sizeof(RANDOM_PUNC) <= CHAR_MAX, "sizeof(RANDOM_PUNC) > CHAR_MAX");
	return RANDOM_PUNC[b % (char)(sizeof(RANDOM_PUNC) - 1)];
}
