#ifndef ROUND_UP_H
#define ROUND_UP_H

static inline size_t round_up_sz(size_t x, size_t mul) {
	return ((x + mul - 1) / mul) * mul;
}

#endif // ROUND_UP_H
