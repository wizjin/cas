#ifndef CAS_CTRL_H
#define CAS_CTRL_H

#include "cas_cli.h"

int cas_ctrl_start(const cas_cli_t *cli);
int cas_ctrl_stop(const cas_cli_t *cli);
int cas_ctrl_status(const cas_cli_t *cli);

#endif
