#include "cas_acts.h"
#include "cas_core.h"

cJSON *cas_acts_stop(cas_udss_t *s, cJSON *body)
{
	(void)body;
	if (s == NULL || s->core == NULL) {
		return NULL;
	}

	cJSON *response = cJSON_CreateObject();
	if (response == NULL) {
		return NULL;
	}

	if (cas_core_request_stop(s->core) != 0) {
		if (!cJSON_AddStringToObject(response, "error", "stop_failed")) {
			cJSON_Delete(response);
			return NULL;
		}
	} else {
		if (!cJSON_AddStringToObject(response, "status", "STOPPING")) {
			cJSON_Delete(response);
			return NULL;
		}
	}

	return response;
}

cJSON *cas_acts_status(cas_udss_t *s, cJSON *body)
{
	(void)body;
	if (s == NULL || s->core == NULL) {
		return NULL;
	}

	cJSON *response = cJSON_CreateObject();
	if (response == NULL) {
		return NULL;
	}

	if (!cJSON_AddStringToObject(response, "status", cas_core_state_text(cas_core_get_state(s->core)))) {
		cJSON_Delete(response);
		return NULL;
	}

	return response;
}
