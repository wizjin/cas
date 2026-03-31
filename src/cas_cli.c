#include "cas_cli.h"
#include "cas_config.h"

#include <stddef.h>
#include <string.h>

typedef struct cas_cli_context cas_cli_context_t;
typedef struct {
	const char *name;
	const char *description;
	int (*run)(const cas_cli_context_t *context);
} cas_cli_command_t;

struct cas_cli_context {
	FILE *out;
	FILE *err;
};

static int cas_cli_run_help(const cas_cli_context_t *context);
static int cas_cli_run_version(const cas_cli_context_t *context);

static const cas_cli_command_t cas_cli_commands[] = {
	{"help", "Show this help message.", cas_cli_run_help},
	{"version", "Show the CAS version.", cas_cli_run_version},
};

static const cas_cli_command_t *cas_cli_find_command(const char *name)
{
	for (size_t index = 0; index < sizeof(cas_cli_commands) / sizeof(cas_cli_commands[0]); index++) {
		if (strcmp(cas_cli_commands[index].name, name) == 0) {
			return &cas_cli_commands[index];
		}
	}

	return NULL;
}

static int cas_cli_run_help(const cas_cli_context_t *context)
{
	(void)fprintf(context->out, "Usage: cas <command>\n\n");
	(void)fprintf(context->out, "Commands:\n");
	for (size_t index = 0; index < sizeof(cas_cli_commands) / sizeof(cas_cli_commands[0]); index++) {
		(void)fprintf(context->out, "  %s\t%s\n", cas_cli_commands[index].name, cas_cli_commands[index].description);
	}

	return 0;
}

static int cas_cli_run_version(const cas_cli_context_t *context)
{
	(void)fprintf(context->out, "%s\n", CAS_VERSION);

	return 0;
}

int cas_cli_run(int argc, char **argv, FILE *out, FILE *err)
{
	cas_cli_context_t context = {
		.out = out,
		.err = err,
	};

	if (argc < 2) {
		return cas_cli_run_help(&context);
	}

	const cas_cli_command_t *command = cas_cli_find_command(argv[1]);
	if (command == NULL) {
		(void)fprintf(context.err, "Unknown command: %s\n", argv[1]);
		(void)fprintf(context.err, "Run 'cas help' to see available commands.\n");
		return 1;
	}

	return command->run(&context);
}
