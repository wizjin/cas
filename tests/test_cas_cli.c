#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "cas_cli.h"
#include "cas_config.h"
#include "cas_udsp.h"
#include "cas_ver.h"
#include "cas_utils.h"

static void cas_test_reset_runtime_hooks(void)
{
	assert_int_equal(unsetenv(CAS_UDS_SOCKET_ENV), 0);
}

static char *cas_test_capture_command(int argc, char **argv, int *result, int use_error_stream)
{
	FILE *out_stream;
	FILE *err_stream;
	char *buffer;
	size_t size;
	cas_cli_t cli;

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

	cli.out = out_stream;
	cli.err = err_stream;
	cli.argc = argc;
	cli.argv = argv;
	*result = cas_cli_run(&cli);

	assert_int_equal(fflush(out_stream), 0);
	if (!use_error_stream) {
		assert_int_equal(fclose(err_stream), 0);
	}
	assert_int_equal(fclose(out_stream), 0);

	return buffer;
}

static char *cas_test_capture_version_details(int *result)
{
	FILE *out_stream;
	char *buffer;
	size_t size;

	buffer = NULL;
	size = 0;
	out_stream = open_memstream(&buffer, &size);
	assert_non_null(out_stream);

	cas_cli_t cli = {.out = out_stream, .err = out_stream, .argc = 2, .argv = NULL};

	*result = cas_ver_show_details(&cli);

	assert_int_equal(fflush(out_stream), 0);
	assert_int_equal(fclose(out_stream), 0);

	return buffer;
}

static FILE *cas_test_open_maps_file(const char *contents)
{
	FILE *maps_file;

	maps_file = tmpfile();
	assert_non_null(maps_file);
	if (contents != NULL) {
		assert_true(fputs(contents, maps_file) >= 0);
	}
	rewind(maps_file);

	return maps_file;
}

static FILE *cas_test_open_maps_file_failure(const char *path, const char *mode)
{
	(void)path;
	(void)mode;

	return NULL;
}

