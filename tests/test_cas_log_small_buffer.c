#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#define CAS_LOG_BUFFER_SIZE 39

#include "cas_log.h"
#include "../src/cas_log.c"

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

static void cas_log_output_truncates_when_prefix_fills_buffer(void **state)
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

	cas_log_info(category, "message is ignored");
	output = cas_test_log_snapshot(stream);
	length = strlen(output);

	assert_int_equal(length, CAS_LOG_BUFFER_SIZE - 1);
	assert_int_equal(output[length - 1], '\n');
	assert_null(strstr(output, "message is ignored"));

	free(output);
	cas_log_release(&log);
	assert_int_equal(fclose(stream), 0);
}

int main(void)
{
	const struct CMUnitTest cas_tests[] = {
		cmocka_unit_test(cas_log_output_truncates_when_prefix_fills_buffer),
	};

	return cmocka_run_group_tests(cas_tests, NULL, NULL);
}
