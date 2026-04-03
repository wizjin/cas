#include "cas_ctrl.h"
#include "cas_core.h"
#include "cas_udsc.h"
#include "cas_udsp.h"
#include "cas_utils.h"

#include <cJSON.h>
#include <stdio.h>

int cas_ctrl_start(cas_cli_ctx_t *cli)
{
	if (cli == NULL) {
		return 1;
	}

	return cas_core_run(stderr);
}

int cas_ctrl_stop(cas_cli_ctx_t *cli)
{
	if (cli == NULL) {
		return 1;
	}

	cJSON *body = cas_udsc_post(cas_udsp_get_skt_path(), cas_udsp_request_tables[CAS_UDSP_CMD_STOP].path, NULL);
	if (body == NULL) {
		cas_cli_error(cli, "Failed to stop service via %s\n", cas_udsp_get_skt_path());
		return 1;
	}

	char *text = cJSON_PrintUnformatted(body);
	cJSON_Delete(body);
	if (text == NULL) {
		return 1;
	}

	cas_cli_info(cli, "%s\n", text);
	cJSON_free(text);

	return 0;
}

int cas_ctrl_status(cas_cli_ctx_t *cli)
{
	if (cli == NULL) {
		return 1;
	}

	cJSON *body = cas_udsc_get(cas_udsp_get_skt_path(), cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path);
	if (body == NULL) {
		cas_cli_error(cli, "Failed to query service via %s\n", cas_udsp_get_skt_path());
		return 1;
	}

	char *text = cJSON_PrintUnformatted(body);
	cJSON_Delete(body);
	if (text == NULL) {
		return 1;
	}

	cas_cli_info(cli, "%s\n", text);
	cJSON_free(text);

	return 0;
}
