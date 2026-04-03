#include "cas_evnt.h"
#include "cas_utils.h"

#include <uv.h>

#include <stdlib.h>
#include <string.h>

struct cas_evnt {
	uv_loop_t loop;
	uv_signal_t signal;
	cas_evnt_signal_cb_t signal_cb;
	void *signal_ctx;
	int signal_ready;
};

struct cas_evnt_uds {
	uv_pipe_t handle;
	cas_evnt_t *evnt;
	cas_evnt_uds_read_cb_t read_cb;
	void *ctx;
};

struct cas_evnt_uds_peer {
	cas_evnt_uds_t *uds;
	uv_pipe_t handle;
	uv_write_t write_req;
	char *buffer;
	size_t size;
	size_t capacity;
};

enum {
	CAS_EVNT_UDS_PEER_BUFFER_CAPACITY = 1024,
	CAS_EVNT_UDS_BACKLOG = 16,
};

static void cas_evnt_close_cb(uv_handle_t *handle)
{
	(void)handle;
}

static void cas_evnt_signal_cb(uv_signal_t *handle, int signum)
{
	cas_evnt_t *evnt = handle->data;

	(void)signum;
	if (evnt != NULL && evnt->signal_cb != NULL) {
		evnt->signal_cb(evnt->signal_ctx);
	}
}

cas_evnt_t *cas_evnt_create(void)
{
	cas_evnt_t *evnt = cas_alloc(sizeof(*evnt));

	if (evnt == NULL) {
		return NULL;
	}
	(void)memset(evnt, 0, sizeof(*evnt));
	if (uv_loop_init(&evnt->loop) != 0) {
		cas_free(evnt);
		return NULL;
	}

	return evnt;
}

void cas_evnt_release(cas_evnt_t *evnt)
{
	if (evnt == NULL) {
		return;
	}

	if (evnt->signal_ready) {
		cas_evnt_stop_signal(evnt);
	}
	while (uv_loop_close(&evnt->loop) != 0) {
		(void)uv_run(&evnt->loop, UV_RUN_NOWAIT);
	}
	cas_free(evnt);
}

int cas_evnt_run(cas_evnt_t *evnt, uv_run_mode mode)
{
	if (evnt == NULL) {
		return -1;
	}

	return uv_run(&evnt->loop, mode);
}

int cas_evnt_start_signal(cas_evnt_t *evnt, int signum, cas_evnt_signal_cb_t cb, void *ctx)
{
	if (evnt == NULL || cb == NULL) {
		return -1;
	}

	evnt->signal_cb = cb;
	evnt->signal_ctx = ctx;
	if (uv_signal_init(&evnt->loop, &evnt->signal) != 0) {
		return -1;
	}

	evnt->signal.data = evnt;
	if (uv_signal_start(&evnt->signal, cas_evnt_signal_cb, signum) != 0) {
		return -1;
	}

	evnt->signal_ready = 1;
	return 0;
}

void cas_evnt_stop_signal(cas_evnt_t *evnt)
{
	if (evnt == NULL || !evnt->signal_ready) {
		return;
	}
	if (uv_is_closing((const uv_handle_t *)&evnt->signal)) {
		return;
	}

	(void)uv_signal_stop(&evnt->signal);
	uv_close((uv_handle_t *)&evnt->signal, cas_evnt_close_cb);
	evnt->signal_ready = 0;
}

static void cas_evnt_uds_peer_close_cb(uv_handle_t *handle)
{
	cas_evnt_uds_peer_t *peer = handle->data;

	if (peer == NULL) {
		return;
	}

	cas_free(peer->buffer);
	cas_free(peer);
}

static void cas_evnt_uds_close_cb(uv_handle_t *handle)
{
	cas_evnt_uds_t *uds = handle->data;

	if (uds == NULL) {
		return;
	}

	cas_free(uds);
}

static void cas_evnt_uds_after_write(uv_write_t *req, int status)
{
	cas_evnt_uds_peer_t *peer = req->handle->data;

	(void)status;
	cas_free(req->data);
	if (peer != NULL && !uv_is_closing((const uv_handle_t *)&peer->handle)) {
		uv_close((uv_handle_t *)&peer->handle, cas_evnt_uds_peer_close_cb);
	}
}

static void cas_evnt_uds_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	cas_evnt_uds_peer_t *peer = handle->data;

	if (peer == NULL) {
		buf->base = NULL;
		buf->len = 0;
		return;
	}

	if (peer->capacity - peer->size < suggested_size) {
		size_t new_capacity = peer->capacity == 0 ? CAS_EVNT_UDS_PEER_BUFFER_CAPACITY : peer->capacity * 2;

		while (new_capacity - peer->size < suggested_size) {
			new_capacity *= 2;
		}

		char *new_buffer = realloc(peer->buffer, new_capacity);
		if (new_buffer == NULL) {
			buf->base = NULL;
			buf->len = 0;
			return;
		}

		peer->buffer = new_buffer;
		peer->capacity = new_capacity;
	}

	buf->base = peer->buffer + peer->size;
	buf->len = peer->capacity - peer->size;
}

