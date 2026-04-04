#include "cas_udsc.h"
#include "cas_udsp.h"
#include "cas_utils.h"

#include <cJSON.h>
#include <llhttp.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SUN_LEN
#define SUN_LEN(ptr) ((size_t)offsetof(struct sockaddr_un, sun_path) + strlen((ptr)->sun_path) + 1U)
#endif

enum {
	CAS_UDSC_READ_BUFFER_CAPACITY = 1024,
	CAS_UDSC_SPAN_BUFFER_CAPACITY = 256,
	CAS_UDSC_HTTP_PREFIX_CAPACITY = 256,
	CAS_UDSC_HTTP_STATUS_OK_MIN = 200,
	CAS_UDSC_HTTP_STATUS_OK_MAX = 300,
};

static int cas_udsc_open_socket(const char *socket_path)
{
	if (socket_path == NULL) {
		return -1;
	}

	struct sockaddr_un addr = {0};
	if (strlen(socket_path) >= sizeof(addr.sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}

	addr.sun_family = AF_UNIX;
	(void)strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	if (connect(fd, (const struct sockaddr *)&addr, (socklen_t)SUN_LEN(&addr)) != 0) {
		int saved_errno = errno;
		(void)close(fd);
		errno = saved_errno;
		return -1;
	}

	return fd;
}

static int cas_udsc_send_all(int fd, const char *buffer, size_t len)
{
	size_t written = 0;

	while (written < len) {
		ssize_t rc = write(fd, buffer + written, len - written);

		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			}
			return -1;
		}

		written += (size_t)rc;
	}

	return 0;
}

static int cas_udsc_read_all(int fd, char **buffer, size_t *size)
{
	size_t cap = CAS_UDSC_READ_BUFFER_CAPACITY;
	size_t len = 0;
	char *data = cas_alloc(cap);

	if (data == NULL) {
		return -1;
	}

	for (;;) {
		if (len == cap) {
			size_t new_cap = cap * 2;
			char *new_data = realloc(data, new_cap);

			if (new_data == NULL) {
				cas_free(data);
				return -1;
			}

			data = new_data;
			cap = new_cap;
		}

		ssize_t rc = read(fd, data + len, cap - len);
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			}
			cas_free(data);
			return -1;
		}

		if (rc == 0) {
			break;
		}

		len += (size_t)rc;
	}

	*buffer = data;
	*size = len;

	return 0;
}

typedef struct {
	llhttp_t parser;
	llhttp_settings_t settings;
	int complete;
	int status_code;
	char *body;
	size_t body_len;
	size_t body_cap;
} cas_udsc_http_response_t;

