#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "cas_acts.h"
#include "cas_cli.h"
#include "cas_config.h"
#include "cas_core.h"
#include "cas_ctrl.h"
#include "cas_udsc.h"
#include "cas_udsp.h"
#include "cas_udss.h"

#include <cJSON.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct {
	pthread_t thread;
	int result;
	FILE *log_out;
} cas_test_service_t;

static void cas_test_make_socket_path(char *buffer, size_t size)
{
	assert_true(snprintf(buffer, size, ".cas/cas.sock") > 0);
}

static void cas_test_prepare_socket_dir(void)
{
	struct stat st;

	if (stat(".cas", &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return;
		}

		assert_int_equal(unlink(".cas"), 0);
	}

	assert_int_equal(mkdir(".cas", 0700), 0);
}

static void *cas_test_service_main(void *ctx)
{
	cas_test_service_t *service = ctx;

	service->result = cas_core_run(service->log_out == NULL ? stderr : service->log_out);
	return NULL;
}

static void cas_test_wait_until_ready(const char *socket_path)
{
	for (size_t i = 0; i < 200; i++) {
		cJSON *response = cas_udsc_get(socket_path, cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path);
		if (response != NULL) {
			cJSON_Delete(response);
			return;
		}
		usleep(10000);
	}

	fail_msg("service did not become ready");
}

static void cas_test_start_service(cas_test_service_t *service, const char *socket_path)
{
	(void)memset(service, 0, sizeof(*service));
	cas_test_prepare_socket_dir();
	assert_int_equal(setenv(CAS_UDS_SOCKET_ENV, socket_path, 1), 0);
	(void)unlink(socket_path);
	service->log_out = tmpfile();
	assert_non_null(service->log_out);
	assert_int_equal(pthread_create(&service->thread, NULL, cas_test_service_main, service), 0);
	cas_test_wait_until_ready(socket_path);
}

static void cas_test_join_service(cas_test_service_t *service)
{
	assert_int_equal(pthread_join(service->thread, NULL), 0);
	assert_int_equal(service->result, 0);
	assert_non_null(service->log_out);
	assert_int_equal(fclose(service->log_out), 0);
	service->log_out = NULL;
}

static void cas_test_stop_service(const char *socket_path)
{
	cJSON *body = cas_udsc_post(socket_path, cas_udsp_request_tables[CAS_UDSP_CMD_STOP].path, NULL);

	assert_non_null(body);
	assert_false(cas_udsp_is_running(body));
	cJSON_Delete(body);
}

static char *cas_test_capture_cli(int argc, char **argv, int *result, int use_error_stream)
{
	FILE *out_stream;
	FILE *err_stream;
	char *buffer = NULL;
	size_t size = 0;
	cas_cli_t cli;

	out_stream = open_memstream(&buffer, &size);
	assert_non_null(out_stream);
	err_stream = use_error_stream ? out_stream : tmpfile();
	assert_non_null(err_stream);
	cli.out = out_stream;
	cli.err = err_stream;
	cli.argc = argc;
	cli.argv = argv;
	*result = cas_cli_run(&cli);
	assert_int_equal(fflush(out_stream), 0);
	if (!use_error_stream) {
		assert_int_equal(fclose(err_stream), 0);
	}
	assert_int_equal(fclose(out_stream), 0);

	return buffer;
}

static int cas_test_open_uds_socket(const char *socket_path)
{
	struct sockaddr_un addr = {0};

	if (strlen(socket_path) >= sizeof(addr.sun_path)) {
		return -1;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}

	addr.sun_family = AF_UNIX;
	(void)strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
	if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
		(void)close(fd);
		return -1;
	}

	return fd;
}

static char *cas_test_read_all(int fd)
{
	size_t cap = 1024;
	size_t len = 0;
	char *buffer = malloc(cap);

	assert_non_null(buffer);
	for (;;) {
		if (len == cap) {
			cap *= 2;
			char *new_buffer = realloc(buffer, cap);

			assert_non_null(new_buffer);
			buffer = new_buffer;
		}

		ssize_t rc = read(fd, buffer + len, cap - len);
		assert_true(rc >= 0);
		if (rc == 0) {
			break;
		}
		len += (size_t)rc;
	}

	buffer[len] = '\0';
	return buffer;
}

