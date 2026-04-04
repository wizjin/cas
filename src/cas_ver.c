#include "cas_ver.h"
#include "cas_config.h"
#include "cas_utils.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
	const char *name;
	const char *match;
} cas_ver_dep_spec_t;

typedef struct {
	char paths[4][PATH_MAX];
	int found[4];
} cas_ver_dep_scan_ctx_t;

static const cas_ver_dep_spec_t cas_ver_dep_specs[] = {
	{"jemalloc", "libjemalloc.so"},
	{"libuv", "libuv.so"},
	{"llhttp", "libllhttp.so"},
	{"cJSON", "libcjson.so"},
};

static FILE *cas_ver_maps_file_open_default(const char *path, const char *mode)
{
	return fopen(path, mode);
}

static cas_ver_maps_file_open_fn_t cas_ver_maps_file_open_fn = cas_ver_maps_file_open_default;

const char *cas_ver_dep_basename(const char *path)
{
	const char *slash = strrchr(path, '/');
	if (slash == NULL) {
		return path;
	}

	return slash + 1;
}

void cas_ver_dep_version(char *ver, size_t ver_size, const char *path)
{
	const char *base = cas_ver_dep_basename(path);
	const char *so = strstr(base, ".so.");
	if (so == NULL || so[4] == '\0') {
		cas_str_copy(ver, ver_size, "unknown");
		return;
	}

	cas_str_copy(ver, ver_size, so + 4);
}

static int cas_ver_dep_matches(const char *path, size_t index)
{
	const char *base = cas_ver_dep_basename(path);
	return strstr(base, cas_ver_dep_specs[index].match) != NULL;
}

static void cas_ver_scan_deps(FILE *maps_file, cas_ver_dep_scan_ctx_t *ctx)
{
	char line[PATH_MAX * 2];

	while (fgets(line, sizeof(line), maps_file) != NULL) {
		char *path = strchr(line, '/');
		if (path == NULL) {
			continue;
		}

		char *nl = strchr(path, '\n');
		if (nl != NULL) {
			*nl = '\0';
		}

		for (size_t i = 0; i < cas_countof(cas_ver_dep_specs); i++) {
			if (!ctx->found[i] && cas_ver_dep_matches(path, i)) {
				cas_str_copy(ctx->paths[i], sizeof(ctx->paths[i]), path);
				ctx->found[i] = 1;
				break;
			}
		}
	}
}

size_t cas_ver_collect_deps(FILE *maps_file, cas_ver_dep_t *deps, size_t capacity)
{
	if (maps_file == NULL || deps == NULL || capacity == 0) {
		return 0;
	}

	cas_ver_dep_scan_ctx_t ctx = {0};
	(void)memset(&ctx, 0, sizeof(ctx));
	cas_ver_scan_deps(maps_file, &ctx);

	size_t count = 0;
	for (size_t i = 0; i < cas_countof(cas_ver_dep_specs); i++) {
		if (!ctx.found[i] || count >= capacity) {
			continue;
		}

		cas_str_copy(deps[count].name, sizeof(deps[count].name), cas_ver_dep_specs[i].name);
		cas_ver_dep_version(deps[count].version, sizeof(deps[count].version), ctx.paths[i]);
		count++;
	}

	return count;
}

void cas_ver_set_maps_open_fn(cas_ver_maps_file_open_fn_t open_fn)
{
	if (open_fn == NULL) {
		cas_ver_maps_file_open_fn = cas_ver_maps_file_open_default;
		return;
	}

	cas_ver_maps_file_open_fn = open_fn;
}

int cas_ver_show_short(const cas_cli_t *cli)
{
	cas_cli_out(cli, "%s\n", CAS_VERSION);

	return 0;
}

int cas_ver_show_details(const cas_cli_t *cli)
{
	cas_ver_dep_t deps[4];

	cas_cli_out(cli,
				"Version:      %s\n"
				"Git Commit:   %s\n"
				"Built:        %s\n"
				"OS/Arch:      %s/%s\n"
				"Dependencies:\n",
				CAS_VERSION, CAS_GIT_SHORT_COMMIT, CAS_BUILD_TIMESTAMP, CAS_TARGET_OS, CAS_TARGET_ARCH);

	size_t count = 0;
	FILE *fp = cas_ver_maps_file_open_fn("/proc/self/maps", "r");
	if (fp != NULL) {
		count = cas_ver_collect_deps(fp, deps, cas_countof(deps));
		(void)fclose(fp);
	}

	if (count == 0) {
		cas_cli_out(cli, "  - none\n");
		return 0;
	}

	for (size_t i = 0; i < count; i++) {
		cas_cli_out(cli, "  - %-8s %s\n", deps[i].name, deps[i].version);
	}

	return 0;
}
