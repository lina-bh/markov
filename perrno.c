#include "perrno.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void perrno(int _errno) {
	fprintf(stderr, "%s\n", strerror(abs(_errno)));
}
