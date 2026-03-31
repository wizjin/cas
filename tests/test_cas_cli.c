#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "cas_cli.h"
#include "cas_config.h"

static char *cas_test_capture_command(int argc, char **argv, int *result, int use_error_stream)
{
	FILE *out_stream;
	FILE *err_stream;
	char *buffer;
	size_t size;

	buffer = NULL;
	size = 0;
	out_stream = open_memstream(&buffer, &size);
	assert_non_null(out_stream);

	if (use_error_stream) {
		err_stream = out_stream;
	} else {
		err_stream = tmpfile();
		assert_non_null(err_stream);
	}

	*result = cas_cli_run(argc, argv, out_stream, err_stream);

	assert_int_equal(fflush(out_stream), 0);
	if (!use_error_stream) {
		assert_int_equal(fclose(err_stream), 0);
	}
	assert_int_equal(fclose(out_stream), 0);

	return buffer;
}

static void cas_cli_without_arguments_shows_help(void **state)
{
	char *argv[] = {"cas"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(1, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_non_null(strstr(output, "Usage: cas <command>"));
	assert_non_null(strstr(output, "help"));
	assert_non_null(strstr(output, "version"));

	free(output);
}

static void cas_cli_help_subcommand_shows_help(void **state)
{
	char *argv[] = {"cas", "help"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_non_null(strstr(output, "Commands:"));
	assert_non_null(strstr(output, "Show this help message."));
	assert_non_null(strstr(output, "Show the CAS version."));

	free(output);
}

static void cas_cli_version_subcommand_shows_version(void **state)
{
	char *argv[] = {"cas", "version"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_string_equal(output, CAS_VERSION "\n");

	free(output);
}

static void cas_cli_unknown_subcommand_returns_failure(void **state)
{
	char *argv[] = {"cas", "unknown"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 1);

	assert_int_equal(result, 1);
	assert_non_null(strstr(output, "Unknown command: unknown"));
	assert_non_null(strstr(output, "cas help"));

	free(output);
}

int main(void)
{
	const struct CMUnitTest cas_tests[] = {
		cmocka_unit_test(cas_cli_without_arguments_shows_help),
		cmocka_unit_test(cas_cli_help_subcommand_shows_help),
		cmocka_unit_test(cas_cli_version_subcommand_shows_version),
		cmocka_unit_test(cas_cli_unknown_subcommand_returns_failure),
	};

	return cmocka_run_group_tests(cas_tests, NULL, NULL);
}
