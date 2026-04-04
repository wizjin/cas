#ifndef CAS_UDSP_H
#define CAS_UDSP_H

#include "cas_cfg.h"

#include <cJSON.h>

typedef struct {
	const char *method;
	const char *path;
} cas_udsp_request_t;

typedef enum {
	CAS_UDSP_CMD_STOP = 0,
	CAS_UDSP_CMD_STATUS,
	CAS_UDSP_CMD_LAST,
} cas_udsp_cmd_t;

extern cas_udsp_request_t cas_udsp_request_tables[CAS_UDSP_CMD_LAST];

const char *cas_udsp_get_skt_path(void);
bool cas_udsp_is_running(cJSON *res);

#endif
