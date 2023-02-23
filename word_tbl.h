#ifndef WORD_TBL_H
#define WORD_TBL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define WORD_TBL_ENT_NEXTWORDS_LEN 4096

typedef uint32_t word_tbl_hash;
typedef uint64_t word_tbl_used;

struct word_tbl_ent {
	size_t start;
	size_t end;
	size_t stackpos;
	word_tbl_hash hash;
	bool occupied;
	word_tbl_hash nextwords[WORD_TBL_ENT_NEXTWORDS_LEN];
};

struct word_tbl_process_state {
	size_t mmap_len;
	int backing_fd;
	bool normal_file;
};

struct word_tbl_data_state {
	word_tbl_used cap;
	word_tbl_used used;
};

union word_tbl_object {
	struct word_tbl_ent ent;
	struct word_tbl_data_state state;
};

struct word_tbl {
	union word_tbl_object *memory;
	struct word_tbl_process_state state;
};

static inline struct word_tbl_data_state *word_tbl_data_state_of(struct word_tbl *tbl) {
	return &tbl->memory[0].state;
}

static inline word_tbl_used word_tbl_used_of(struct word_tbl *tbl) {
	return word_tbl_data_state_of(tbl)->used;
}

static inline word_tbl_used word_tbl_cap_of(struct word_tbl *tbl) {
	return word_tbl_data_state_of(tbl)->cap;
}

int word_tbl_init(struct word_tbl *tbl, size_t cap, int fd);
void word_tbl_destroy(struct word_tbl *tbl);
struct word_tbl_ent *word_tbl_by_string(struct word_tbl *tbl, uint8_t *key, size_t key_len);
struct word_tbl_ent *word_tbl_by_hash(struct word_tbl *tbl, word_tbl_hash hash);
struct word_tbl_ent *word_tbl_by_index(struct word_tbl *tbl, size_t idx);
void word_tbl_ent_push(struct word_tbl_ent *ent, word_tbl_hash hash);

#endif // WORD_TBL_H
