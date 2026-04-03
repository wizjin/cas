#ifndef CAS_CORE_H
#define CAS_CORE_H

#include "cas_evnt.h"
#include "cas_log.h"

#include <stdio.h>
#include <stdbool.h>

typedef struct cas_udss cas_udss_t;

typedef enum {
	CAS_CORE_STATE_RUNNING = 0,
	CAS_CORE_STATE_STOPPING,
	CAS_CORE_STATE_STOPPED,
} cas_core_state_t;

typedef struct cas_core {
	const char *socket_path;
	FILE *log_out;
	cas_log_t *log;
	cas_log_category_t *core_log;
	cas_core_state_t state;
	cas_evnt_t *evnt;
	cas_udss_t *server;
	bool evnt_ready;
	bool signal_ready;
	bool server_ready;
} cas_core_t;

int cas_core_run(FILE *log_out);
int cas_core_request_stop(cas_core_t *core);
cas_core_state_t cas_core_get_state(const cas_core_t *core);
const char *cas_core_state_text(cas_core_state_t state);

#endif
