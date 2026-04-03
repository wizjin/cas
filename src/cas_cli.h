#ifndef CAS_CLI_H
#define CAS_CLI_H

#include <stdio.h>

typedef struct cas_cli_ctx {
	FILE *out;
	FILE *err;
} cas_cli_ctx_t;

#define cas_cli_info(c, ...)                                                                                         \
	do {                                                                                                           \
		if ((c) != NULL && (c)->out != NULL) {                                                                    \
			(void)fprintf((c)->out, __VA_ARGS__);                                                                 \
		}                                                                                                          \
	} while (0)

#define cas_cli_error(c, ...)                                                                                        \
	do {                                                                                                           \
		if ((c) != NULL && (c)->err != NULL) {                                                                    \
			(void)fprintf((c)->err, __VA_ARGS__);                                                                 \
		}                                                                                                          \
	} while (0)

int cas_cli_run(int argc, char **argv, FILE *out, FILE *err);

#endif
