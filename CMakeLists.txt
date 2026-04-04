cmake_minimum_required(VERSION 3.21)

project(CAS VERSION 0.1.0 LANGUAGES C)

include(CTest)
include(FetchContent)
find_package(Threads REQUIRED)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CAS_DEPS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/libs")
set(CAS_DEPS_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps")

option(CAS_ENABLE_TESTS "Build unit tests" ON)
option(CAS_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
option(CAS_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

set(CAS_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(CAS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(CAS_TEST_DIR "${CMAKE_SOURCE_DIR}/tests")
set(CAS_MAIN_FILE "src/main.c")

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
	pkg_check_modules(JEMALLOC QUIET jemalloc)
	pkg_check_modules(LIBUV QUIET libuv)
	pkg_check_modules(LLHTTP QUIET llhttp)
	pkg_check_modules(CJSON QUIET libcjson)

	if(NOT CJSON_FOUND)
		pkg_check_modules(CJSON QUIET cjson)
	endif()
endif()

function(cas_add_pkg_config_interface target_name prefix)
	if(NOT ${prefix}_FOUND)
		return()
	endif()

	add_library(${target_name} INTERFACE)
	target_include_directories(${target_name} INTERFACE ${${prefix}_INCLUDE_DIRS})

	if(${prefix}_LIBRARY_DIRS)
		target_link_directories(${target_name} INTERFACE ${${prefix}_LIBRARY_DIRS})
	endif()

	target_link_libraries(${target_name} INTERFACE ${${prefix}_LIBRARIES})
	target_compile_options(${target_name} INTERFACE ${${prefix}_CFLAGS_OTHER})
endfunction()

function(cas_wrap_target_if_exists wrapper_name actual_name)
	if(TARGET ${actual_name} AND NOT TARGET ${wrapper_name})
		add_library(${wrapper_name} INTERFACE)
		target_link_libraries(${wrapper_name} INTERFACE ${actual_name})
	endif()
endfunction()

function(cas_ensure_libuv_target)
	set(cas_libuv_source_dir "${CAS_DEPS_SOURCE_DIR}/cas_libuv-src")
	set(cas_libuv_binary_dir "${CAS_DEPS_BUILD_DIR}/cas_libuv-build")
	set(cas_libuv_subbuild_dir "${CAS_DEPS_BUILD_DIR}/cas_libuv-subbuild")

	if(LIBUV_FOUND)
		cas_add_pkg_config_interface(cas_dep_libuv LIBUV)
		return()
	endif()

	if(EXISTS "${cas_libuv_source_dir}/CMakeLists.txt")
		FetchContent_Declare(cas_libuv
			SOURCE_DIR "${cas_libuv_source_dir}"
			BINARY_DIR "${cas_libuv_binary_dir}"
			SUBBUILD_DIR "${cas_libuv_subbuild_dir}"
		)
	else()
		FetchContent_Declare(cas_libuv
			GIT_REPOSITORY https://github.com/libuv/libuv.git
			GIT_TAG v1.51.0
			SOURCE_DIR "${cas_libuv_source_dir}"
			BINARY_DIR "${cas_libuv_binary_dir}"
			SUBBUILD_DIR "${cas_libuv_subbuild_dir}"
		)
	endif()
	FetchContent_MakeAvailable(cas_libuv)

	cas_wrap_target_if_exists(cas_dep_libuv uv)
	cas_wrap_target_if_exists(cas_dep_libuv uv_a)

	if(NOT TARGET cas_dep_libuv)
		message(FATAL_ERROR "Unable to configure libuv target.")
	endif()
endfunction()

function(cas_ensure_llhttp_target)
	set(cas_llhttp_source_dir "${CAS_DEPS_SOURCE_DIR}/cas_llhttp-src")
	set(cas_llhttp_binary_dir "${CAS_DEPS_BUILD_DIR}/cas_llhttp-build")
	set(cas_llhttp_subbuild_dir "${CAS_DEPS_BUILD_DIR}/cas_llhttp-subbuild")

	if(LLHTTP_FOUND)
		cas_add_pkg_config_interface(cas_dep_llhttp LLHTTP)
		return()
	endif()

	if(EXISTS "${cas_llhttp_source_dir}/CMakeLists.txt")
		FetchContent_Declare(cas_llhttp
			SOURCE_DIR "${cas_llhttp_source_dir}"
			BINARY_DIR "${cas_llhttp_binary_dir}"
			SUBBUILD_DIR "${cas_llhttp_subbuild_dir}"
		)
	else()
		FetchContent_Declare(cas_llhttp
			GIT_REPOSITORY https://github.com/nodejs/llhttp.git
			GIT_TAG release/v9.3.0
			SOURCE_DIR "${cas_llhttp_source_dir}"
			BINARY_DIR "${cas_llhttp_binary_dir}"
			SUBBUILD_DIR "${cas_llhttp_subbuild_dir}"
		)
	endif()
	set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
	set(BUILD_STATIC_LIBS OFF CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(cas_llhttp)

	cas_wrap_target_if_exists(cas_dep_llhttp llhttp::llhttp)
	cas_wrap_target_if_exists(cas_dep_llhttp llhttp)
	cas_wrap_target_if_exists(cas_dep_llhttp llhttp_static)
	cas_wrap_target_if_exists(cas_dep_llhttp llhttp_shared)

	if(NOT TARGET cas_dep_llhttp)
		message(FATAL_ERROR "Unable to configure llhttp target.")
	endif()
endfunction()

function(cas_ensure_cjson_target)
	set(cas_cjson_source_dir "${CAS_DEPS_SOURCE_DIR}/cas_cjson-src")
	set(cas_cjson_binary_dir "${CAS_DEPS_BUILD_DIR}/cas_cjson-build")
	set(cas_cjson_subbuild_dir "${CAS_DEPS_BUILD_DIR}/cas_cjson-subbuild")

	if(CJSON_FOUND)
		cas_add_pkg_config_interface(cas_dep_cjson CJSON)
		return()
	endif()

	if(EXISTS "${cas_cjson_source_dir}/CMakeLists.txt")
		FetchContent_Declare(cas_cjson
			SOURCE_DIR "${cas_cjson_source_dir}"
			BINARY_DIR "${cas_cjson_binary_dir}"
			SUBBUILD_DIR "${cas_cjson_subbuild_dir}"
		)
	else()
		FetchContent_Declare(cas_cjson
			GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
			GIT_TAG v1.7.19
			SOURCE_DIR "${cas_cjson_source_dir}"
			BINARY_DIR "${cas_cjson_binary_dir}"
			SUBBUILD_DIR "${cas_cjson_subbuild_dir}"
		)
	endif()
	set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
	set(ENABLE_CJSON_UTILS OFF CACHE BOOL "" FORCE)
	set(ENABLE_TARGET_EXPORT OFF CACHE BOOL "" FORCE)
	set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(cas_cjson)

	cas_wrap_target_if_exists(cas_dep_cjson cjson)
	cas_wrap_target_if_exists(cas_dep_cjson cJSON)

	if(TARGET cas_dep_cjson AND EXISTS "${cas_cjson_source_dir}/cJSON.h")
		target_include_directories(cas_dep_cjson INTERFACE "${cas_cjson_source_dir}")
	endif()

	if(NOT TARGET cas_dep_cjson)
		message(FATAL_ERROR "Unable to configure cJSON target.")
	endif()
endfunction()

cas_ensure_libuv_target()
cas_ensure_llhttp_target()
cas_ensure_cjson_target()

execute_process(
	COMMAND git rev-parse --short HEAD
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	RESULT_VARIABLE CAS_GIT_REV_PARSE_RESULT
	OUTPUT_VARIABLE CAS_GIT_SHORT_COMMIT
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT CAS_GIT_REV_PARSE_RESULT EQUAL 0 OR CAS_GIT_SHORT_COMMIT STREQUAL "")
	set(CAS_GIT_SHORT_COMMIT "unknown")
endif()

if(CMAKE_SYSTEM_PROCESSOR)
	set(CAS_TARGET_ARCH_VALUE "${CMAKE_SYSTEM_PROCESSOR}")
else()
	set(CAS_TARGET_ARCH_VALUE "unknown")
endif()

if(CMAKE_SYSTEM_NAME)
	set(CAS_TARGET_OS_VALUE "${CMAKE_SYSTEM_NAME}")
	string(TOLOWER "${CAS_TARGET_OS_VALUE}" CAS_TARGET_OS_VALUE)
else()
	set(CAS_TARGET_OS_VALUE "unknown")
endif()

string(TIMESTAMP CAS_BUILD_TIMESTAMP_VALUE "%Y-%m-%dT%H:%M:%S")

file(GLOB_RECURSE CAS_SOURCE_FILES RELATIVE "${CMAKE_SOURCE_DIR}" CONFIGURE_DEPENDS "src/*.c")
file(GLOB_RECURSE CAS_PUBLIC_HEADER_FILES RELATIVE "${CMAKE_SOURCE_DIR}" CONFIGURE_DEPENDS "include/*.h")
file(GLOB_RECURSE CAS_INTERNAL_HEADER_FILES RELATIVE "${CMAKE_SOURCE_DIR}" CONFIGURE_DEPENDS "src/*.h")
file(GLOB_RECURSE CAS_TEST_FILES RELATIVE "${CMAKE_SOURCE_DIR}" CONFIGURE_DEPENDS "tests/*.c")
set(CAS_HEADER_FILES ${CAS_PUBLIC_HEADER_FILES} ${CAS_INTERNAL_HEADER_FILES})
set(CAS_LIBRARY_SOURCE_FILES ${CAS_SOURCE_FILES})

if(EXISTS "${CMAKE_SOURCE_DIR}/${CAS_MAIN_FILE}")
	list(REMOVE_ITEM CAS_LIBRARY_SOURCE_FILES "${CAS_MAIN_FILE}")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	set(CAS_WARNING_FLAGS
		-Wall
		-Wextra
		-Wpedantic
		-Wconversion
		-Wshadow
		-Wstrict-prototypes
		-Wmissing-prototypes
		-Werror=implicit-function-declaration
	)
	set(CAS_PATH_REMAP_FLAGS
		-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
		-ffile-prefix-map=${CMAKE_SOURCE_DIR}/=
	)

	if(CAS_WARNINGS_AS_ERRORS)
		list(APPEND CAS_WARNING_FLAGS -Werror)
	endif()
endif()

if(CAS_ENABLE_COVERAGE AND CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
	add_compile_options(-O0 -g --coverage)
	add_link_options(--coverage)
endif()

add_library(cas_project_options INTERFACE)
target_compile_features(cas_project_options INTERFACE c_std_17)
target_compile_options(cas_project_options INTERFACE ${CAS_WARNING_FLAGS})
target_compile_options(cas_project_options INTERFACE ${CAS_PATH_REMAP_FLAGS})
target_compile_definitions(cas_project_options INTERFACE
	_POSIX_C_SOURCE=200809L
	CAS_VERSION="${PROJECT_VERSION}"
	CAS_GIT_SHORT_COMMIT="${CAS_GIT_SHORT_COMMIT}"
	CAS_BUILD_TIMESTAMP="${CAS_BUILD_TIMESTAMP_VALUE}"
	CAS_TARGET_OS="${CAS_TARGET_OS_VALUE}"
	CAS_TARGET_ARCH="${CAS_TARGET_ARCH_VALUE}"
)
target_link_libraries(cas_project_options INTERFACE ${CMAKE_DL_LIBS})
target_link_libraries(cas_project_options INTERFACE Threads::Threads)

target_include_directories(cas_project_options INTERFACE "${CAS_SRC_DIR}")

if(EXISTS "${CAS_INCLUDE_DIR}")
	target_include_directories(cas_project_options INTERFACE "${CAS_INCLUDE_DIR}")
endif()

if(JEMALLOC_FOUND)
	target_include_directories(cas_project_options INTERFACE ${JEMALLOC_INCLUDE_DIRS})
	target_link_directories(cas_project_options INTERFACE ${JEMALLOC_LIBRARY_DIRS})
	target_link_libraries(cas_project_options INTERFACE ${JEMALLOC_LIBRARIES})
	target_compile_options(cas_project_options INTERFACE ${JEMALLOC_CFLAGS_OTHER})
endif()

if(CAS_LIBRARY_SOURCE_FILES)
	add_library(cas_lib STATIC ${CAS_LIBRARY_SOURCE_FILES} ${CAS_HEADER_FILES})
	target_link_libraries(cas_lib
		PUBLIC
			cas_project_options
			cas_dep_libuv
			cas_dep_llhttp
			cas_dep_cjson
	)
	target_compile_definitions(cas_lib PUBLIC _GNU_SOURCE)
	set_target_properties(cas_lib PROPERTIES OUTPUT_NAME cas_core)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/${CAS_MAIN_FILE}")
	add_executable(cas "${CAS_MAIN_FILE}")
	target_link_libraries(cas PRIVATE cas_project_options)

	if(TARGET cas_lib)
		target_link_libraries(cas PRIVATE cas_lib)
	endif()

	if(CAS_ENABLE_TESTS)
		add_test(NAME cas_help COMMAND cas help)
		add_test(NAME cas_short_version COMMAND cas --version)
		add_test(NAME cas_ver COMMAND cas version)
	endif()
else()
	add_custom_target(cas
		COMMAND ${CMAKE_COMMAND} -E echo "No sources found under src/."
		VERBATIM
	)
endif()

if(CAS_ENABLE_TESTS)
	enable_testing()

	if(CAS_TEST_FILES)
		find_package(cmocka QUIET)

		if(cmocka_FOUND)
			foreach(CAS_TEST_FILE IN LISTS CAS_TEST_FILES)
				get_filename_component(CAS_TEST_NAME "${CAS_TEST_FILE}" NAME_WE)
				add_executable(${CAS_TEST_NAME} ${CAS_TEST_FILE})
				target_compile_definitions(${CAS_TEST_NAME} PRIVATE _GNU_SOURCE)
				target_link_libraries(${CAS_TEST_NAME} PRIVATE cas_project_options cmocka::cmocka)

				if(TARGET cas_lib AND NOT ${CAS_TEST_NAME} MATCHES "^test_cas_log")
					target_link_libraries(${CAS_TEST_NAME} PRIVATE cas_lib)
				endif()

				if(${CAS_TEST_NAME} STREQUAL "test_cas_log")
					target_link_options(${CAS_TEST_NAME} PRIVATE
						"LINKER:--wrap=malloc"
						"LINKER:--wrap=pthread_mutex_init"
						"LINKER:--wrap=pthread_mutex_lock"
						"LINKER:--wrap=clock_gettime"
						"LINKER:--wrap=localtime_r"
						"LINKER:--wrap=strftime"
						"LINKER:--wrap=vsnprintf"
					)
				endif()

				add_test(NAME ${CAS_TEST_NAME} COMMAND ${CAS_TEST_NAME})
			endforeach()
		else()
			message(WARNING "cmocka not found; tests are disabled.")
		endif()
	endif()
endif()

find_program(CAS_CLANG_FORMAT clang-format)
if(CAS_CLANG_FORMAT)
	set(CAS_FORMAT_FILES ${CAS_SOURCE_FILES} ${CAS_HEADER_FILES} ${CAS_TEST_FILES})
	list(REMOVE_DUPLICATES CAS_FORMAT_FILES)

	if(CAS_FORMAT_FILES)
		add_custom_target(cas_format
			COMMAND ${CAS_CLANG_FORMAT} -i ${CAS_FORMAT_FILES}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			VERBATIM
		)
	else()
		add_custom_target(cas_format
			COMMAND ${CMAKE_COMMAND} -E echo "No files found for formatting."
			VERBATIM
		)
	endif()
else()
	add_custom_target(cas_format
		COMMAND ${CMAKE_COMMAND} -E echo "clang-format not found."
		VERBATIM
	)
endif()

find_program(CAS_CLANG_TIDY clang-tidy)
if(CAS_CLANG_TIDY)
	set(CAS_TIDY_FILES ${CAS_SOURCE_FILES} ${CAS_HEADER_FILES})
	list(REMOVE_DUPLICATES CAS_TIDY_FILES)

	if(CAS_TIDY_FILES)
		set(CAS_TIDY_ARGS)
		foreach(CAS_TIDY_FILE IN LISTS CAS_TIDY_FILES)
			list(APPEND CAS_TIDY_ARGS "${CAS_TIDY_FILE}")
		endforeach()

		add_custom_target(cas_tidy
			COMMAND bash -o pipefail -c "\"$@\" 2>&1 | grep -Ev '^[0-9]+ warnings generated\\.$' | sed 's|${CMAKE_SOURCE_DIR}/||g'"
				_ ${CAS_CLANG_TIDY} --quiet -p ${CMAKE_BINARY_DIR} ${CAS_TIDY_ARGS}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			VERBATIM
		)
	else()
		add_custom_target(cas_tidy
			COMMAND ${CMAKE_COMMAND} -E echo "No src/ or include/ files found for clang-tidy."
			VERBATIM
		)
	endif()
else()
	add_custom_target(cas_tidy
		COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy not found."
		VERBATIM
	)
endif()

find_program(CAS_GCOVR gcovr)
if(CAS_GCOVR AND CAS_ENABLE_COVERAGE)
	add_custom_target(cas_coverage
		COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --quiet
		COMMAND ${CAS_GCOVR}
			--root ${CMAKE_SOURCE_DIR}
			--filter ${CAS_SRC_DIR}
			--filter ${CAS_INCLUDE_DIR}
			--exclude ${CAS_TEST_DIR}
			--gcov-ignore-parse-errors negative_hits.warn
			--print-summary
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		VERBATIM
	)
else()
	add_custom_target(cas_coverage
		COMMAND ${CMAKE_COMMAND} -E echo "Coverage requires CAS_ENABLE_COVERAGE=ON and gcovr."
		VERBATIM
	)
endif()
