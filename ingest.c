#include "ingest.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "mmapped_file.h"
#include "word_tbl.h"

#define SKIPPED_CHARS ".,\"'?!()*@#:\n"

static bool skipchar(uint8_t c) {
	for (size_t i = 0; i < sizeof(SKIPPED_CHARS); i++) {
		if (c == SKIPPED_CHARS[i]) {
			return true;
		}
	}
	return false;
}

void ingest(struct mmapped_file *mf, struct word_tbl *tbl) {
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
