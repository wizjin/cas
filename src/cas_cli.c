#include "cas_cli.h"
#include "cas_version.h"

#include <stddef.h>
#include <string.h>

typedef struct cas_cli_ctx cas_cli_ctx_t;
typedef struct {
	const char *name;
	const char *description;
	int (*run)(const cas_cli_ctx_t *ctx);
} cas_cli_cmd_t;

#define CAS_CLI_COMMAND_WIDTH ((int)(sizeof("version") - 1))

struct cas_cli_ctx {
	FILE *out;
	FILE *err;
};

static int cas_cli_run_help(const cas_cli_ctx_t *ctx);
static int cas_cli_run_short_version(const cas_cli_ctx_t *ctx);
static int cas_cli_run_version_details(const cas_cli_ctx_t *ctx);

static const cas_cli_cmd_t cas_cli_cmds[] = {
	{"help", "Show this help message.", cas_cli_run_help},
	{"version", "Show detailed build information.", cas_cli_run_version_details},
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

static int cas_cli_run_help(const cas_cli_ctx_t *ctx)
{
	(void)fprintf(ctx->out, "Usage: cas [--help] [--version] <command>\n\n");
	(void)fprintf(ctx->out, "Commands:\n");
	for (size_t i = 0; i < sizeof(cas_cli_cmds) / sizeof(cas_cli_cmds[0]); i++) {
		(void)fprintf(ctx->out, "  %-*s  %s\n", CAS_CLI_COMMAND_WIDTH, cas_cli_cmds[i].name,
					  cas_cli_cmds[i].description);
	}

	return 0;
}

static int cas_cli_run_short_version(const cas_cli_ctx_t *ctx)
{
	return cas_version_run_short(ctx->out);
}

static int cas_cli_run_version_details(const cas_cli_ctx_t *ctx)
{
	return cas_version_run_details(ctx->out);
}

int cas_cli_run(int argc, char **argv, FILE *out, FILE *err)
{
	if (argc < 2) {
		return cas_cli_run_help(&(cas_cli_ctx_t){
			.out = out,
			.err = err,
		});
	}

	if (strcmp(argv[1], "--help") == 0) {
		return cas_cli_run_help(&(cas_cli_ctx_t){
			.out = out,
			.err = err,
		});
	}

	if (strcmp(argv[1], "--version") == 0) {
		return cas_cli_run_short_version(&(cas_cli_ctx_t){
			.out = out,
			.err = err,
		});
	}

	const cas_cli_cmd_t *cmd = cas_cli_find_command(argv[1]);
	const cas_cli_ctx_t ctx = {
		.out = out,
		.err = err,
	};
	if (cmd == NULL) {
		(void)fprintf(ctx.err, "Unknown command: %s\n", argv[1]);
		(void)fprintf(ctx.err, "Run 'cas help' to see available commands.\n");
		return 1;
	}

	return cmd->run(&ctx);
}
