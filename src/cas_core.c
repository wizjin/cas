#include "cas_core.h"
#include "cas_evnt.h"
#include "cas_log.h"
#include "cas_udsc.h"
#include "cas_udsp.h"
#include "cas_udss.h"

#include <cJSON.h>
#include <unistd.h>

#include <signal.h>
#include <errno.h>

static void cas_core_signal_stop(void *ctx)
{
	(void)cas_core_request_stop(ctx);
}

static int cas_core_cleanup_stale_socket(void)
{
	const char *socket_path = cas_udsp_get_skt_path();

	if (access(socket_path, F_OK) != 0) {
		return 0;
	}

	cJSON *response = cas_udsc_get(socket_path, cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path);
	if (response != NULL) {
		bool running = cas_udsp_is_running(response);
		cJSON_Delete(response);
		if (running) {
			errno = EADDRINUSE;
			return -1;
		}
	}

	return unlink(socket_path);
}

static int cas_core_init(cas_core_t *core, const char *socket_path, FILE *log_out)
{
	if (core == NULL || socket_path == NULL || log_out == NULL) {
		return -1;
	}

	(void)memset(core, 0, sizeof(*core));
	core->socket_path = socket_path;
	core->log_out = log_out;
	core->state = CAS_CORE_STATE_RUNNING;
	core->log = cas_log_create(log_out, CAS_LOG_LEVEL_INFO);
	if (core->log == NULL) {
		return -1;
	}

	core->core_log = cas_log_get_category(core->log, "core");
	if (core->core_log == NULL) {
		return -1;
	}

	core->evnt = cas_evnt_create();
	if (core->evnt == NULL) {
		return -1;
	}
	core->evnt_ready = true;

	core->server = cas_udss_create(core);
	if (core->server == NULL) {
		return -1;
	}
	core->server_ready = true;

	if (cas_evnt_start_signal(core->evnt, SIGINT, cas_core_signal_stop, core) != 0) {
		return -1;
	}
	core->signal_ready = true;

	return 0;
}

static void cas_core_fini(cas_core_t *core)
{
	if (core == NULL) {
		return;
	}

	if (core->server_ready) {
		cas_udss_stop(core->server);
	}
	if (core->signal_ready) {
		cas_evnt_stop_signal(core->evnt);
	}
	if (core->socket_path != NULL) {
		(void)unlink(core->socket_path);
	}
	if (core->evnt_ready) {
		cas_evnt_release(core->evnt);
		core->evnt = NULL;
	}
	if (core->server != NULL) {
		cas_udss_release(core->server);
		core->server = NULL;
	}
	cas_log_release(&core->log);
}

int cas_core_run(FILE *log_out)
{
	const char *socket_path = cas_udsp_get_skt_path();
	cas_core_t core = {0};

	if (cas_core_cleanup_stale_socket() != 0) {
		(void)fprintf(log_out, "Failed to prepare socket %s: %s\n", socket_path, strerror(errno));
		return 1;
	}

	if (cas_core_init(&core, socket_path, log_out) != 0) {
		(void)fprintf(log_out, "Failed to initialize core service\n");
		cas_core_fini(&core);
		return 1;
	}

	if (cas_udss_start(core.server) != 0) {
		(void)fprintf(log_out, "Failed to start control socket at %s\n", socket_path);
		cas_core_fini(&core);
		return 1;
	}

	cas_log_info(core.core_log, "service started");
	(void)cas_evnt_run(core.evnt, UV_RUN_DEFAULT);
	core.state = CAS_CORE_STATE_STOPPED;
	cas_log_info(core.core_log, "service stopped");
	cas_core_fini(&core);

	return 0;
}

int cas_core_request_stop(cas_core_t *core)
{
	if (core == NULL) {
		return -1;
	}

	if (core->state == CAS_CORE_STATE_STOPPED) {
		return 0;
	}

	if (core->state == CAS_CORE_STATE_STOPPING) {
		return 0;
	}

	core->state = CAS_CORE_STATE_STOPPING;
	cas_log_info(core->core_log, "service stopping");
	if (core->server_ready) {
		cas_udss_stop(core->server);
	}
	if (core->signal_ready) {
		cas_evnt_stop_signal(core->evnt);
	}

	return 0;
}

cas_core_state_t cas_core_get_state(const cas_core_t *core)
{
	if (core == NULL) {
		return CAS_CORE_STATE_STOPPED;
	}

	return core->state;
}

const char *cas_core_state_text(cas_core_state_t state)
{
	switch (state) {
	case CAS_CORE_STATE_RUNNING:
		return "UP";
	case CAS_CORE_STATE_STOPPING:
		return "STOPPING";
	case CAS_CORE_STATE_STOPPED:
	default:
		return "DOWN";
	}
}
