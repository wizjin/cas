#ifndef CAS_VER_H
#define CAS_VER_H

#include "cas_cli.h"

#define CAS_VER_DEP_NAME_SIZE 16
#define CAS_VER_DEP_VER_SIZE  64

typedef struct {
	char name[CAS_VER_DEP_NAME_SIZE];
	char version[CAS_VER_DEP_VER_SIZE];
} cas_ver_dep_t;

typedef FILE *(*cas_ver_maps_file_open_fn_t)(const char *path, const char *mode);

int cas_ver_show_short(const cas_cli_t *cli);
int cas_ver_show_details(const cas_cli_t *cli);

const char *cas_ver_dep_basename(const char *path);
void cas_ver_dep_version(char *ver, size_t ver_size, const char *path);
size_t cas_ver_collect_deps(FILE *maps_file, cas_ver_dep_t *deps, size_t capacity);
void cas_ver_set_maps_open_fn(cas_ver_maps_file_open_fn_t open_fn);

#endif
