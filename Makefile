UNAME := $(uname -s)

CC := gcc
ifeq ($(UNAME), Darwin)
CC := clang
endif

IWYU := include-what-you-use
ifeq ($(UNAME), Linux)
IWYU := iwyu
endif

CFLAGS := -O3 -flto -pipe
CPPFLAGS := -D_GNU_SOURCE

all: *.c
	$(CC) -Wall -Wextra $(CFLAGS) $(CPPFLAGS) -lm $^

.PHONY: all iwyu
