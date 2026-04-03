#include "cas_udss.h"
#include "cas_acts.h"
#include "cas_core.h"
#include "cas_udsp.h"
#include "cas_utils.h"

#include <cJSON.h>
#include <llhttp.h>
#include <uv.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct cas_udss_handler cas_udss_handler_t;
typedef struct {
	const char *method;
	char *path;
	size_t body_len;
	size_t body_cap;
	char *body;
} cas_udss_request_t;
typedef struct {
	int status_code;
	const char *reason_phrase;
	const char *body;
	size_t body_len;
} cas_udss_response_t;

struct cas_udss_handler {
	cas_udsp_request_t *request;
	cJSON *(*handler)(cas_udss_t *s, cJSON *body);
};

enum {
	CAS_UDSS_SPAN_BUFFER_CAPACITY = 256,
	CAS_UDSS_HTTP_PREFIX_CAPACITY = 256,
	CAS_UDSS_HTTP_STATUS_OK = 200,
	CAS_UDSS_HTTP_STATUS_BAD_REQUEST = 400,
	CAS_UDSS_HTTP_STATUS_NOT_FOUND = 404,
	CAS_UDSS_HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
};

#define CAS_UDSS_HANDLER(_name, _handler) [_name] = {.request = NULL, .handler = (_handler)}

static cas_udss_handler_t handlers[CAS_UDSP_CMD_LAST] = {
	CAS_UDSS_HANDLER(CAS_UDSP_CMD_STOP, cas_acts_stop),
	CAS_UDSS_HANDLER(CAS_UDSP_CMD_STATUS, cas_acts_status),
};

static void cas_udss_init_handlers(void)
{
	for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
		handlers[i].request = &cas_udsp_request_tables[i];
	}
}

typedef struct {
	llhttp_t parser;
	llhttp_settings_t settings;
	cas_udss_request_t *request;
	char *url;
	size_t url_len;
	size_t url_cap;
	int complete;
} cas_udss_http_request_t;

static int cas_udss_append_span(char **buffer, size_t *len, size_t *cap, const char *at, size_t size)
{
	if (size == 0) {
		return 0;
	}

	if (*cap - *len <= size) {
		size_t new_cap = *cap == 0 ? CAS_UDSS_SPAN_BUFFER_CAPACITY : *cap;
		while (new_cap - *len <= size) {
			new_cap *= 2;
		}

		char *new_buffer = realloc(*buffer, new_cap);
		if (new_buffer == NULL) {
			return -1;
		}

		*buffer = new_buffer;
		*cap = new_cap;
	}

	(void)memcpy(*buffer + *len, at, size);
	*len += size;
	(*buffer)[*len] = '\0';

	return 0;
}

static int cas_udss_on_request_url(llhttp_t *parser, const char *at, size_t length)
{
	cas_udss_http_request_t *request = parser->data;

	if (request == NULL) {
		return -1;
	}

	return cas_udss_append_span(&request->url, &request->url_len, &request->url_cap, at, length);
}

static int cas_udss_on_request_body(llhttp_t *parser, const char *at, size_t length)
{
	cas_udss_http_request_t *request = parser->data;

	if (request == NULL || request->request == NULL) {
		return -1;
	}

	return cas_udss_append_span(&request->request->body, &request->request->body_len, &request->request->body_cap, at,
								length);
}

static int cas_udss_on_request_complete(llhttp_t *parser)
{
	cas_udss_http_request_t *request = parser->data;

	if (request == NULL || request->request == NULL) {
		return -1;
	}

	request->complete = 1;
	request->request->method = llhttp_method_name((llhttp_method_t)llhttp_get_method(parser));
	request->request->path = request->url;
	return 0;
}

static void cas_udss_reset_request(cas_udss_request_t *request)
{
	if (request == NULL) {
		return;
	}

	cas_free(request->body);
	cas_free(request->path);
	request->body = NULL;
	request->path = NULL;
	request->body_len = 0;
	request->body_cap = 0;
}

static int cas_udss_parse_request(const char *buffer, size_t len, cas_udss_request_t *request)
{
	if (buffer == NULL || request == NULL) {
		return -1;
	}

	cas_udss_http_request_t parser_ctx = {0};
	(void)memset(request, 0, sizeof(*request));
	(void)memset(&parser_ctx, 0, sizeof(parser_ctx));
	parser_ctx.request = request;
	llhttp_settings_init(&parser_ctx.settings);
	parser_ctx.settings.on_url = cas_udss_on_request_url;
	parser_ctx.settings.on_body = cas_udss_on_request_body;
	parser_ctx.settings.on_message_complete = cas_udss_on_request_complete;
	llhttp_init(&parser_ctx.parser, HTTP_REQUEST, &parser_ctx.settings);
	parser_ctx.parser.data = &parser_ctx;

	llhttp_errno_t rc = llhttp_execute(&parser_ctx.parser, buffer, len);
	if (rc != HPE_OK) {
		cas_udss_reset_request(request);
		return rc == HPE_INVALID_EOF_STATE ? 0 : -1;
	}

	rc = llhttp_finish(&parser_ctx.parser);
	if (rc != HPE_OK) {
		cas_udss_reset_request(request);
		return rc == HPE_INVALID_EOF_STATE ? 0 : -1;
	}
	if (!parser_ctx.complete) {
		cas_udss_reset_request(request);
		return 0;
	}

	return 1;
}

static char *cas_udss_build_json_field(const char *name, const char *value)
{
	cJSON *root = cJSON_CreateObject();

	if (root == NULL) {
		return NULL;
	}
	if (!cJSON_AddStringToObject(root, name, value)) {
		cJSON_Delete(root);
		return NULL;
	}

	char *text = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return text;
}

