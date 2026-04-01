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
	const char *slash;

	slash = strrchr(path, '/');
	if (slash == NULL) {
		return path;
	}

	return slash + 1;
}

void cas_version_extract_dependency_version(char *version, size_t version_size, const char *path)
{
	const char *basename;
	const char *marker;

	basename = cas_version_dependency_basename(path);
	marker = strstr(basename, ".so.");
	if (marker == NULL || marker[4] == '\0') {
		cas_utils_copy_string(version, version_size, "unknown");
		return;
	}

	cas_utils_copy_string(version, version_size, marker + 4);
}

static int cas_version_dependency_matches(const char *path, size_t index)
{
	const char *basename;

	basename = cas_version_dependency_basename(path);
	return strstr(basename, cas_version_dependency_specs[index].match) != NULL;
}

static void cas_version_scan_dependencies(FILE *maps_file, cas_version_dependency_scan_context_t *context)
{
	char line[PATH_MAX * 2];
	size_t index;

	while (fgets(line, sizeof(line), maps_file) != NULL) {
		char *path;
		char *newline;

		path = strchr(line, '/');
		if (path == NULL) {
			continue;
		}

		newline = strchr(path, '\n');
		if (newline != NULL) {
			*newline = '\0';
		}

		for (index = 0; index < sizeof(cas_version_dependency_specs) / sizeof(cas_version_dependency_specs[0]); index++) {
			if (!context->found[index] && cas_version_dependency_matches(path, index)) {
				cas_utils_copy_string(context->paths[index], sizeof(context->paths[index]), path);
				context->found[index] = 1;
				break;
			}
		}
	}
}

size_t cas_version_collect_dependencies_from_file(FILE *maps_file, cas_version_dependency_t *dependencies, size_t capacity)
{
	cas_version_dependency_scan_context_t context;
	size_t count;
	size_t index;

	if (maps_file == NULL || dependencies == NULL || capacity == 0) {
		return 0;
	}

	(void)memset(&context, 0, sizeof(context));
	cas_version_scan_dependencies(maps_file, &context);

	count = 0;
	for (index = 0; index < sizeof(cas_version_dependency_specs) / sizeof(cas_version_dependency_specs[0]); index++) {
		if (!context.found[index] || count >= capacity) {
			continue;
		}

		cas_utils_copy_string(
			dependencies[count].name, sizeof(dependencies[count].name), cas_version_dependency_specs[index].name);
		cas_version_extract_dependency_version(
			dependencies[count].version, sizeof(dependencies[count].version), context.paths[index]);
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
	cas_version_dependency_t dependencies[4];
	size_t dependency_count;
	size_t index;
	FILE *maps_file;

	(void)fprintf(out, "Version:      %s\n", CAS_VERSION);
	(void)fprintf(out, "Git Commit:   %s\n", CAS_GIT_SHORT_COMMIT);
	(void)fprintf(out, "Built:        %s\n", CAS_BUILD_TIMESTAMP);
	(void)fprintf(out, "OS/Arch:      %s/%s\n", CAS_TARGET_OS, CAS_TARGET_ARCH);
	(void)fprintf(out, "Dependencies:\n");

	maps_file = cas_version_maps_file_open_fn("/proc/self/maps", "r");
	if (maps_file == NULL) {
		dependency_count = 0;
	} else {
		dependency_count = cas_version_collect_dependencies_from_file(
			maps_file, dependencies, sizeof(dependencies) / sizeof(dependencies[0]));
		(void)fclose(maps_file);
	}

	if (dependency_count == 0) {
		(void)fprintf(out, "  - none\n");
		return 0;
	}

	for (index = 0; index < dependency_count; index++) {
		(void)fprintf(out, "  - %-8s %s\n", dependencies[index].name, dependencies[index].version);
	}

	return 0;
}
