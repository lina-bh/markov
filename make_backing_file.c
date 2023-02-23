#include "make_backing_file.h"

#ifdef __linux__
#include <errno.h>
#include <sys/mman.h>
#else
#include <sys/errno.h>
#include <unistd.h>
#endif

#define BACKING_FILE_TEMPLATE "/tmp/markovXXXXXX"

int make_backing_file(void) {
#ifdef __linux__
	int fd = memfd_create("markov", 0);
	if (fd < 0) {
		return -errno;
	}
#else
	char tmpl[sizeof(BACKING_FILE_TEMPLATE)] = BACKING_FILE_TEMPLATE;
	int fd = mkstemp(tmpl);
	if (fd < 0) {
		return -errno;
	}
	if (unlink(tmpl) == -1) {
		return -errno;
	}
#endif
	return fd;
}
