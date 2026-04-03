#ifndef CAS_UTILS_H
#define CAS_UTILS_H

#include "cas_config.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define cas_alloc(size) malloc(size)
#define cas_free(ptr)	free(ptr)
#define cas_str_copy(dst, dst_size, src) ((void)strlcpy((dst), (src), (dst_size)))

#ifndef bzero
#define bzero(ptr, len) memset(ptr, 0, len)
#endif

#endif
