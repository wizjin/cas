#include "cas_cli.h"
#include "cas_ctrl.h"
#include "cas_utils.h"
#include "cas_ver.h"

#include <stddef.h>
#include <string.h>
#include <assert.h>

typedef struct {
	const char *name;
	const char *desc;
	int (*exec)(const cas_cli_t *cli);
} cas_cli_cmd_t;

#define CAS_CLI_CMD_WIDTH ((int)(sizeof("version") - 1))

static int cas_cli_show_help(const cas_cli_t *cli);

static const cas_cli_cmd_t cas_cli_cmds[] = {
	{"help", "Show this help message.", cas_cli_show_help},
	{"start", "Start the CAS service.", cas_ctrl_start},
	{"status", "Show the CAS service status.", cas_ctrl_status},
	{"stop", "Stop the CAS service.", cas_ctrl_stop},
	{"version", "Show detailed build information.", cas_ver_show_details},
};

static const cas_cli_cmd_t *cas_cli_find_cmd(const char *name)
{
	for (int i = 0; i < cas_countof(cas_cli_cmds); i++) {
		if (strcmp(cas_cli_cmds[i].name, name) == 0) {
			return cas_cli_cmds + i;
		}
	}

	return NULL;
}

static int cas_cli_show_help(const cas_cli_t *cli)
{
	assert(cli != NULL);
	cas_cli_out(cli, "AI agent system.\n\n"
					 "Usage:\n"
					 "  cas [options] [command]\n\n"
					 "Commands:\n");
	for (int i = 0; i < cas_countof(cas_cli_cmds); i++) {
		cas_cli_out(cli, "  %-*s  %s\n", CAS_CLI_CMD_WIDTH, cas_cli_cmds[i].name, cas_cli_cmds[i].desc);
	}
	cas_cli_out(cli, "\n"
					 "Options:\n"
					 "  -h, --help    Show this help message.\n"
					 "  -v, --version Show detailed build information.\n");
	return 0;
}

int cas_cli_run(const cas_cli_t *cli)
{
	assert(cli != NULL);
	if (cli->argc < 2) {
		return cas_cli_show_help(cli);
	}

	const char *c = cli->argv[1];
	if (strcmp(c, "--help") == 0 || strcmp(c, "-h") == 0) {
		return cas_cli_show_help(cli);
	}

	if (strcmp(c, "--version") == 0 || strcmp(c, "-v") == 0) {
		return cas_ver_show_short(cli);
	}

	const cas_cli_cmd_t *cmd = cas_cli_find_cmd(c);
	if (cmd == NULL) {
		cas_cli_err(cli,
					"Unknown command: %s\n"
					"Run 'cas help' to see available commands.\n",
					c);
		return 1;
	}

	return cmd->exec(cli);
}
