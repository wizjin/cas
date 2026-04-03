#ifndef CAS_CTRL_H
#define CAS_CTRL_H

#include "cas_cli.h"

int cas_ctrl_start(cas_cli_ctx_t *cli);
int cas_ctrl_stop(cas_cli_ctx_t *cli);
int cas_ctrl_status(cas_cli_ctx_t *cli);

#endif
