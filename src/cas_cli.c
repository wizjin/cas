#include "cas_cli.h"
#include "cas_version.h"

#include <stddef.h>
#include <string.h>

typedef struct cas_cli_context cas_cli_context_t;
typedef struct {
	const char *name;
	const char *description;
	int (*run)(const cas_cli_context_t *context);
} cas_cli_command_t;

#define CAS_CLI_COMMAND_WIDTH ((int)(sizeof("version") - 1))

struct cas_cli_context {
	FILE *out;
	FILE *err;
};

static int cas_cli_run_help(const cas_cli_context_t *context);
static int cas_cli_run_short_version(const cas_cli_context_t *context);
static int cas_cli_run_version_details(const cas_cli_context_t *context);

static const cas_cli_command_t cas_cli_commands[] = {
	{"help", "Show this help message.", cas_cli_run_help},
	{"version", "Show detailed build information.", cas_cli_run_version_details},
};

static const cas_cli_command_t *cas_cli_find_command(const char *name)
{
	for (size_t i = 0; i < sizeof(cas_cli_commands) / sizeof(cas_cli_commands[0]); i++) {
		if (strcmp(cas_cli_commands[i].name, name) == 0) {
			return &cas_cli_commands[i];
		}
	}

	return NULL;
}

static int cas_cli_run_help(const cas_cli_context_t *context)
{
	(void)fprintf(context->out, "Usage: cas [--help] [--version] <command>\n\n");
	(void)fprintf(context->out, "Commands:\n");
	for (size_t i = 0; i < sizeof(cas_cli_commands) / sizeof(cas_cli_commands[0]); i++) {
		(void)fprintf(context->out, "  %-*s  %s\n", CAS_CLI_COMMAND_WIDTH, cas_cli_commands[i].name,
					  cas_cli_commands[i].description);
	}

	return 0;
}

static int cas_cli_run_short_version(const cas_cli_context_t *context)
{
	return cas_version_run_short(context->out);
}

static int cas_cli_run_version_details(const cas_cli_context_t *context)
{
	return cas_version_run_details(context->out);
}

int cas_cli_run(int argc, char **argv, FILE *out, FILE *err)
{
	if (argc < 2) {
		return cas_cli_run_help(&(cas_cli_context_t){
			.out = out,
			.err = err,
		});
	}

	if (strcmp(argv[1], "--help") == 0) {
		return cas_cli_run_help(&(cas_cli_context_t){
			.out = out,
			.err = err,
		});
	}

	if (strcmp(argv[1], "--version") == 0) {
		return cas_cli_run_short_version(&(cas_cli_context_t){
			.out = out,
			.err = err,
		});
	}

	const cas_cli_command_t *cmd = cas_cli_find_command(argv[1]);
	const cas_cli_context_t ctx = {
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
