#ifndef WIN_UNISTD_H
#define WIN_UNISTD_H

#include <stdlib.h>
#ifdef _WIN64
#include <io.h>
#else
#include <sys/io.h>
#endif

#include <process.h>

#define ftruncate _chsize_s
#define unlink _unlink
#define fileno _fileno
#define lseek _lseeki64

#ifdef _WIN64
#define ssize_t __int64
#define SSIZE_MAX 9223372036854775807i64
#else
#define ssize_t long
#define SSIZE_MAX 2147483647L
#endif

#include "stdint.h"

#endif //WIN_UNISTD_H
