#include "cas_utils.h"

#include <stdio.h>

void cas_utils_copy_string(char *destination, size_t destination_size, const char *source)
{
	if (destination_size == 0) {
		return;
	}

	(void)snprintf(destination, destination_size, "%s", source);
}
