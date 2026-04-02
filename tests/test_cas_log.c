#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmocka.h>

#include "cas_log.h"
#include "../src/cas_log.c"

#define CAS_TEST_REAL_STRFTIME	   ((size_t) - 2)
#define CAS_TEST_STRFTIME_OVERFLOW ((size_t) - 1)
#define CAS_TEST_REAL_VSNPRINTF	   (INT32_MIN)

typedef struct {
	cas_log_category_t *category;
	int thread_id;
	int message_count;
} cas_log_thread_args_t;

static int cas_test_wrap_malloc;
static int cas_test_wrap_mutex_init;
static int cas_test_wrap_mutex_lock;
static int cas_test_wrap_clock_gettime;
static int cas_test_wrap_localtime_r;
static int cas_test_wrap_strftime;
static int cas_test_wrap_vsnprintf;

void *__real_malloc(size_t size);
int __real_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int __real_pthread_mutex_lock(pthread_mutex_t *mutex);
int __real_clock_gettime(clockid_t clock_id, struct timespec *ts);
struct tm *__real_localtime_r(const time_t *timer, struct tm *result);
size_t __real_strftime(char *dst, size_t size, const char *fmt, const struct tm *tm);
int __real_vsnprintf(char *dst, size_t size, const char *fmt, va_list args);

void *__wrap_malloc(size_t size);
int __wrap_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int __wrap_pthread_mutex_lock(pthread_mutex_t *mutex);
int __wrap_clock_gettime(clockid_t clock_id, struct timespec *ts);
struct tm *__wrap_localtime_r(const time_t *timer, struct tm *result);
size_t __wrap_strftime(char *dst, size_t size, const char *fmt, const struct tm *tm);
int __wrap_vsnprintf(char *dst, size_t size, const char *fmt, va_list args);

void *__wrap_malloc(size_t size)
{
	if (!cas_test_wrap_malloc) {
		return __real_malloc(size);
	}

	return (void *)(uintptr_t)mock_type(uintptr_t);
}

int __wrap_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	if (!cas_test_wrap_mutex_init) {
		return __real_pthread_mutex_init(mutex, attr);
	}

	return mock_type(int);
}

int __wrap_pthread_mutex_lock(pthread_mutex_t *mutex)
{
	if (!cas_test_wrap_mutex_lock) {
		return __real_pthread_mutex_lock(mutex);
	}

	return mock_type(int);
}

int __wrap_clock_gettime(clockid_t clock_id, struct timespec *ts)
{
	if (!cas_test_wrap_clock_gettime) {
		return __real_clock_gettime(clock_id, ts);
	}

	return mock_type(int);
}

struct tm *__wrap_localtime_r(const time_t *timer, struct tm *result)
{
	if (!cas_test_wrap_localtime_r) {
		return __real_localtime_r(timer, result);
	}

	return mock_type(struct tm *);
}

size_t __wrap_strftime(char *dst, size_t size, const char *fmt, const struct tm *tm)
{
	if (!cas_test_wrap_strftime) {
		return __real_strftime(dst, size, fmt, tm);
	}

	size_t value = mock_type(size_t);
	if (value == CAS_TEST_REAL_STRFTIME) {
		return __real_strftime(dst, size, fmt, tm);
	}
	if (value == CAS_TEST_STRFTIME_OVERFLOW) {
		return size;
	}

	return value;
}

int __wrap_vsnprintf(char *dst, size_t size, const char *fmt, va_list args)
{
	if (!cas_test_wrap_vsnprintf) {
		return __real_vsnprintf(dst, size, fmt, args);
	}

	int value = mock_type(int);
	if (value == CAS_TEST_REAL_VSNPRINTF) {
		return __real_vsnprintf(dst, size, fmt, args);
	}

	return value;
}

static int cas_test_setup(void **state)
{
	(void)state;

	cas_test_wrap_malloc = 0;
	cas_test_wrap_mutex_init = 0;
	cas_test_wrap_mutex_lock = 0;
	cas_test_wrap_clock_gettime = 0;
	cas_test_wrap_localtime_r = 0;
	cas_test_wrap_strftime = 0;
	cas_test_wrap_vsnprintf = 0;

	return 0;
}

