#ifndef CAS_CFG_H
#define CAS_CFG_H

#include <stdint.h>
#include <stdbool.h>
#include <paths.h>

#ifndef CAS_VERSION
#define CAS_VERSION "0.1.0"
#endif

#ifndef CAS_GIT_COMMIT
#define CAS_GIT_COMMIT "unknown"
#endif

#ifndef CAS_BUILD_DATETIME
#define CAS_BUILD_DATETIME "unknown"
#endif

#ifndef CAS_TARGET_OS
#define CAS_TARGET_OS "unknown"
#endif

#ifndef CAS_TARGET_ARCH
#define CAS_TARGET_ARCH "unknown"
#endif

#ifndef CAS_LOG_BUF_SIZE
#define CAS_LOG_BUF_SIZE 1024
#endif

#define CAS_UDS_SOCKET_ENV "CAS_UDS_SOCKET_PATH"

#ifdef _PATH_VARRUN
#define CAS_UDS_SKT_DEFAULT_PATH _PATH_VARRUN "cas.sock"
#else
#define CAS_UDS_SKT_DEFAULT_PATH "/var/run/cas.sock"
#endif

#endif