static char *cas_test_send_raw_request(const char *socket_path, const char *request)
{
	int fd = cas_test_open_uds_socket(socket_path);

	assert_true(fd >= 0);
	assert_true(write(fd, request, strlen(request)) >= 0);
	assert_int_equal(shutdown(fd, SHUT_WR), 0);
	char *response = cas_test_read_all(fd);
	assert_int_equal(close(fd), 0);
	return response;
}

static void cas_test_cleanup_socket_env(const char *socket_path)
{
	assert_int_equal(unsetenv(CAS_UDS_SOCKET_ENV), 0);
	(void)unlink(socket_path);
	(void)rmdir(".cas");
}

static void cas_test_evnt_signal_cb(void *ctx)
{
	int *count = ctx;

	if (count != NULL) {
		(*count)++;
	}
}

static void cas_test_evnt_read_cb(cas_evnt_uds_peer_t *peer, const char *buffer, size_t size, void *ctx)
{
	(void)peer;
	(void)buffer;
	(void)size;
	(void)ctx;
}

static void cas_service_status_and_stop_round_trip(void **state)
{
	char socket_path[108];
	struct stat st;
	cas_test_service_t service;

	(void)state;

	cas_test_make_socket_path(socket_path, sizeof(socket_path));
	cas_test_start_service(&service, socket_path);

	cJSON *body = cas_udsc_get(socket_path, cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path);
	assert_non_null(body);
	assert_true(cas_udsp_is_running(body));
	cJSON_Delete(body);

	cas_test_stop_service(socket_path);
	cas_test_join_service(&service);
	assert_int_not_equal(stat(socket_path, &st), 0);
	cas_test_cleanup_socket_env(socket_path);
}

static void cas_service_cli_status_and_stop_round_trip(void **state)
{
	char socket_path[108];
	cas_test_service_t service;
	int result = 0;
	char *output;
	char *status_argv[] = {"cas", "status"};
	char *stop_argv[] = {"cas", "stop"};

	(void)state;

	cas_test_make_socket_path(socket_path, sizeof(socket_path));
	cas_test_start_service(&service, socket_path);

	output = cas_test_capture_cli(2, status_argv, &result, 0);
	assert_int_equal(result, 0);
	assert_non_null(strstr(output, "\"status\":\"UP\""));
	free(output);

	output = cas_test_capture_cli(2, stop_argv, &result, 0);
	assert_int_equal(result, 0);
	assert_non_null(strstr(output, "\"status\":\"STOPPING\""));
	free(output);

	cas_test_join_service(&service);
	cas_test_cleanup_socket_env(socket_path);
}

static void cas_service_bad_request_returns_400(void **state)
{
	char socket_path[108];
	cas_test_service_t service;
	char *response;

	(void)state;

	cas_test_make_socket_path(socket_path, sizeof(socket_path));
	cas_test_start_service(&service, socket_path);

	response = cas_test_send_raw_request(socket_path, "GET / HTTP/1.1\r\nHost localhost\r\nContent-Length: 0\r\n\r\n");
	assert_non_null(strstr(response, "400 Bad Request"));
	assert_non_null(strstr(response, "bad_request"));
	free(response);

	cas_test_stop_service(socket_path);
	cas_test_join_service(&service);
	cas_test_cleanup_socket_env(socket_path);
}

static void cas_service_not_found_returns_404(void **state)
{
	char socket_path[108];
	cas_test_service_t service;
	char *response;

	(void)state;

	cas_test_make_socket_path(socket_path, sizeof(socket_path));
	cas_test_start_service(&service, socket_path);

	response =
		cas_test_send_raw_request(socket_path, "GET /unknown HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n");
	assert_non_null(strstr(response, "404 Not Found"));
	assert_non_null(strstr(response, "not_found"));
	free(response);

	cas_test_stop_service(socket_path);
	cas_test_join_service(&service);
	cas_test_cleanup_socket_env(socket_path);
}

