#ifndef CAS_EVNT_H
#define CAS_EVNT_H

#include "cas_cfg.h"
#include <uv.h>

typedef void (*cas_evnt_signal_cb_t)(void *ctx);
typedef struct cas_evnt cas_evnt_t;
typedef struct cas_evnt_uds cas_evnt_uds_t;
typedef struct cas_evnt_uds_peer cas_evnt_uds_peer_t;
typedef void (*cas_evnt_uds_read_cb_t)(cas_evnt_uds_peer_t *peer, const char *buffer, size_t size, void *ctx);

cas_evnt_t *cas_evnt_create(void);
void cas_evnt_release(cas_evnt_t *evnt);
int cas_evnt_run(cas_evnt_t *evnt, uv_run_mode mode);
int cas_evnt_start_signal(cas_evnt_t *evnt, int signum, cas_evnt_signal_cb_t cb, void *ctx);
void cas_evnt_stop_signal(cas_evnt_t *evnt);

cas_evnt_uds_t *cas_evnt_uds_create(cas_evnt_t *evnt, cas_evnt_uds_read_cb_t read_cb, void *ctx);
void cas_evnt_uds_release(cas_evnt_uds_t *uds);
int cas_evnt_uds_start(cas_evnt_uds_t *uds, const char *socket_path);
void cas_evnt_uds_stop(cas_evnt_uds_t *uds);
int cas_evnt_uds_reply(cas_evnt_uds_peer_t *peer, const char *buffer, size_t size);
void cas_evnt_uds_close_peer(cas_evnt_uds_peer_t *peer);

#endif
