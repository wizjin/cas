#include "cas_cli.h"
#include "cas_ctrl.h"
#include "cas_version.h"

#include <stddef.h>
#include <string.h>

typedef struct {
	const char *name;
	const char *description;
	int (*run)(cas_cli_ctx_t *ctx);
} cas_cli_cmd_t;

#define CAS_CLI_COMMAND_WIDTH ((int)(sizeof("version") - 1))

static int cas_cli_run_help(cas_cli_ctx_t *ctx);
static int cas_cli_run_short_version(cas_cli_ctx_t *ctx);
static int cas_cli_run_version_details(cas_cli_ctx_t *ctx);
static int cas_cli_run_with_ctx(int argc, char **argv, cas_cli_ctx_t *ctx);

static const cas_cli_cmd_t cas_cli_cmds[] = {
	{"version", "Show this help message.", cas_cli_run_version_details},
	{"help", "Show detailed build information.", cas_cli_run_help},
	{"start", "Start the CAS service.", cas_ctrl_start},
	{"stop", "Stop the CAS service.", cas_ctrl_stop},
	{"status", "Show the CAS service status.", cas_ctrl_status},
};

static const cas_cli_cmd_t *cas_cli_find_command(const char *name)
{
	for (size_t i = 0; i < sizeof(cas_cli_cmds) / sizeof(cas_cli_cmds[0]); i++) {
		if (strcmp(cas_cli_cmds[i].name, name) == 0) {
			return &cas_cli_cmds[i];
		}
	}

	return NULL;
}

static int cas_cli_run_help(cas_cli_ctx_t *ctx)
{
	FILE *out = ctx == NULL ? NULL : ctx->out;

	if (out == NULL) {
		return 0;
	}

	(void)fputs("AI agent system.\n\n", out);
	(void)fputs("Usage:\n", out);
	(void)fputs("  cas [options] [command]\n\n", out);
	(void)fputs("Commands:\n", out);
	for (size_t i = 0; i < 2; i++) {
		(void)fprintf(out, "  %-*s  %s\n", CAS_CLI_COMMAND_WIDTH, cas_cli_cmds[i].name,
					  cas_cli_cmds[i].description);
	}
	(void)fputs("\n", out);
	(void)fputs("Options:\n", out);
	(void)fputs("  -h, --help    Show this help message.\n", out);
	(void)fputs("  -v, --version Show detailed build information.\n", out);

	return 0;
}

static int cas_cli_run_short_version(cas_cli_ctx_t *ctx)
{
	return cas_version_run_short(ctx->out);
}

static int cas_cli_run_version_details(cas_cli_ctx_t *ctx)
{
	return cas_version_run_details(ctx->out);
}

static int cas_cli_run_with_ctx(int argc, char **argv, cas_cli_ctx_t *ctx)
{
	if (argc < 2) {
		return cas_cli_run_help(ctx);
	}

	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		return cas_cli_run_help(ctx);
	}

	if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		return cas_cli_run_short_version(ctx);
	}

	const cas_cli_cmd_t *cmd = cas_cli_find_command(argv[1]);
	if (cmd == NULL) {
		cas_cli_error(ctx, "Unknown command: %s\n", argv[1]);
		cas_cli_error(ctx, "Run 'cas help' to see available commands.\n");
		return 1;
	}

	return cmd->run(ctx);
}

int cas_cli_run(int argc, char **argv, FILE *out, FILE *err)
{
	cas_cli_ctx_t ctx = {
		.out = out,
		.err = err,
	};

	return cas_cli_run_with_ctx(argc, argv, &ctx);
}