static void cas_service_bad_json_returns_400(void **state)
{
	char socket_path[108];
	cas_test_service_t service;
	char *response;

	(void)state;

	cas_test_make_socket_path(socket_path, sizeof(socket_path));
	cas_test_start_service(&service, socket_path);

	response =
		cas_test_send_raw_request(socket_path, "POST /actuator/stop HTTP/1.1\r\nHost: localhost\r\nContent-Type: "
											   "application/json\r\nContent-Length: 1\r\n\r\n{");
	assert_non_null(strstr(response, "400 Bad Request"));
	assert_non_null(strstr(response, "bad_json"));
	free(response);

	cas_test_stop_service(socket_path);
	cas_test_join_service(&service);
	cas_test_cleanup_socket_env(socket_path);
}

static void cas_core_run_removes_stale_socket_file(void **state)
{
	char socket_path[108];
	cas_test_service_t service;

	(void)state;

	cas_test_make_socket_path(socket_path, sizeof(socket_path));
	cas_test_prepare_socket_dir();
	assert_int_equal(setenv(CAS_UDS_SOCKET_ENV, socket_path, 1), 0);
	FILE *stale = fopen(socket_path, "w");
	assert_non_null(stale);
	assert_int_equal(fclose(stale), 0);

	service.log_out = tmpfile();
	assert_non_null(service.log_out);
	service.result = -1;
	assert_int_equal(pthread_create(&service.thread, NULL, cas_test_service_main, &service), 0);
	cas_test_wait_until_ready(socket_path);
	cas_test_stop_service(socket_path);
	cas_test_join_service(&service);
	cas_test_cleanup_socket_env(socket_path);
}

static void cas_core_and_acts_helpers_work(void **state)
{
	cas_core_t core = {.state = CAS_CORE_STATE_RUNNING};
	cas_udss_t server = {.core = &core};

	(void)state;

	assert_string_equal(cas_core_state_text(CAS_CORE_STATE_RUNNING), "UP");
	assert_string_equal(cas_core_state_text(CAS_CORE_STATE_STOPPING), "STOPPING");
	assert_string_equal(cas_core_state_text(CAS_CORE_STATE_STOPPED), "DOWN");
	assert_int_equal(cas_core_get_state(NULL), CAS_CORE_STATE_STOPPED);
	assert_int_equal(cas_core_request_stop(NULL), -1);
	core.state = CAS_CORE_STATE_STOPPED;
	assert_int_equal(cas_core_request_stop(&core), 0);
	core.state = CAS_CORE_STATE_STOPPING;
	assert_int_equal(cas_core_request_stop(&core), 0);
	core.state = CAS_CORE_STATE_RUNNING;
	assert_int_equal(cas_core_request_stop(&core), 0);
	assert_int_equal(cas_core_get_state(&core), CAS_CORE_STATE_STOPPING);
	assert_int_equal(cas_core_request_stop(&core), 0);

	cJSON *status = cas_acts_status(&server, NULL);
	assert_non_null(status);
	assert_false(cas_udsp_is_running(status));
	cJSON_Delete(status);

	core.state = CAS_CORE_STATE_RUNNING;
	cJSON *stop = cas_acts_stop(&server, NULL);
	assert_non_null(stop);
	assert_false(cas_udsp_is_running(stop));
	cJSON_Delete(stop);

	assert_null(cas_acts_status(NULL, NULL));
	assert_null(cas_acts_stop(NULL, NULL));
}

static void cas_udsp_status_helpers_cover_false_cases(void **state)
{
	cJSON *empty = cJSON_CreateObject();
	cJSON *down = cJSON_CreateObject();

	(void)state;

	assert_false(cas_udsp_is_running(NULL));
	assert_non_null(empty);
	assert_false(cas_udsp_is_running(empty));
	assert_non_null(down);
	assert_true(cJSON_AddStringToObject(down, "status", "DOWN") != NULL);
	assert_false(cas_udsp_is_running(down));
	cJSON_Delete(empty);
	cJSON_Delete(down);
}