static void cas_cli_without_arguments_shows_help(void **state)
{
	char *argv[] = {"cas"};
	char *output;
	int result;
	const char *expected = "AI agent system.\n"
						   "\n"
						   "Usage:\n"
						   "  cas [options] [command]\n"
						   "\n"
						   "Commands:\n"
						   "  help     Show this help message.\n"
						   "  start    Start the CAS service.\n"
						   "  status   Show the CAS service status.\n"
						   "  stop     Stop the CAS service.\n"
						   "  version  Show detailed build information.\n"
						   "\n"
						   "Options:\n"
						   "  -h, --help    Show this help message.\n"
						   "  -v, --version Show detailed build information.\n";

	(void)state;

	output = cas_test_capture_command(1, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_string_equal(output, expected);

	cas_free(output);
}

static void cas_udsp_socket_path_uses_default_when_env_is_unset(void **state)
{
	(void)state;

	assert_string_equal(cas_udsp_get_skt_path(), CAS_UDS_SKT_DEFAULT_PATH);
	cas_test_reset_runtime_hooks();
}

static void cas_udsp_socket_path_uses_env_override(void **state)
{
	(void)state;

	assert_int_equal(setenv(CAS_UDS_SOCKET_ENV, "/tmp/cas-test.sock", 1), 0);
	assert_string_equal(cas_udsp_get_skt_path(), "/tmp/cas-test.sock");
	cas_test_reset_runtime_hooks();
}

static void cas_udsp_socket_path_uses_default_when_env_is_empty(void **state)
{
	(void)state;

	assert_int_equal(setenv(CAS_UDS_SOCKET_ENV, "", 1), 0);
	assert_string_equal(cas_udsp_get_skt_path(), CAS_UDS_SKT_DEFAULT_PATH);
	cas_test_reset_runtime_hooks();
}

static void cas_cli_help_subcommand_shows_help(void **state)
{
	char *argv[] = {"cas", "help"};
	char *output;
	int result;
	const char *expected = "AI agent system.\n"
						   "\n"
						   "Usage:\n"
						   "  cas [options] [command]\n"
						   "\n"
						   "Commands:\n"
						   "  help     Show this help message.\n"
						   "  start    Start the CAS service.\n"
						   "  status   Show the CAS service status.\n"
						   "  stop     Stop the CAS service.\n"
						   "  version  Show detailed build information.\n"
						   "\n"
						   "Options:\n"
						   "  -h, --help    Show this help message.\n"
						   "  -v, --version Show detailed build information.\n";

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_string_equal(output, expected);

	cas_free(output);
}

static void cas_cli_help_option_shows_help(void **state)
{
	char *argv[] = {"cas", "--help"};
	char *output;
	int result;
	const char *expected = "AI agent system.\n"
						   "\n"
						   "Usage:\n"
						   "  cas [options] [command]\n"
						   "\n"
						   "Commands:\n"
						   "  help     Show this help message.\n"
						   "  start    Start the CAS service.\n"
						   "  status   Show the CAS service status.\n"
						   "  stop     Stop the CAS service.\n"
						   "  version  Show detailed build information.\n"
						   "\n"
						   "Options:\n"
						   "  -h, --help    Show this help message.\n"
						   "  -v, --version Show detailed build information.\n";

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_string_equal(output, expected);

	cas_free(output);
}

static void cas_cli_short_help_option_shows_help(void **state)
{
	char *argv[] = {"cas", "-h"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_non_null(strstr(output, "AI agent system."));
	assert_non_null(strstr(output, "  start    Start the CAS service.\n"));
	assert_non_null(strstr(output, "  stop     Stop the CAS service.\n"));
	assert_non_null(strstr(output, "  status   Show the CAS service status.\n"));
	assert_non_null(strstr(output, "Options:\n"));

	cas_free(output);
}

static void cas_cli_short_version_option_shows_version(void **state)
{
	char *argv[] = {"cas", "--version"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_string_equal(output, CAS_VERSION "\n");

	cas_free(output);
}

static void cas_cli_short_version_alias_shows_version(void **state)
{
	char *argv[] = {"cas", "-v"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_string_equal(output, CAS_VERSION "\n");

	cas_free(output);
}

static void cas_cli_version_subcommand_shows_build_details(void **state)
{
	char *argv[] = {"cas", "version"};
	char *output;
	int result;

	(void)state;

	output = cas_test_capture_command(2, argv, &result, 0);

	assert_int_equal(result, 0);
	assert_non_null(strstr(output, "Version:      " CAS_VERSION));
	assert_non_null(strstr(output, "Git Commit:   " CAS_GIT_SHORT_COMMIT));
	assert_non_null(strstr(output, "Built:        " CAS_BUILD_TIMESTAMP));
	assert_non_null(strstr(output, "OS/Arch:      " CAS_TARGET_OS "/" CAS_TARGET_ARCH));
	assert_non_null(strstr(output, "Dependencies:\n"));
	assert_true(strstr(output, "  - none") != NULL || strstr(output, "  - jemalloc") != NULL ||
				strstr(output, "  - libuv") != NULL || strstr(output, "  - llhttp") != NULL ||
				strstr(output, "  - cJSON") != NULL);

	cas_free(output);
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

	cas_free(output);
}

static void cas_ver_dep_basename_handles_paths_without_separator(void **state)
{
	(void)state;

	assert_string_equal(cas_ver_dep_basename("libuv.so.1"), "libuv.so.1");
}

static void cas_str_copy_ignores_zero_sized_destination(void **state)
{
	char destination[8] = "keep";

	(void)state;

	cas_str_copy(destination, 0, "change");
	assert_string_equal(destination, "keep");
}

static void cas_ver_dep_version_returns_unknown_without_version_suffix(void **state)
{
	char version[CAS_VER_DEP_VER_SIZE];

	(void)state;

	cas_ver_dep_version(version, sizeof(version), "/usr/lib/libuv.so");
	assert_string_equal(version, "unknown");
}

static void cas_ver_dep_version_returns_unknown_with_empty_version_suffix(void **state)
{
	char version[CAS_VER_DEP_VER_SIZE];

	(void)state;

	cas_ver_dep_version(version, sizeof(version), "/usr/lib/libuv.so.");
	assert_string_equal(version, "unknown");
}

static void cas_ver_collect_deps_rejects_invalid_inputs(void **state)
{
	cas_ver_dep_t dependencies[1];

	(void)state;

	assert_int_equal(cas_ver_collect_deps(NULL, dependencies, 1), 0);
	assert_int_equal(cas_ver_collect_deps(stdout, NULL, 1), 0);
	assert_int_equal(cas_ver_collect_deps(stdout, dependencies, 0), 0);
}

static void cas_ver_collect_deps_limits_results_to_capacity(void **state)
{
	FILE *maps_file;
	cas_ver_dep_t dependencies[1];

	(void)state;

	maps_file = cas_test_open_maps_file("7f00-7f01 r--p 00000000 00:00 0 /usr/lib/libjemalloc.so.2\n"
										"7f01-7f02 r--p 00000000 00:00 0 /usr/lib/libuv.so.1\n");

	assert_int_equal(cas_ver_collect_deps(maps_file, dependencies, 1), 1);
	assert_string_equal(dependencies[0].name, "jemalloc");
	assert_string_equal(dependencies[0].version, "2");
	assert_int_equal(fclose(maps_file), 0);
}

static void cas_ver_collect_deps_accepts_last_line_without_newline(void **state)
{
	FILE *maps_file;
	cas_ver_dep_t dependencies[1];

	(void)state;

	maps_file = cas_test_open_maps_file("7f01-7f02 r--p 00000000 00:00 0 /usr/lib/libuv.so.1");
	assert_int_equal(cas_ver_collect_deps(maps_file, dependencies, 1), 1);
	assert_string_equal(dependencies[0].name, "libuv");
	assert_string_equal(dependencies[0].version, "1");
	assert_int_equal(fclose(maps_file), 0);
}

static void cas_ver_show_details_reports_none_when_maps_file_is_unavailable(void **state)
{
	char *output;
	int result;

	(void)state;

	cas_ver_set_maps_open_fn(cas_test_open_maps_file_failure);
	output = cas_test_capture_version_details(&result);
	cas_ver_set_maps_open_fn(NULL);

	assert_int_equal(result, 0);
	assert_non_null(strstr(output, "Dependencies:\n"));
	assert_non_null(strstr(output, "  - none\n"));

	cas_free(output);
}

int main(void)
{
	const struct CMUnitTest cas_tests[] = {
		cmocka_unit_test(cas_cli_without_arguments_shows_help),
		cmocka_unit_test(cas_udsp_socket_path_uses_default_when_env_is_unset),
		cmocka_unit_test(cas_udsp_socket_path_uses_env_override),
		cmocka_unit_test(cas_udsp_socket_path_uses_default_when_env_is_empty),
		cmocka_unit_test(cas_cli_help_subcommand_shows_help),
		cmocka_unit_test(cas_cli_help_option_shows_help),
		cmocka_unit_test(cas_cli_short_help_option_shows_help),
		cmocka_unit_test(cas_cli_short_version_option_shows_version),
		cmocka_unit_test(cas_cli_short_version_alias_shows_version),
		cmocka_unit_test(cas_cli_version_subcommand_shows_build_details),
		cmocka_unit_test(cas_cli_unknown_subcommand_returns_failure),
		cmocka_unit_test(cas_ver_dep_basename_handles_paths_without_separator),
		cmocka_unit_test(cas_str_copy_ignores_zero_sized_destination),
		cmocka_unit_test(cas_ver_dep_version_returns_unknown_without_version_suffix),
		cmocka_unit_test(cas_ver_dep_version_returns_unknown_with_empty_version_suffix),
		cmocka_unit_test(cas_ver_collect_deps_rejects_invalid_inputs),
		cmocka_unit_test(cas_ver_collect_deps_limits_results_to_capacity),
		cmocka_unit_test(cas_ver_collect_deps_accepts_last_line_without_newline),
		cmocka_unit_test(cas_ver_show_details_reports_none_when_maps_file_is_unavailable),
	};

	return cmocka_run_group_tests(cas_tests, NULL, NULL);
}
