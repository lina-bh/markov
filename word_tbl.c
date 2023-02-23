#include "word_tbl.h"
#include "assert_syscall.h"
#include "eprintf.h"
#include "get_page_size.h"
#include "make_backing_file.h"
#include "perrno.h"
#include "round_up.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __linux__
#include <errno.h>
#else
#include <sys/errno.h>
#endif

static word_tbl_hash djb2hash(const uint8_t *data, size_t len) {
	word_tbl_hash state = 5381;
	for (size_t i = 0; i < len; i++) {
		state = (state * 33) + data[i];
	}
	return state;
}

int word_tbl_init(struct word_tbl *tbl, size_t cap, int fd) {
	size_t mmap_len = round_up_sz((cap * sizeof(union word_tbl_object)) + sizeof(union word_tbl_object), (size_t)get_page_size());
	int backing_fd = -1;
	if (fd > 0) {
		backing_fd = fd;
	} else {
		backing_fd = make_backing_file();
		if (backing_fd < 0) {
			return backing_fd; // -errno
		}
	}
	if (mmap_len > LLONG_MAX) {
		return -EFBIG;
	}
	if (ftruncate(backing_fd, (off_t)mmap_len) == -1) {
		assert_syscall(close(backing_fd));
		return -errno;
	}
	void *memory = mmap(NULL, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, backing_fd, 0);
	if (memory == MAP_FAILED) {
		if (fd < 0) {
			// close our own fd
			assert_syscall(close(backing_fd));
		}
		return -errno;
	}
	*tbl = (struct word_tbl){
		.memory = memory,
		.state = {
			.mmap_len = mmap_len,
			.backing_fd = backing_fd,
			.normal_file = fd > 0,
		},
	};
	*word_tbl_data_state_of(tbl) = (struct word_tbl_data_state){
		.used = 0,
		.cap = mmap_len / sizeof(union word_tbl_object) - 1, // memory[0] is data state
	};
	return 1;
}

/*
int word_tbl_from_file(struct word_tbl *tbl, int fd) {
	off_t flen = lseek(fd, 0, SEEK_END);
	if (flen == -1 || lseek(fd, 0, SEEK_SET) == -1) {
		return -errno;
	}
	void *memory = mmap(NULL, flen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (memory == MAP_FAILED) {
		return -errno;
	}
	*tbl = (struct word_tbl){
		.memory = memory,
		.state = {
			.mmap_len = flen,
			.backing_fd = fd,
			.normal_file = true,
		}
	};
	return 1;
}
*/

void word_tbl_destroy(struct word_tbl *tbl) {
	assert_syscall_msg(close(tbl->state.backing_fd), "word_tbl_destroy: couldn't close backing fd:");
	if (tbl->state.normal_file && msync(tbl->memory, tbl->state.mmap_len, MS_SYNC) == 1) {
		eprintf("word_tbl_destroy: msync failed, file probably useless\n");
	}
	if (munmap(tbl->memory, tbl->state.mmap_len) == 1) {
		perror("word_tbl_destroy: can't unmap memory, refusing to leak");
		abort();
	}
	memset(tbl, 0, sizeof(*tbl));
}

static int word_tbl_grow(struct word_tbl *tbl, unsigned int mul) {
	size_t new_mmap_len = tbl->state.mmap_len * mul; // will always be integer multiple of page size
	if (new_mmap_len < tbl->state.mmap_len) {
		return -EOVERFLOW;
	}
	if (new_mmap_len > LLONG_MAX) {
		return -EFBIG;
	}
	size_t new_object_cap = new_mmap_len / sizeof(union word_tbl_object) - 1;
	if (ftruncate(tbl->state.backing_fd, (off_t)new_mmap_len) == -1) {
		return -errno;
	}
	if (msync(tbl->memory, tbl->state.mmap_len, MS_SYNC) == -1 && tbl->state.normal_file) {
		eprintf("word_tbl_grow: msync failed, file probably useless\n");
	}
#ifdef __linux__
	void *new_memory = mremap(tbl->memory, tbl->state.mmap_len, new_mmap_len, MREMAP_MAYMOVE);
	if (new_memory == MAP_FAILED) {
		return -errno;
	}
#else
	void *new_memory = mmap(NULL, new_mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED, tbl->state.backing_fd, 0);
	if (new_memory == MAP_FAILED) {
		eprintf("word_tbl_grow: backing file resized but mmap failed, refusing to leave inconsistent state: ");
		perrno(errno);
		abort();
	}
	if (munmap(tbl->memory, tbl->state.mmap_len) == -1) {
		eprintf("word_tbl_grow: munmap failed, refusing to leak memory: ");
		perrno(errno);
		abort();
	}
#endif
	*tbl = (struct word_tbl){
		.memory = new_memory,
		.state = {
			.mmap_len = new_mmap_len,
			.backing_fd = tbl->state.backing_fd,
			.normal_file = tbl->state.normal_file,
		}
	};
	word_tbl_data_state_of(tbl)->cap = new_object_cap;
	return 1;
}

struct word_tbl_ent *word_tbl_by_string(struct word_tbl *tbl, uint8_t *key, size_t key_len) {
	word_tbl_hash hash = djb2hash(key, key_len);
	return word_tbl_by_hash(tbl, hash);
}

struct word_tbl_ent *word_tbl_by_hash(struct word_tbl *tbl, word_tbl_hash hash) {
	struct word_tbl_data_state *data_state = word_tbl_data_state_of(tbl);
	size_t idx = hash % data_state->cap;
	for (size_t j = 0; j < data_state->cap; j++) {
		struct word_tbl_ent *ent = word_tbl_by_index(tbl, idx);
		if (!ent->occupied) {
			// unused slot
			ent->occupied = true;
			ent->hash = hash;
			data_state->used++;
			return ent;
		}
		// elif ent->occupied,
		if (ent->hash == hash) {
			return ent;
		}
		// elif ent->occupied and ent->hash != hash,
		idx = (idx + 1) % data_state->cap;
	}
	// ran out of slots
	int retval;
	if (!(retval = word_tbl_grow(tbl, 2))) {
		return NULL;
	}
	return word_tbl_by_hash(tbl, hash);
}

struct word_tbl_ent *word_tbl_by_index(struct word_tbl *tbl, size_t idx) {
	if (idx >= word_tbl_cap_of(tbl)) {
		return NULL;
	}
	return &tbl->memory[idx + 1].ent;
}

void word_tbl_ent_push(struct word_tbl_ent *ent, word_tbl_hash hash) {
	for (size_t i = 0; i < ent->stackpos; i++) {
		if (ent->nextwords[i] == hash) {
			return;
		}
	}
	if (ent->stackpos + 1 > WORD_TBL_ENT_NEXTWORDS_LEN) {
		eprintf("word_tbl_ent_push: out of space for next words, "
			"increase WORD_TBL_ENT_NEXTWORDS_LEN\n");
		return;
	}
	ent->nextwords[ent->stackpos++] = hash;
}
