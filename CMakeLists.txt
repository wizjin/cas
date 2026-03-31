cmake_minimum_required(VERSION 3.21)

project(CAS VERSION 0.1.0 LANGUAGES C)

include(CTest)
include(FetchContent)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/libs")

option(CAS_ENABLE_TESTS "Build unit tests" ON)
option(CAS_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
option(CAS_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

set(CAS_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(CAS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(CAS_TEST_DIR "${CMAKE_SOURCE_DIR}/tests")
set(CAS_MAIN_FILE "${CAS_SRC_DIR}/main.c")

file(GLOB_RECURSE CAS_SOURCE_FILES CONFIGURE_DEPENDS "${CAS_SRC_DIR}/*.c")
file(GLOB_RECURSE CAS_PUBLIC_HEADER_FILES CONFIGURE_DEPENDS "${CAS_INCLUDE_DIR}/*.h")
file(GLOB_RECURSE CAS_INTERNAL_HEADER_FILES CONFIGURE_DEPENDS "${CAS_SRC_DIR}/*.h")
file(GLOB_RECURSE CAS_TEST_FILES CONFIGURE_DEPENDS "${CAS_TEST_DIR}/*.c")
set(CAS_HEADER_FILES ${CAS_PUBLIC_HEADER_FILES} ${CAS_INTERNAL_HEADER_FILES})
set(CAS_LIBRARY_SOURCE_FILES ${CAS_SOURCE_FILES})

if(EXISTS "${CAS_MAIN_FILE}")
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
target_compile_definitions(cas_project_options INTERFACE CAS_VERSION="${PROJECT_VERSION}")

target_include_directories(cas_project_options INTERFACE "${CAS_SRC_DIR}")

if(EXISTS "${CAS_INCLUDE_DIR}")
	target_include_directories(cas_project_options INTERFACE "${CAS_INCLUDE_DIR}")
endif()

if(CAS_LIBRARY_SOURCE_FILES)
	add_library(cas_core STATIC ${CAS_LIBRARY_SOURCE_FILES} ${CAS_HEADER_FILES})
	target_link_libraries(cas_core PUBLIC cas_project_options)
	set_target_properties(cas_core PROPERTIES OUTPUT_NAME cas_core)
endif()

if(EXISTS "${CAS_MAIN_FILE}")
	add_executable(cas "${CAS_MAIN_FILE}")
	target_link_libraries(cas PRIVATE cas_project_options)

	if(TARGET cas_core)
		target_link_libraries(cas PRIVATE cas_core)
	endif()

	if(CAS_ENABLE_TESTS)
		add_test(NAME cas_help COMMAND cas help)
		add_test(NAME cas_version COMMAND cas version)
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
			add_executable(cas_tests ${CAS_TEST_FILES})
			target_compile_definitions(cas_tests PRIVATE _GNU_SOURCE)
			target_link_libraries(cas_tests PRIVATE cas_project_options cmocka::cmocka)

			if(TARGET cas_core)
				target_link_libraries(cas_tests PRIVATE cas_core)
			endif()

			add_test(NAME cas_tests COMMAND cas_tests)
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
			COMMAND ${CAS_CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${CAS_TIDY_ARGS}
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
		COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
		COMMAND ${CAS_GCOVR}
			--root ${CMAKE_SOURCE_DIR}
			--filter ${CAS_SRC_DIR}
			--filter ${CAS_INCLUDE_DIR}
			--exclude ${CAS_TEST_DIR}
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
