#ifndef CAS_UDSS_H
#define CAS_UDSS_H

#include "cas_evnt.h"
#include "cas_log.h"

typedef struct cas_core cas_core_t;

typedef struct cas_udss {
	cas_core_t *core;
	cas_log_category_t *log;
	cas_evnt_uds_t *uds;
} cas_udss_t;

cas_udss_t *cas_udss_create(cas_core_t *c);
void cas_udss_release(cas_udss_t *s);
int cas_udss_start(cas_udss_t *s);
void cas_udss_stop(cas_udss_t *s);

#endif
