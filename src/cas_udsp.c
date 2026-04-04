#include "cas_udsp.h"
#include "cas_utils.h"

cas_udsp_request_t cas_udsp_request_tables[CAS_UDSP_CMD_LAST] = {
	[CAS_UDSP_CMD_STOP] = {.method = "POST", .path = "/actuator/stop"},
	[CAS_UDSP_CMD_STATUS] = {.method = "GET", .path = "/actuator/health"},
};

const char *cas_udsp_get_skt_path(void)
{
	const char *value = getenv(CAS_UDS_SOCKET_ENV);

	if (value == NULL || value[0] == '\0') {
		return CAS_UDS_SKT_DEFAULT_PATH;
	}

	return value;
}

bool cas_udsp_is_running(cJSON *res)
{
	cJSON *status;

	if (res == NULL) {
		return false;
	}

	status = cJSON_GetObjectItemCaseSensitive(res, "status");
	if (!cJSON_IsString(status) || status->valuestring == NULL) {
		return false;
	}

	return strcmp(status->valuestring, "UP") == 0;
}