static void cas_evnt_uds_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	cas_evnt_uds_peer_t *peer = stream->data;

	(void)buf;
	if (peer == NULL) {
		return;
	}
	if (nread < 0) {
		cas_evnt_uds_close_peer(peer);
		return;
	}

	peer->size += (size_t)nread;
	if (peer->uds != NULL && peer->uds->read_cb != NULL) {
		peer->uds->read_cb(peer, peer->buffer, peer->size, peer->uds->ctx);
	}
}

static void cas_evnt_uds_connection_cb(uv_stream_t *stream, int status)
{
	cas_evnt_uds_t *uds = stream->data;

	if (status != 0 || uds == NULL) {
		return;
	}

	cas_evnt_uds_peer_t *peer = cas_alloc(sizeof(*peer));
	if (peer == NULL) {
		return;
	}

	(void)memset(peer, 0, sizeof(*peer));
	peer->uds = uds;
	if (uv_pipe_init(&uds->evnt->loop, &peer->handle, 0) != 0) {
		cas_free(peer);
		return;
	}

	peer->handle.data = peer;
	if (uv_accept(stream, (uv_stream_t *)&peer->handle) != 0) {
		uv_close((uv_handle_t *)&peer->handle, cas_evnt_uds_peer_close_cb);
		return;
	}

	(void)uv_read_start((uv_stream_t *)&peer->handle, cas_evnt_uds_alloc_cb, cas_evnt_uds_read_cb);
}

cas_evnt_uds_t *cas_evnt_uds_create(cas_evnt_t *evnt, cas_evnt_uds_read_cb_t read_cb, void *ctx)
{
	if (evnt == NULL || read_cb == NULL) {
		return NULL;
	}

	cas_evnt_uds_t *uds = cas_alloc(sizeof(*uds));
	if (uds == NULL) {
		return NULL;
	}

	(void)memset(uds, 0, sizeof(*uds));
	uds->evnt = evnt;
	uds->read_cb = read_cb;
	uds->ctx = ctx;
	if (uv_pipe_init(&evnt->loop, &uds->handle, 0) != 0) {
		cas_free(uds);
		return NULL;
	}

	uds->handle.data = uds;
	return uds;
}

void cas_evnt_uds_release(cas_evnt_uds_t *uds)
{
	if (uds == NULL) {
		return;
	}

	if (uv_is_closing((const uv_handle_t *)&uds->handle)) {
		return;
	}

	uv_close((uv_handle_t *)&uds->handle, cas_evnt_uds_close_cb);
}

int cas_evnt_uds_start(cas_evnt_uds_t *uds, const char *socket_path)
{
	if (uds == NULL || socket_path == NULL) {
		return -1;
	}

	int rc = uv_pipe_bind(&uds->handle, socket_path);
	if (rc != 0) {
		return rc;
	}

	return uv_listen((uv_stream_t *)&uds->handle, CAS_EVNT_UDS_BACKLOG, cas_evnt_uds_connection_cb);
}

void cas_evnt_uds_stop(cas_evnt_uds_t *uds)
{
	if (uds == NULL) {
		return;
	}
	if (uv_is_closing((const uv_handle_t *)&uds->handle)) {
		return;
	}

	uv_close((uv_handle_t *)&uds->handle, cas_evnt_uds_close_cb);
}

int cas_evnt_uds_reply(cas_evnt_uds_peer_t *peer, const char *buffer, size_t size)
{
	if (peer == NULL || buffer == NULL) {
		return -1;
	}

	char *payload = cas_alloc(size);
	if (payload == NULL) {
		return -1;
	}

	(void)memcpy(payload, buffer, size);
	uv_buf_t buf = uv_buf_init(payload, (unsigned int)size);
	peer->write_req.data = payload;
	if (uv_write(&peer->write_req, (uv_stream_t *)&peer->handle, &buf, 1, cas_evnt_uds_after_write) != 0) {
		cas_free(payload);
		return -1;
	}

	return 0;
}

void cas_evnt_uds_close_peer(cas_evnt_uds_peer_t *peer)
{
	if (peer == NULL) {
		return;
	}
	if (uv_is_closing((const uv_handle_t *)&peer->handle)) {
		return;
	}

	uv_close((uv_handle_t *)&peer->handle, cas_evnt_uds_peer_close_cb);
}
