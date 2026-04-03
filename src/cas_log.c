#include "cas_log.h"
#include "cas_config.h"
#include "cas_utils.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#define CAS_LOG_CATEGORY_NAME_SIZE			4
#define CAS_LOG_MILLISECONDS_DIGITS			3
#define CAS_LOG_PREFIX_SUFFIX_SIZE			6
#define CAS_LOG_DECIMAL_BASE				10L
#define CAS_LOG_MILLISECONDS_HUNDREDS		100L
#define CAS_LOG_NANOSECONDS_PER_MILLISECOND 1000000L

struct cas_log_category {
	cas_log_t *log;
	cas_log_category_t *next;
	char name[CAS_LOG_CATEGORY_NAME_SIZE];
};

struct cas_log {
	FILE *out;
	atomic_uint_least8_t level;
	pthread_mutex_t mutex;
	cas_log_category_t *categories;
};

static int cas_log_lock(cas_log_t *log)
{
	return pthread_mutex_lock(&log->mutex) == 0;
}

static void cas_log_unlock(cas_log_t *log)
{
	(void)pthread_mutex_unlock(&log->mutex);
}

static char cas_log_level_char(uint8_t level)
{
	static const char level_chars[] = {'E', 'W', 'I', 'D', 'T'};

	if (level > CAS_LOG_LEVEL_TRACE) {
		return 'N';
	}

	return level_chars[level];
}

static size_t cas_log_format_prefix(char *buffer, size_t size, uint8_t level,
									const char name[CAS_LOG_CATEGORY_NAME_SIZE])
{
	struct timespec ts = {0};
	struct tm tm = {0};

	if (clock_gettime(CLOCK_REALTIME, &ts) != 0 || localtime_r(&ts.tv_sec, &tm) == NULL) {
		return 0;
	}

	size_t len = strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", &tm);
	if (len == 0 || len >= size) {
		return 0;
	}

	if (size - len <= CAS_LOG_MILLISECONDS_DIGITS + 1) {
		return 0;
	}
	long ms = ts.tv_nsec / CAS_LOG_NANOSECONDS_PER_MILLISECOND;
	buffer[len] = '.';
	buffer[len + 1] = (char)('0' + (ms / CAS_LOG_MILLISECONDS_HUNDREDS));
	buffer[len + 2] = (char)('0' + ((ms / CAS_LOG_DECIMAL_BASE) % CAS_LOG_DECIMAL_BASE));
	buffer[len + 3] = (char)('0' + (ms % CAS_LOG_DECIMAL_BASE));
	len += CAS_LOG_MILLISECONDS_DIGITS + 1;
	buffer[len] = '\0';

	len += strftime(buffer + len, size - len, "%z", &tm);
	if (len >= size) {
		return 0;
	}

	if (size - len <= CAS_LOG_CATEGORY_NAME_SIZE + CAS_LOG_PREFIX_SUFFIX_SIZE) {
		return 0;
	}
	buffer[len++] = ' ';
	buffer[len++] = cas_log_level_char(level);
	buffer[len++] = '/';
	(void)memcpy(buffer + len, name, CAS_LOG_CATEGORY_NAME_SIZE);
	len += CAS_LOG_CATEGORY_NAME_SIZE;
	buffer[len++] = ' ';
	buffer[len++] = '-';
	buffer[len++] = ' ';
	buffer[len] = '\0';

	return len;
}

cas_log_t *cas_log_create(FILE *out, uint8_t level)
{
	if (out == NULL || level > CAS_LOG_LEVEL_TRACE) {
		return NULL;
	}

	cas_log_t *log = cas_alloc(sizeof(*log));
	if (log == NULL) {
		return NULL;
	}

	log->out = out;
	atomic_init(&log->level, level);
	log->categories = NULL;
	if (pthread_mutex_init(&log->mutex, NULL) != 0) {
		cas_free(log);
		return NULL;
	}

	return log;
}

void cas_log_release(cas_log_t **log)
{
	if (log == NULL || *log == NULL) {
		return;
	}

	for (cas_log_category_t *category = (*log)->categories; category != NULL;) {
		cas_log_category_t *next = category->next;
		cas_free(category);
		category = next;
	}

	(void)pthread_mutex_destroy(&(*log)->mutex);
	cas_free(*log);
	*log = NULL;
}

cas_log_category_t *cas_log_get_category(cas_log_t *log, const char *name)
{
	char cat_name[CAS_LOG_CATEGORY_NAME_SIZE];
	size_t idx = 0;

	if (log == NULL || name == NULL) {
		return NULL;
	}

	(void)memset(cat_name, ' ', sizeof(cat_name));
	while (idx < sizeof(cat_name) && name[idx] != '\0') {
		cat_name[idx] = name[idx];
		idx++;
	}

	if (!cas_log_lock(log)) {
		return NULL;
	}

	cas_log_category_t *cat = NULL;
	for (cas_log_category_t *it = log->categories; it != NULL; it = it->next) {
		if (memcmp(it->name, cat_name, sizeof(cat_name)) == 0) {
			cat = it;
			break;
		}
	}

	if (cat == NULL) {
		cas_log_category_t *new_cat = cas_alloc(sizeof(*new_cat));
		if (new_cat != NULL) {
			new_cat->log = log;
			new_cat->next = log->categories;
			(void)memcpy(new_cat->name, cat_name, sizeof(cat_name));
			log->categories = new_cat;
			cat = new_cat;
		}
	}

	cas_log_unlock(log);

	return cat;
}

void cas_log_output(cas_log_category_t *c, uint8_t level, const char *format, ...)
{
	char msg[CAS_LOG_BUFFER_SIZE];

	if (c == NULL || format == NULL) {
		return;
	}

	cas_log_t *log = c->log;
	if (log == NULL) {
		return;
	}

	uint8_t max = (uint8_t)atomic_load_explicit(&log->level, memory_order_relaxed);
	if (level <= CAS_LOG_LEVEL_TRACE && level > max) {
		return;
	}

	size_t len = cas_log_format_prefix(msg, sizeof(msg), level, c->name);
	if (len == 0) {
		return;
	}

	if (len < sizeof(msg) - 1) {
		va_list args;

		va_start(args, format);
		int n = vsnprintf(msg + len, sizeof(msg) - len, format, args);
		va_end(args);
		if (n < 0) {
			return;
		}

		if ((size_t)n >= sizeof(msg) - len) {
			len = sizeof(msg) - 1;
		} else {
			len += (size_t)n;
		}
	}

	if (len >= sizeof(msg) - 1) {
		msg[sizeof(msg) - 2] = '\n';
		msg[sizeof(msg) - 1] = '\0';
		len = sizeof(msg) - 1;
	} else if (msg[len - 1] != '\n') {
		msg[len] = '\n';
		msg[len + 1] = '\0';
		len++;
	}

	if (cas_log_lock(log)) {
		(void)fwrite(msg, 1, len, log->out);
		cas_log_unlock(log);
	}
}
