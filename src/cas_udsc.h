#ifndef CAS_UDSC_H
#define CAS_UDSC_H

#include <cJSON.h>

cJSON *cas_udsc_get(const char *skt, const char *uri);
cJSON *cas_udsc_post(const char *skt, const char *uri, cJSON *body);

#endif
