#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __linux__
#include <errno.h>
#include <fcntl.h>
#else
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#endif

#include "assert_syscall.h"
#include "get_page_size.h"
#include "mmapped_file.h"
#include "round_up.h"

int mmapped_file_open(struct mmapped_file *mf, const char *path, int oflags) {
	int mmap_flags = 0;
	if (oflags & O_RDWR) {
		mmap_flags = PROT_READ | PROT_WRITE;
	} else if ((oflags & O_RDONLY) == 0) {
		mmap_flags = PROT_READ;
	} else {
		return -EINVAL;
	}

	int fd = -1;
	void *mem = NULL;
	size_t mmap_len = 0;

	fd = open(path, oflags);
	if (fd < 0) {
		goto err;
	}

	off_t flen = lseek(fd, 0, SEEK_END);
	if (flen < 0 || lseek(fd, 0, SEEK_SET) < 0) {
		goto err;
	}
	mmap_len = round_up_sz((size_t)flen, (size_t)get_page_size());
	if (mmap_len < (uint64_t)flen) {
		errno = EOVERFLOW;
		goto err;
	}
	mem = mmap(NULL, mmap_len, mmap_flags, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED) {
		goto err;
	}

	assert_syscall(close(fd));
	fd = -1;

	memset(mf, 0, sizeof(*mf));
	mf->file_len = (uint64_t)flen;
	mf->mmap_len = mmap_len;
	mf->map = mem;
	// mf->fd = -1; // don't keep it
	return 1;
err:
	if (mem != NULL) {
		assert_syscall(munmap(mem, mmap_len));
	}
	if (fd > 0) {
		assert_syscall(close(fd));
	}
	return -errno;
}
