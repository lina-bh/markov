#include "get_page_size.h"

#include <unistd.h>

long get_page_size(void) {
	static long page_size = -1;
	if (page_size == -1) {
		page_size = sysconf(_SC_PAGESIZE);
	}
	return page_size;
}
