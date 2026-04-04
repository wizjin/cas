#include "cas_ctrl.h"
#include "cas_core.h"
#include "cas_udsc.h"
#include "cas_udsp.h"
#include "cas_utils.h"

#include <cJSON.h>

int cas_ctrl_start(const cas_cli_t *cli)
{
	assert(cli != NULL);
	return cas_core_run(cli->err);
}

int cas_ctrl_stop(const cas_cli_t *cli)
{
	assert(cli != NULL);
	cJSON *body = cas_udsc_post(cas_udsp_get_skt_path(), cas_udsp_request_tables[CAS_UDSP_CMD_STOP].path, NULL);
	if (body == NULL) {
		cas_cli_err(cli, "Failed to stop service via %s\n", cas_udsp_get_skt_path());
		return 1;
	}

	char *text = cJSON_PrintUnformatted(body);
	cJSON_Delete(body);
	if (text == NULL) {
		return 1;
	}

	cas_cli_out(cli, "%s\n", text);
	cJSON_free(text);

	return 0;
}

int cas_ctrl_status(const cas_cli_t *cli)
{
	assert(cli != NULL);

	cJSON *body = cas_udsc_get(cas_udsp_get_skt_path(), cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path);
	if (body == NULL) {
		cas_cli_err(cli, "Failed to query service via %s\n", cas_udsp_get_skt_path());
		return 1;
	}

	char *text = cJSON_PrintUnformatted(body);
	cJSON_Delete(body);
	if (text == NULL) {
		return 1;
	}

	cas_cli_out(cli, "%s\n", text);
	cJSON_free(text);

	return 0;
}
