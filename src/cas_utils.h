#ifndef CAS_UTILS_H
#define CAS_UTILS_H

#include "cas_cfg.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define CAS_PRINTF_FORMAT(format_index, first_arg_index) __attribute__((format(printf, format_index, first_arg_index)))
#else
#define CAS_PRINTF_FORMAT(format_index, first_arg_index)
#endif

#define cas_alloc(size)					 malloc(size)
#define cas_free(ptr)					 free(ptr)
#define cas_str_copy(dst, dst_size, src) ((void)strlcpy((dst), (src), (dst_size)))
#define cas_countof(c)					 ((int)(sizeof(c) / sizeof((c)[0])))

#ifndef bzero
#define bzero(ptr, len) memset(ptr, 0, len)
#endif

#endif