static void cas_udsc_invalid_inputs_return_null(void **state)
{
	char socket_path[sizeof(((struct sockaddr_un *)0)->sun_path) + 32];
	cJSON *body = cJSON_CreateObject();

	(void)state;

	(void)memset(socket_path, 'a', sizeof(socket_path) - 1);
	socket_path[sizeof(socket_path) - 1] = '\0';
	assert_null(cas_udsc_get(NULL, cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path));
	errno = 0;
	assert_null(cas_udsc_get(socket_path, cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path));
	assert_int_equal(errno, ENAMETOOLONG);
	assert_null(cas_udsc_get(".cas/missing.sock", cas_udsp_request_tables[CAS_UDSP_CMD_STATUS].path));
	assert_non_null(body);
	assert_true(cJSON_AddStringToObject(body, "status", "UP") != NULL);
	assert_null(cas_udsc_post(".cas/missing.sock", cas_udsp_request_tables[CAS_UDSP_CMD_STOP].path, body));
	cJSON_Delete(body);
}

static void cas_evnt_public_guards_and_lifecycle(void **state)
{
	(void)state;

	assert_null(cas_evnt_uds_create(NULL, cas_test_evnt_read_cb, NULL));
	assert_null(cas_evnt_uds_create((cas_evnt_t *)1, NULL, NULL));
	cas_evnt_release(NULL);
	assert_int_equal(cas_evnt_run(NULL, UV_RUN_NOWAIT), -1);
	assert_int_equal(cas_evnt_start_signal(NULL, SIGINT, cas_test_evnt_signal_cb, NULL), -1);
	cas_evnt_stop_signal(NULL);
	cas_evnt_uds_release(NULL);
	assert_int_equal(cas_evnt_uds_start(NULL, ".cas/unused.sock"), -1);
	cas_evnt_uds_stop(NULL);
	assert_int_equal(cas_evnt_uds_reply(NULL, "x", 1), -1);
	assert_int_equal(cas_evnt_uds_reply((cas_evnt_uds_peer_t *)1, NULL, 1), -1);
	cas_evnt_uds_close_peer(NULL);

	cas_evnt_t *evnt = cas_evnt_create();
	assert_non_null(evnt);
	assert_int_equal(cas_evnt_run(evnt, UV_RUN_NOWAIT), 0);
	assert_int_equal(cas_evnt_start_signal(evnt, SIGUSR1, cas_test_evnt_signal_cb, NULL), 0);
	cas_evnt_stop_signal(evnt);
	cas_evnt_stop_signal(evnt);

	cas_evnt_uds_t *uds = cas_evnt_uds_create(evnt, cas_test_evnt_read_cb, NULL);
	assert_non_null(uds);
	assert_int_equal(cas_evnt_uds_start(uds, NULL), -1);
	assert_true(cas_evnt_uds_start(uds, ".cas/missing/cas.sock") != 0);
	cas_evnt_uds_release(uds);
	cas_evnt_uds_stop(uds);
	assert_int_equal(cas_evnt_run(evnt, UV_RUN_NOWAIT), 0);
	cas_evnt_release(evnt);
}

static void cas_core_run_fails_when_service_is_already_running(void **state)
{
	char socket_path[108];
	cas_test_service_t service;
	char buffer[256] = {0};

	(void)state;

	cas_test_make_socket_path(socket_path, sizeof(socket_path));
	cas_test_start_service(&service, socket_path);

	FILE *log_out = tmpfile();
	assert_non_null(log_out);
	assert_int_equal(cas_core_run(log_out), 1);
	rewind(log_out);
	assert_true(fread(buffer, 1, sizeof(buffer) - 1, log_out) > 0);
	assert_non_null(strstr(buffer, "Failed to prepare socket"));
	assert_int_equal(fclose(log_out), 0);

	cas_test_stop_service(socket_path);
	cas_test_join_service(&service);
	cas_test_cleanup_socket_env(socket_path);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(cas_service_status_and_stop_round_trip),
		cmocka_unit_test(cas_service_cli_status_and_stop_round_trip),
		cmocka_unit_test(cas_service_bad_request_returns_400),
		cmocka_unit_test(cas_service_not_found_returns_404),
		cmocka_unit_test(cas_service_bad_json_returns_400),
		cmocka_unit_test(cas_core_run_removes_stale_socket_file),
		cmocka_unit_test(cas_core_and_acts_helpers_work),
		cmocka_unit_test(cas_udsp_status_helpers_cover_false_cases),
		cmocka_unit_test(cas_udsc_invalid_inputs_return_null),
		cmocka_unit_test(cas_evnt_public_guards_and_lifecycle),
		cmocka_unit_test(cas_core_run_fails_when_service_is_already_running),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
