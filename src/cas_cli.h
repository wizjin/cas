#ifndef CAS_CLI_H
#define CAS_CLI_H

#include "cas_cfg.h"
#include <stdio.h>

typedef struct {
	FILE *out;
	FILE *err;
	int argc;
	char **argv;
} cas_cli_t;

#define cas_cli_out(c, ...) (void)fprintf((c)->out, __VA_ARGS__)
#define cas_cli_err(c, ...) (void)fprintf((c)->err, __VA_ARGS__)

int cas_cli_run(const cas_cli_t *cli);

#endif