static char *cas_udss_build_response_text(const cas_udss_response_t *response, size_t *size)
{
	char prefix[CAS_UDSS_HTTP_PREFIX_CAPACITY];

	if (response == NULL || response->reason_phrase == NULL) {
		return NULL;
	}

	(void)snprintf(prefix, sizeof(prefix),
				  "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n"
				  "Connection: close\r\n\r\n",
				  response->status_code, response->reason_phrase, response->body_len);
	size_t prefix_len = strlen(prefix);
	size_t total = prefix_len + response->body_len + 1;
	char *buffer = cas_alloc(total);
	if (buffer == NULL) {
		return NULL;
	}

	(void)snprintf(buffer, total, "%s%s", prefix, response->body == NULL ? "" : response->body);
	if (size != NULL) {
		*size = total - 1;
	}

	return buffer;
}

static cas_udss_handler_t *cas_udss_find_handler(const cas_udss_request_t *request)
{
	for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); i++) {
		cas_udss_handler_t *handler = &handlers[i];

		if (handler->request == NULL) {
			continue;
		}
		if (strcmp(request->method, handler->request->method) == 0 &&
			strcmp(request->path, handler->request->path) == 0) {
			return handler;
		}
	}

	return NULL;
}

static cJSON *cas_udss_parse_body(const cas_udss_request_t *request)
{
	if (request->body_len == 0) {
		return NULL;
	}

	return cJSON_Parse(request->body);
}

static int cas_udss_build_success_response(cas_udss_response_t *response, cJSON *body)
{
	if (response == NULL || body == NULL) {
		cJSON_Delete(body);
		return -1;
	}

	char *body_text = cJSON_PrintUnformatted(body);
	cJSON_Delete(body);
	if (body_text == NULL) {
		return -1;
	}

	response->status_code = CAS_UDSS_HTTP_STATUS_OK;
	response->reason_phrase = "OK";
	response->body = body_text;
	response->body_len = strlen(body_text);
	return 0;
}

static void cas_udss_set_error_response(cas_udss_response_t *response, int status_code, const char *reason,
										const char *error)
{
	response->status_code = status_code;
	response->reason_phrase = reason;
	response->body = cas_udss_build_json_field("error", error);
	response->body_len = response->body == NULL ? 0 : strlen(response->body);
}

static void cas_udss_on_read(cas_evnt_uds_peer_t *peer, const char *buffer, size_t size, void *ctx)
{
	cas_udss_t *server = ctx;

	if (server == NULL || peer == NULL) {
		return;
	}

	cas_udss_request_t request = {0};
	int parse_result = cas_udss_parse_request(buffer, size, &request);
	if (parse_result == 0) {
		return;
	}

	cas_udss_response_t response = {0};
	if (parse_result < 0) {
		cas_udss_set_error_response(&response, CAS_UDSS_HTTP_STATUS_BAD_REQUEST, "Bad Request", "bad_request");
	} else {
		cas_udss_handler_t *handler = cas_udss_find_handler(&request);
		if (handler == NULL) {
			cas_udss_set_error_response(&response, CAS_UDSS_HTTP_STATUS_NOT_FOUND, "Not Found", "not_found");
		} else {
			cJSON *body = cas_udss_parse_body(&request);
			if (request.body_len > 0 && body == NULL) {
				cas_udss_set_error_response(&response, CAS_UDSS_HTTP_STATUS_BAD_REQUEST, "Bad Request", "bad_json");
			} else {
				cJSON *reply = handler->handler(server, body);
				cJSON_Delete(body);
				if (cas_udss_build_success_response(&response, reply) != 0) {
					cas_udss_set_error_response(&response, CAS_UDSS_HTTP_STATUS_INTERNAL_SERVER_ERROR,
												"Internal Server Error", "handler_failed");
				}
			}
		}
	}

	size_t payload_size;
	char *payload = cas_udss_build_response_text(&response, &payload_size);
	cas_free((void *)response.body);
	cas_udss_reset_request(&request);
	if (payload == NULL) {
		cas_evnt_uds_close_peer(peer);
		return;
	}

	if (cas_evnt_uds_reply(peer, payload, payload_size) != 0) {
		cas_evnt_uds_close_peer(peer);
	}
	cas_free(payload);
}

cas_udss_t *cas_udss_create(cas_core_t *core)
{
	if (core == NULL || core->log == NULL || core->evnt == NULL) {
		return NULL;
	}

	cas_udss_t *server = cas_alloc(sizeof(*server));
	if (server == NULL) {
		return NULL;
	}

	(void)memset(server, 0, sizeof(*server));
	server->core = core;
	server->log = cas_log_get_category(core->log, "udss");
	if (server->log == NULL) {
		cas_free(server);
		return NULL;
	}

	server->uds = cas_evnt_uds_create(core->evnt, cas_udss_on_read, server);
	if (server->uds == NULL) {
		cas_free(server);
		return NULL;
	}

	cas_udss_init_handlers();

	return server;
}

void cas_udss_release(cas_udss_t *server)
{
	if (server == NULL) {
		return;
	}
	cas_free(server);
}

int cas_udss_start(cas_udss_t *server)
{
	const char *socket_path = cas_udsp_get_skt_path();

	if (server == NULL || socket_path == NULL) {
		return -1;
	}

	int rc = cas_evnt_uds_start(server->uds, socket_path);
	if (rc != 0) {
		cas_log_error(server->log, "bind/listen failed for %s: %s", socket_path, uv_strerror(rc));
		return rc;
	}

	cas_log_info(server->log, "control socket listening at %s", socket_path);
	return 0;
}

void cas_udss_stop(cas_udss_t *server)
{
	if (server == NULL || server->uds == NULL) {
		return;
	}

	cas_evnt_uds_stop(server->uds);
}
