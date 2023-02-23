#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __linux__
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#include "eprintf.h"
#include "ingest.h"
#include "mmapped_file.h"
#include "perrno.h"
#include "vomit.h"
#include "word_tbl.h"

int main(int argc, char *argv[]) {
	const char *path = "/Users/lina/Developer/markov-c/corpus3";
	if (argc > 1) {
		path = argv[1];
	}
	int our_fd = -1;
	if (argc > 2) {
		our_fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666);
		if (our_fd < 0) {
			perror(NULL);
			abort();
		}
	}

	struct mmapped_file mf = { 0 };
	int retval = -1;
	if ((retval = mmapped_file_open(&mf, path, O_RDONLY)) < 0) {
		perrno(retval);
		return 1;
	}

	uint64_t words = 0;
	for (size_t i = 0; i < mf.file_len; i++) {
		words += mfi(&mf, i) == ' ';
	}

	struct word_tbl tbl = { 0 };
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