static int cas_test_teardown(void **state)
{
	(void)state;

	return 0;
}

static char *cas_test_log_snapshot(FILE *stream)
{
	long position;
	char *buffer;
	size_t size;

	assert_non_null(stream);
	assert_int_equal(fflush(stream), 0);
	position = ftell(stream);
	assert_true(position >= 0);
	size = (size_t)position;
	assert_int_equal(fseek(stream, 0, SEEK_SET), 0);

	buffer = malloc(size + 1);
	assert_non_null(buffer);
	if (size > 0) {
		assert_int_equal((int)fread(buffer, 1, size, stream), (int)size);
	}
	buffer[size] = '\0';
	assert_int_equal(fseek(stream, 0, SEEK_END), 0);

	return buffer;
}

static void cas_log_create_and_release_manage_handle(void **state)
{
	FILE *stream;
	cas_log_t *log;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);

	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);

	cas_log_release(&log);
	assert_null(log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_create_rejects_invalid_inputs(void **state)
{
	(void)state;

	assert_null(cas_log_create(NULL, CAS_LOG_LEVEL_INFO));
	assert_null(cas_log_create(stdout, (uint8_t)99));

	cas_test_wrap_malloc = 1;
	will_return(__wrap_malloc, (uintptr_t)NULL);
	assert_null(cas_log_create(stdout, CAS_LOG_LEVEL_INFO));

	cas_test_wrap_malloc = 0;
	cas_test_wrap_mutex_init = 1;
	will_return(__wrap_pthread_mutex_init, -1);
	assert_null(cas_log_create(stdout, CAS_LOG_LEVEL_INFO));
}

static void cas_log_release_ignores_null_inputs(void **state)
{
	(void)state;

	cas_log_release(NULL);

	cas_log_t *log = NULL;
	cas_log_release(&log);
	assert_null(log);
}

static void cas_log_get_category_reuses_existing_name(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *first;
	cas_log_category_t *second;
	cas_log_category_t *third;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);

	first = cas_log_get_category(log, "core");
	second = cas_log_get_category(log, "core");
	third = cas_log_get_category(log, "net");

	assert_non_null(first);
	assert_ptr_equal(first, second);
	assert_non_null(third);
	assert_ptr_not_equal(first, third);

	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_get_category_normalizes_name_to_fixed_width(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *short_name;
	cas_log_category_t *short_name_again;
	cas_log_category_t *long_name;
	cas_log_category_t *long_name_again;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);

	short_name = cas_log_get_category(log, "net");
	short_name_again = cas_log_get_category(log, "net ");
	long_name = cas_log_get_category(log, "trace");
	long_name_again = cas_log_get_category(log, "trac");

	assert_non_null(short_name);
	assert_ptr_equal(short_name, short_name_again);
	assert_memory_equal(short_name->name, "net ", 4);

	assert_non_null(long_name);
	assert_ptr_equal(long_name, long_name_again);
	assert_memory_equal(long_name->name, "trac", 4);

	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_get_category_rejects_invalid_inputs(void **state)
{
	FILE *stream;
	cas_log_t *log;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_INFO);
	assert_non_null(log);

	assert_null(cas_log_get_category(NULL, "core"));
	assert_null(cas_log_get_category(log, NULL));

	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_get_category_handles_lock_and_alloc_failures(void **state)
{
	FILE *stream;
	cas_log_t *log;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_INFO);
	assert_non_null(log);

	cas_test_wrap_mutex_lock = 1;
	will_return(__wrap_pthread_mutex_lock, -1);
	assert_null(cas_log_get_category(log, "core"));

	cas_test_wrap_mutex_lock = 0;
	cas_test_wrap_malloc = 1;
	will_return(__wrap_malloc, (uintptr_t)NULL);
	assert_null(cas_log_get_category(log, "core"));

	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_writes_expected_format(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;
	size_t length;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_log_info(category, "hello %s", "world");
	output = cas_test_log_snapshot(stream);
	length = strlen(output);

	assert_true(length > 38);
	assert_int_equal(output[4], '-');
	assert_int_equal(output[7], '-');
	assert_int_equal(output[10], 'T');
	assert_int_equal(output[13], ':');
	assert_int_equal(output[16], ':');
	assert_int_equal(output[19], '.');
	assert_true(output[23] == '+' || output[23] == '-');
	assert_non_null(strstr(output, " I/core - hello world\n"));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_filters_by_level(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_WARN);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_log_info(category, "hidden");
	cas_log_warn(category, "shown");
	output = cas_test_log_snapshot(stream);

	assert_null(strstr(output, "hidden"));
	assert_non_null(strstr(output, " W/core - shown\n"));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_uses_fixed_width_category_name(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "net");
	assert_non_null(category);

	cas_log_info(category, "shown");
	output = cas_test_log_snapshot(stream);

	assert_non_null(strstr(output, " I/net  - shown\n"));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_truncates_long_message(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char message[1500];
	char *output;
	size_t length;

	(void)state;

	(void)memset(message, 'a', sizeof(message) - 1);
	message[sizeof(message) - 1] = '\0';
	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_log_trace(category, "%s", message);
	output = cas_test_log_snapshot(stream);
	length = strlen(output);

	assert_true(length <= 1023);
	assert_true(length > 0);
	assert_int_equal(output[length - 1], '\n');
	assert_non_null(strstr(output, " T/core - "));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_handles_null_inputs(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_log_output(NULL, CAS_LOG_LEVEL_INFO, "skip");
	cas_log_output(category, CAS_LOG_LEVEL_INFO, NULL);
	output = cas_test_log_snapshot(stream);
	assert_string_equal(output, "");

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_uses_unknown_level_marker(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_INFO);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_log_output(category, (uint8_t)99, "unknown");
	output = cas_test_log_snapshot(stream);
	assert_non_null(strstr(output, " N/core - unknown\n"));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_handles_prefix_failures(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_test_wrap_clock_gettime = 1;
	will_return(__wrap_clock_gettime, -1);
	cas_log_info(category, "clock");

	cas_test_wrap_clock_gettime = 0;
	cas_test_wrap_localtime_r = 1;
	will_return(__wrap_localtime_r, NULL);
	cas_log_info(category, "localtime");

	cas_test_wrap_localtime_r = 0;
	cas_test_wrap_strftime = 1;
	will_return(__wrap_strftime, (size_t)0);
	cas_log_info(category, "strftime-zero");

	cas_test_wrap_strftime = 1;
	will_return(__wrap_strftime, CAS_TEST_REAL_STRFTIME);
	will_return(__wrap_strftime, CAS_TEST_STRFTIME_OVERFLOW);
	cas_log_info(category, "strftime-overflow");

	cas_test_wrap_strftime = 0;

	output = cas_test_log_snapshot(stream);
	assert_string_equal(output, "");

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_format_prefix_rejects_too_small_buffer_for_milliseconds(void **state)
{
	char buffer[23];

	(void)state;

	cas_test_wrap_strftime = 1;
	will_return(__wrap_strftime, (size_t)19);

	assert_int_equal(cas_log_format_prefix(buffer, sizeof(buffer), CAS_LOG_LEVEL_INFO, "core"), 0);
}

static void cas_log_format_prefix_rejects_too_small_buffer_for_category(void **state)
{
	char buffer[33];

	(void)state;

	cas_test_wrap_strftime = 1;
	will_return(__wrap_strftime, (size_t)19);
	will_return(__wrap_strftime, (size_t)5);

	assert_int_equal(cas_log_format_prefix(buffer, sizeof(buffer), CAS_LOG_LEVEL_INFO, "core"), 0);
}

static void cas_log_format_prefix_rejects_first_strftime_overflow(void **state)
{
	char buffer[64];

	(void)state;

	cas_test_wrap_strftime = 1;
	will_return(__wrap_strftime, CAS_TEST_STRFTIME_OVERFLOW);

	assert_int_equal(cas_log_format_prefix(buffer, sizeof(buffer), CAS_LOG_LEVEL_INFO, "core"), 0);
}

static void cas_log_output_handles_log_pointer_and_message_failures(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	struct cas_log_category fake = {0};
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	fake.log = NULL;
	cas_log_output(&fake, CAS_LOG_LEVEL_INFO, "skip");

	cas_test_wrap_vsnprintf = 1;
	will_return(__wrap_vsnprintf, -1);
	cas_log_info(category, "message-fail");

	output = cas_test_log_snapshot(stream);
	assert_string_equal(output, "");

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_keeps_existing_newline(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_log_info(category, "line\n");
	output = cas_test_log_snapshot(stream);
	assert_non_null(strstr(output, " I/core - line\n"));
	assert_null(strstr(output, "line\n\n"));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_keeps_existing_newline_with_direct_call(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_log_output(category, CAS_LOG_LEVEL_INFO, "%s", "line\n");
	output = cas_test_log_snapshot(stream);
	assert_non_null(strstr(output, " I/core - line\n"));
	assert_null(strstr(output, "line\n\n"));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void cas_log_output_handles_write_lock_failure(void **state)
{
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	char *output;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_TRACE);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	cas_test_wrap_mutex_lock = 1;
	will_return(__wrap_pthread_mutex_lock, -1);
	cas_log_info(category, "skip");
	output = cas_test_log_snapshot(stream);
	assert_string_equal(output, "");

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

static void *cas_log_thread_run(void *opaque)
{
	cas_log_thread_args_t *args;
	int index;

	args = opaque;
	for (index = 0; index < args->message_count; index++) {
		cas_log_info(args->category, "thread-%d message-%d", args->thread_id, index);
	}

	return NULL;
}

static void cas_log_output_is_thread_safe(void **state)
{
	enum { cas_thread_count = 4, cas_message_count = 40 };
	FILE *stream;
	cas_log_t *log;
	cas_log_category_t *category;
	pthread_t threads[cas_thread_count];
	cas_log_thread_args_t args[cas_thread_count];
	char *output;
	char *saveptr;
	char *line;
	int index;
	int line_count;

	(void)state;

	stream = tmpfile();
	assert_non_null(stream);
	log = cas_log_create(stream, CAS_LOG_LEVEL_INFO);
	assert_non_null(log);
	category = cas_log_get_category(log, "core");
	assert_non_null(category);

	for (index = 0; index < cas_thread_count; index++) {
		args[index].category = category;
		args[index].thread_id = index;
		args[index].message_count = cas_message_count;
		assert_int_equal(pthread_create(&threads[index], NULL, cas_log_thread_run, &args[index]), 0);
	}

	for (index = 0; index < cas_thread_count; index++) {
		assert_int_equal(pthread_join(threads[index], NULL), 0);
	}

	output = cas_test_log_snapshot(stream);
	line_count = 0;
	saveptr = NULL;
	line = strtok_r(output, "\n", &saveptr);
	while (line != NULL) {
		assert_non_null(strstr(line, " I/core - thread-"));
		assert_null(strstr(line, "\n"));
		line_count++;
		line = strtok_r(NULL, "\n", &saveptr);
	}

	assert_int_equal(line_count, cas_thread_count * cas_message_count);

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

int main(void)
{
	const struct CMUnitTest cas_tests[] = {
		cmocka_unit_test_setup_teardown(cas_log_create_and_release_manage_handle, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_create_rejects_invalid_inputs, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_release_ignores_null_inputs, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_get_category_reuses_existing_name, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_get_category_normalizes_name_to_fixed_width, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_get_category_rejects_invalid_inputs, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_get_category_handles_lock_and_alloc_failures, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_writes_expected_format, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_filters_by_level, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_uses_fixed_width_category_name, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_truncates_long_message, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_handles_null_inputs, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_uses_unknown_level_marker, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_handles_prefix_failures, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_format_prefix_rejects_too_small_buffer_for_milliseconds, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_format_prefix_rejects_too_small_buffer_for_category, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_format_prefix_rejects_first_strftime_overflow, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_handles_log_pointer_and_message_failures, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_keeps_existing_newline, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_keeps_existing_newline_with_direct_call, cas_test_setup,
										cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_handles_write_lock_failure, cas_test_setup, cas_test_teardown),
		cmocka_unit_test_setup_teardown(cas_log_output_is_thread_safe, cas_test_setup, cas_test_teardown),
	};

	return cmocka_run_group_tests(cas_tests, NULL, NULL);
}
