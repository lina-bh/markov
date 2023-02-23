#include "vomit.h"

#include <stdint.h>
#include <stdio.h>

#include "mmapped_file.h"
#include "random.h"
#include "word_tbl.h"

static inline void fwrite_ent(struct mmapped_file *mf, struct word_tbl_ent *ent) {
	fwrite(mfs(mf, ent->start, ent->end), 1, ent->end - ent->start, stdout);
}

void vomit(struct mmapped_file *mf, struct word_tbl *tbl, int urandom_fd) {
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
