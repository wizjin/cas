#include "cas_utils.h"

#include <stdio.h>

void cas_utils_copy_string(char *dst, size_t dst_size, const char *src)
{
	if (dst_size == 0) {
		return;
	}

	(void)snprintf(dst, dst_size, "%s", src);
}
