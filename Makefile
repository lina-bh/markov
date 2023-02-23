UNAME := $(uname -s)

CC := gcc
ifeq ($(UNAME), Darwin)
CC := clang
endif

IWYU := include-what-you-use
ifeq ($(UNAME), Linux)
IWYU := iwyu
endif

CFLAGS := -g -O0
CPPFLAGS := -D_GNU_SOURCE

all: *.c
	$(CC) -Wall -Wextra $(CFLAGS) $(CPPFLAGS) -lm $^

iwyu:
	$(shell for c in *.c\; do $(IWYU) $(CPPFLAGS) \$c\; done)

.PHONY: all iwyu