static int cas_udsc_append_span(char **buffer, size_t *len, size_t *cap, const char *at, size_t size)
{
	if (size == 0) {
		return 0;
	}

	if (*cap - *len <= size) {
		size_t new_cap = *cap == 0 ? CAS_UDSC_SPAN_BUFFER_CAPACITY : *cap;
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

static int cas_udsc_on_response_body(llhttp_t *parser, const char *at, size_t length)
{
	cas_udsc_http_response_t *response = parser->data;

	if (response == NULL) {
		return -1;
	}

	if (cas_udsc_append_span(&response->body, &response->body_len, &response->body_cap, at, length) != 0) {
		return -1;
	}

	return 0;
}

static int cas_udsc_on_response_complete(llhttp_t *parser)
{
	cas_udsc_http_response_t *response = parser->data;

	if (response == NULL) {
		return -1;
	}

	response->complete = 1;
	response->status_code = llhttp_get_status_code(parser);
	return 0;
}

static void cas_udsc_reset_response(cas_udsc_http_response_t *response)
{
	if (response == NULL) {
		return;
	}

	cas_free(response->body);
	response->body = NULL;
	response->body_len = 0;
	response->body_cap = 0;
}

static int cas_udsc_parse_response(const char *buffer, size_t len, int *status_code, const char **body,
								   size_t *body_len)
{
	if (buffer == NULL || status_code == NULL || body == NULL || body_len == NULL) {
		return -1;
	}

	cas_udsc_http_response_t response = {0};
	(void)memset(&response, 0, sizeof(response));
	llhttp_settings_init(&response.settings);
	response.settings.on_body = cas_udsc_on_response_body;
	response.settings.on_message_complete = cas_udsc_on_response_complete;
	llhttp_init(&response.parser, HTTP_RESPONSE, &response.settings);
	response.parser.data = &response;

	llhttp_errno_t rc = llhttp_execute(&response.parser, buffer, len);
	if (rc != HPE_OK) {
		cas_udsc_reset_response(&response);
		return rc == HPE_INVALID_EOF_STATE ? 0 : -1;
	}

	rc = llhttp_finish(&response.parser);
	if (rc != HPE_OK) {
		cas_udsc_reset_response(&response);
		return rc == HPE_INVALID_EOF_STATE ? 0 : -1;
	}
	if (!response.complete) {
		cas_udsc_reset_response(&response);
		return 0;
	}
	if (response.body == NULL) {
		response.body = cas_alloc(1);
		if (response.body == NULL) {
			return -1;
		}
		response.body[0] = '\0';
	}

	*status_code = response.status_code;
	*body = response.body;
	*body_len = response.body_len;
	return 1;
}

static char *cas_udsc_build_request_text(const char *method, const char *path, const char *body, size_t *size)
{
	size_t body_len = body == NULL ? 0 : strlen(body);
	char prefix[CAS_UDSC_HTTP_PREFIX_CAPACITY];

	if (method == NULL || path == NULL) {
		return NULL;
	}

	(void)snprintf(prefix, sizeof(prefix),
				   "%s %s HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\n"
				   "Content-Length: %zu\r\nConnection: close\r\n\r\n",
				   method, path, body_len);
	size_t prefix_len = strlen(prefix);
	size_t total = prefix_len + body_len + 1;
	char *buffer = cas_alloc(total);
	if (buffer == NULL) {
		return NULL;
	}

	(void)snprintf(buffer, total, "%s%s", prefix, body == NULL ? "" : body);
	if (size != NULL) {
		*size = total - 1;
	}

	return buffer;
}

static cJSON *cas_udsc_request(const char *socket_path, const char *method, const char *path, const char *json_body)
{
	int fd = cas_udsc_open_socket(socket_path);
	if (fd < 0) {
		return NULL;
	}

	size_t request_size;
	char *request = cas_udsc_build_request_text(method, path, json_body, &request_size);
	if (request == NULL) {
		(void)close(fd);
		return NULL;
	}

	if (cas_udsc_send_all(fd, request, request_size) != 0) {
		cas_free(request);
		(void)close(fd);
		return NULL;
	}

	if (shutdown(fd, SHUT_WR) != 0) {
		cas_free(request);
		(void)close(fd);
		return NULL;
	}

	size_t response_size;
	char *response;
	if (cas_udsc_read_all(fd, &response, &response_size) != 0) {
		cas_free(request);
		(void)close(fd);
		return NULL;
	}
	cas_free(request);
	(void)close(fd);

	int status_code;
	size_t response_body_len;
	const char *response_body;
	int parse_result =
		cas_udsc_parse_response(response, response_size, &status_code, &response_body, &response_body_len);
	if (parse_result != 1) {
		cas_free(response);
		return NULL;
	}

	if (status_code < CAS_UDSC_HTTP_STATUS_OK_MIN || status_code >= CAS_UDSC_HTTP_STATUS_OK_MAX) {
		cas_free((void *)response_body);
		cas_free(response);
		return NULL;
	}

	cJSON *result = cJSON_Parse(response_body);
	cas_free((void *)response_body);
	cas_free(response);

	return result;
}

cJSON *cas_udsc_get(const char *skt, const char *uri)
{
	return cas_udsc_request(skt, "GET", uri, NULL);
}

cJSON *cas_udsc_post(const char *skt, const char *uri, cJSON *body)
{
	if (body == NULL) {
		return cas_udsc_request(skt, "POST", uri, "{}");
	}

	char *text = cJSON_PrintUnformatted(body);
	if (text == NULL) {
		return NULL;
	}

	cJSON *response = cas_udsc_request(skt, "POST", uri, text);
	cJSON_free(text);
	return response;
}
