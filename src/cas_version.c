#include "cas_version.h"
#include "cas_config.h"
#include "cas_utils.h"

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
	const char *name;
	const char *match;
} cas_version_dependency_spec_t;

typedef struct {
	char paths[4][PATH_MAX];
	int found[4];
} cas_version_dependency_scan_context_t;

static const cas_version_dependency_spec_t cas_version_dependency_specs[] = {
	{"jemalloc", "libjemalloc.so"},
	{"libuv", "libuv.so"},
	{"llhttp", "libllhttp.so"},
	{"cJSON", "libcjson.so"},
};

static FILE *cas_version_maps_file_open_default(const char *path, const char *mode)
{
	return fopen(path, mode);
}

static cas_version_maps_file_open_fn_t cas_version_maps_file_open_fn = cas_version_maps_file_open_default;

const char *cas_version_dependency_basename(const char *path)
{
	const char *slash = strrchr(path, '/');
	if (slash == NULL) {
		return path;
	}

	return slash + 1;
}

void cas_version_extract_dependency_version(char *version, size_t version_size, const char *path)
{
	const char *base = cas_version_dependency_basename(path);
	const char *so = strstr(base, ".so.");
	if (so == NULL || so[4] == '\0') {
		cas_utils_copy_string(version, version_size, "unknown");
		return;
	}

	cas_utils_copy_string(version, version_size, so + 4);
}

static int cas_version_dependency_matches(const char *path, size_t index)
{
	const char *base = cas_version_dependency_basename(path);
	return strstr(base, cas_version_dependency_specs[index].match) != NULL;
}

static void cas_version_scan_dependencies(FILE *maps_file, cas_version_dependency_scan_context_t *context)
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

		for (size_t i = 0; i < sizeof(cas_version_dependency_specs) / sizeof(cas_version_dependency_specs[0]);
			 i++) {
			if (!context->found[i] && cas_version_dependency_matches(path, i)) {
				cas_utils_copy_string(context->paths[i], sizeof(context->paths[i]), path);
				context->found[i] = 1;
				break;
			}
		}
	}
}

size_t cas_version_collect_dependencies_from_file(FILE *maps_file, cas_version_dependency_t *dependencies,
												  size_t capacity)
{
	if (maps_file == NULL || dependencies == NULL || capacity == 0) {
		return 0;
	}

	cas_version_dependency_scan_context_t ctx;
	(void)memset(&ctx, 0, sizeof(ctx));
	cas_version_scan_dependencies(maps_file, &ctx);

	size_t count = 0;
	for (size_t i = 0; i < sizeof(cas_version_dependency_specs) / sizeof(cas_version_dependency_specs[0]); i++) {
		if (!ctx.found[i] || count >= capacity) {
			continue;
		}

		cas_utils_copy_string(dependencies[count].name, sizeof(dependencies[count].name),
							  cas_version_dependency_specs[i].name);
		cas_version_extract_dependency_version(dependencies[count].version, sizeof(dependencies[count].version),
											   ctx.paths[i]);
		count++;
	}

	return count;
}

void cas_version_set_maps_file_open_fn(cas_version_maps_file_open_fn_t open_fn)
{
	if (open_fn == NULL) {
		cas_version_maps_file_open_fn = cas_version_maps_file_open_default;
		return;
	}

	cas_version_maps_file_open_fn = open_fn;
}

int cas_version_run_short(FILE *out)
{
	(void)fprintf(out, "%s\n", CAS_VERSION);

	return 0;
}

int cas_version_run_details(FILE *out)
{
	cas_version_dependency_t deps[4];

	(void)fprintf(out, "Version:      %s\n", CAS_VERSION);
	(void)fprintf(out, "Git Commit:   %s\n", CAS_GIT_SHORT_COMMIT);
	(void)fprintf(out, "Built:        %s\n", CAS_BUILD_TIMESTAMP);
	(void)fprintf(out, "OS/Arch:      %s/%s\n", CAS_TARGET_OS, CAS_TARGET_ARCH);
	(void)fprintf(out, "Dependencies:\n");

	FILE *fp = cas_version_maps_file_open_fn("/proc/self/maps", "r");
	size_t count;
	if (fp == NULL) {
		count = 0;
	} else {
		count = cas_version_collect_dependencies_from_file(fp, deps, sizeof(deps) / sizeof(deps[0]));
		(void)fclose(fp);
	}

	if (count == 0) {
		(void)fprintf(out, "  - none\n");
		return 0;
	}

	for (size_t i = 0; i < count; i++) {
		(void)fprintf(out, "  - %-8s %s\n", deps[i].name, deps[i].version);
	}

	return 0;
}
