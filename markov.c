#include "eprintf.h"
#include "mmapped_file.h"
#include "perrno.h"
#include "word_tbl.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

#ifdef __linux__
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#define SKIPPED_CHARS ".,\"'?!()*@#:\n"

static bool skipchar(uint8_t c) {
	for (size_t i = 0; i < sizeof(SKIPPED_CHARS); i++) {
		if (c == SKIPPED_CHARS[i]) {
			return true;
		}
	}
	return false;
}

static uint64_t random_u64(int urandom_fd) {
	uint8_t buf[sizeof(uint64_t) / sizeof(uint8_t)] = {0};
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

#define RANDOM_PUNC ".!?,"

static char random_punc(int urandom_fd) {
	char b = 0;
	if (read(urandom_fd, &b, 1) != 1) {
		eprintf("random_punc: can't read /dev/urandom\n");
		abort();
	}
	static_assert(sizeof(RANDOM_PUNC) <= CHAR_MAX, "sizeof(RANDOM_PUNC) > CHAR_MAX");
	return RANDOM_PUNC[b % (char)(sizeof(RANDOM_PUNC) - 1)];
}

static void ingest(struct mmapped_file *mf, struct word_tbl *tbl) {
	size_t start = 0;
	size_t end = 0;
	word_tbl_hash prev_ent_hash = 0;
	while (end < mf->file_len) {
		if (mfi(mf, end) != ' ') {
			end += 1;
			continue;
		}
		size_t ent_start = start;
		size_t ent_end = end;
		while (((ent_end - ent_start) > 0) && skipchar(mfi(mf, ent_end - 1))) {
			ent_end -= 1;
		}
		while ((ent_start - ent_end > 1) && skipchar(mfi(mf, ent_start))) {
			ent_start += 1;
		}
		struct word_tbl_ent *ent = word_tbl_by_string(tbl, mfs(mf, ent_start, ent_end), ent_end - ent_start);
		ent->start = ent_start;
		ent->end = ent_end;
		if (prev_ent_hash != 0) {
			struct word_tbl_ent *prev_ent = word_tbl_by_hash(tbl, prev_ent_hash);
			word_tbl_ent_push(prev_ent, ent->hash);
		}
		prev_ent_hash = ent->hash;
		start = end = (end + 1);
	}
}

static inline void fwrite_ent(struct mmapped_file *mf, struct word_tbl_ent *ent) {
	fwrite(mfs(mf, ent->start, ent->end), 1, ent->end - ent->start, stdout);
}

static void vomit(struct mmapped_file *mf, struct word_tbl *tbl, int urandom_fd) {
	struct word_tbl_ent *prev_ent = NULL;
	for (;;) {
		uint64_t idx = random_u64(urandom_fd) % word_tbl_cap_of(tbl);
		prev_ent = word_tbl_by_index(tbl, idx);
		if (prev_ent->occupied) {
			break;
		}
	}
	fwrite_ent(mf, prev_ent);
	for (size_t i = 0; i < 32; i++) {
		if (prev_ent->stackpos == 0) {
			break;
		}
		struct word_tbl_ent *next_ent = NULL;
		for (size_t j = 0; j < prev_ent->stackpos; j++) {
			uint64_t idx = random_u64(urandom_fd) % prev_ent->stackpos;
			word_tbl_hash hash = prev_ent->nextwords[idx];
			next_ent = word_tbl_by_hash(tbl, hash);
		}
		if (next_ent == NULL) {
			break;
		}
		fputc(' ', stdout);
		fwrite_ent(mf, next_ent);
		prev_ent = next_ent;
	}
	printf("%c\n", random_punc(urandom_fd));
}

int main(int argc, char *argv[]) {
	const char *path = "/Users/lina/Developer/markov-c/corpus3";
	if (argc > 1) {
		path = argv[1];
	}
	int our_fd = -1;
	if (argc > 2) {
		our_fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC);
		if (our_fd < 0) {
			perror(NULL);
			abort();
		}
	}

	struct mmapped_file mf = {0};
	int retval = -1;
	if ((retval = mmapped_file_open(&mf, path, O_RDONLY)) < 0) {
		perrno(retval);
		return 1;
	}

	uint64_t words = 0;
	for (size_t i = 0; i < mf.file_len; i++) {
		words += mfi(&mf, i) == ' ';
	}

	struct word_tbl tbl = {0};
	if ((retval = word_tbl_init(&tbl, words / 6, our_fd)) < 0) {
		perrno(retval);
		return 1;
	}

	ingest(&mf, &tbl);

	int urandom_fd = open("/dev/urandom", O_RDONLY);
	if (urandom_fd == -1) {
		eprintf("can't open /dev/urandom?\n");
		abort();
	}

	do {
		vomit(&mf, &tbl, urandom_fd);
	} while (getc(stdin) != EOF);
	
	// word_tbl_destroy(&tbl);

	puts("");
	return 0;
}
