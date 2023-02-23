#ifndef MMAPPED_FILE_H
#define MMAPPED_FILE_H

#include "eprintf.h"

#include <inttypes.h>
#include <stdlib.h>

struct mmapped_file {
	uint8_t *map;
	size_t mmap_len;
	uint64_t file_len;
};

static inline uint8_t mmapped_file_index(struct mmapped_file *mf, size_t idx) {
	if (idx >= mf->file_len) {
		eprintf("mmapped_file_index: invalid index %zu >= file_len %" PRIu64 "d\n", idx, mf->file_len);
		abort();
	}
	return (mf->map)[idx];
}

#define mfi mmapped_file_index

static inline uint8_t *mmapped_file_substring(struct mmapped_file *mf, size_t start, size_t end) {
	if (mf->map + end >= mf->map + mf->file_len) {
		eprintf("mmapped_file_substring: invalid end index %zu, file_len %" PRIu64 "d\n", end, mf->file_len);
		abort();
	}
	return &mf->map[start];
}

#define mfs mmapped_file_substring

int mmapped_file_open(struct mmapped_file *mf, const char *path, int oflags);

#endif // MMAPPED_FILE_H
