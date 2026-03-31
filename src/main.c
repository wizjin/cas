#include "cas_cli.h"

int main(int argc, char **argv)
{
	return cas_cli_run(argc, argv, stdout, stderr);
}
