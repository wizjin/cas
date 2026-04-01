#ifndef CAS_UTILS_H
#define CAS_UTILS_H

#include "cas_config.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define cas_alloc(size) malloc(size)
#define cas_free(ptr) free(ptr)

#ifndef bzero
#define bzero(ptr, len) memset(ptr, 0, len)
#endif

void cas_utils_copy_string(char *destination, size_t destination_size, const char *source);

#endif
