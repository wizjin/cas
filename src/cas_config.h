#ifndef CAS_CONFIG_H
#define CAS_CONFIG_H

#include <paths.h>

#ifndef CAS_VERSION
#define CAS_VERSION "0.1.0"
#endif

#ifndef CAS_GIT_SHORT_COMMIT
#define CAS_GIT_SHORT_COMMIT "unknown"
#endif

#ifndef CAS_BUILD_TIMESTAMP
#define CAS_BUILD_TIMESTAMP "unknown"
#endif

#ifndef CAS_TARGET_OS
#define CAS_TARGET_OS "unknown"
#endif

#ifndef CAS_TARGET_ARCH
#define CAS_TARGET_ARCH "unknown"
#endif

#ifndef CAS_LOG_BUFFER_SIZE
#define CAS_LOG_BUFFER_SIZE 1024
#endif

#define CAS_UDS_SOCKET_ENV "CAS_UDS_SOCKET_PATH"

#ifdef _PATH_VARRUN
#define CAS_UDS_SKT_DEFAULT_PATH _PATH_VARRUN "cas.sock"
#else
#define CAS_UDS_SKT_DEFAULT_PATH "/var/run/cas.sock"
#endif

#endif
