#ifndef CAS_VERSION_H
#define CAS_VERSION_H

#include <stddef.h>
#include <stdio.h>

#define CAS_VERSION_DEPENDENCY_NAME_SIZE	16
#define CAS_VERSION_DEPENDENCY_VERSION_SIZE 64

typedef struct {
	char name[CAS_VERSION_DEPENDENCY_NAME_SIZE];
	char version[CAS_VERSION_DEPENDENCY_VERSION_SIZE];
} cas_version_dependency_t;

typedef FILE *(*cas_version_maps_file_open_fn_t)(const char *path, const char *mode);

int cas_version_run_short(FILE *out);
int cas_version_run_details(FILE *out);
const char *cas_version_dependency_basename(const char *path);
void cas_version_extract_dependency_version(char *version, size_t version_size, const char *path);
size_t cas_version_collect_dependencies_from_file(FILE *maps_file, cas_version_dependency_t *dependencies,
												  size_t capacity);
void cas_version_set_maps_file_open_fn(cas_version_maps_file_open_fn_t open_fn);

#endif
