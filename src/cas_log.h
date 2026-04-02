#ifndef CAS_LOG_H
#define CAS_LOG_H

#include <stdint.h>
#include <stdio.h>

#if defined(__GNUC__) || defined(__clang__)
#define CAS_PRINTF_FORMAT(format_index, first_arg_index) __attribute__((format(printf, format_index, first_arg_index)))
#else
#define CAS_PRINTF_FORMAT(format_index, first_arg_index)
#endif

#define CAS_LOG_LEVEL_ERROR ((uint8_t)0)
#define CAS_LOG_LEVEL_WARN	((uint8_t)1)
#define CAS_LOG_LEVEL_INFO	((uint8_t)2)
#define CAS_LOG_LEVEL_DEBUG ((uint8_t)3)
#define CAS_LOG_LEVEL_TRACE ((uint8_t)4)

typedef struct cas_log cas_log_t;
typedef struct cas_log_category cas_log_category_t;

cas_log_t *cas_log_create(FILE *out, uint8_t level);
void cas_log_release(cas_log_t **log);
cas_log_category_t *cas_log_get_category(cas_log_t *log, const char *name);
void cas_log_output(cas_log_category_t *c, uint8_t level, const char *format, ...) CAS_PRINTF_FORMAT(3, 4);

#define cas_log_error(c, ...) cas_log_output((c), CAS_LOG_LEVEL_ERROR, __VA_ARGS__)
#define cas_log_warn(c, ...)  cas_log_output((c), CAS_LOG_LEVEL_WARN, __VA_ARGS__)
#define cas_log_info(c, ...)  cas_log_output((c), CAS_LOG_LEVEL_INFO, __VA_ARGS__)
#define cas_log_debug(c, ...) cas_log_output((c), CAS_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define cas_log_trace(c, ...) cas_log_output((c), CAS_LOG_LEVEL_TRACE, __VA_ARGS__)

#undef CAS_PRINTF_FORMAT

#endif
