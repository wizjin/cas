#ifndef CAS_ACTS_H
#define CAS_ACTS_H

#include "cas_udss.h"

#include <cJSON.h>

cJSON *cas_acts_stop(cas_udss_t *s, cJSON *body);
cJSON *cas_acts_status(cas_udss_t *s, cJSON *body);

#endif
