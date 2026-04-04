#include "cas_cli.h"

int main(int argc, char **argv)
{
	const cas_cli_t ctx = {
		.out = stdout,
		.err = stderr,
		.argc = argc,
		.argv = argv,
	};
	return cas_cli_run(&ctx);
}
