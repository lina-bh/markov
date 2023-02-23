#ifndef ASSERT_SYSCALL_H
#define ASSERT_SYSCALL_H

#include <stdio.h>

#define assert_syscall(expr) do { if ((expr) == -1) { perror(NULL); abort(); } } while (0)

#define assert_syscall_msg(expr, fmt) do { if ((expr) == -1) { perror(fmt); abort(); } } while (0)

#endif // ASSERT_SYSCALL_H
