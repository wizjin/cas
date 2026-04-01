.PHONY: clean build format tidy test coverage release
.NOTPARALLEL:
.SILENT:

CMAKE ?= cmake
CTEST ?= ctest
BUILD_DIR := build/debug
COVERAGE_DIR := build/coverage
RELEASE_DIR := build/release
CMAKE_CONFIGURE := $(CMAKE) -Wno-deprecated --log-level=NOTICE
CMAKE_BUILD := $(CMAKE) --build
PATH_FILTER_SCRIPT := sed -E "s|$(CURDIR)/||g; s|-- Build files have been written to: .*build/|-- Build files have been written to: build/|g"

clean:
	echo "Cleaning build directory"
	test ! -d build || find build -type f -delete
	test ! -d build || find build -depth -type d -empty -delete
	echo "Cleaning legacy FetchContent build state"
	test ! -d libs || find libs -maxdepth 1 \( -name '*-build' -o -name '*-subbuild' \) -type d -exec rm -rf {} +

build:
	echo "Configuring debug build"
	bash -o pipefail -c '$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Building debug targets"
	bash -o pipefail -c '$(CMAKE_BUILD) $(BUILD_DIR) -- -s 2>&1 | $(PATH_FILTER_SCRIPT)'

format:
	echo "Configuring format target"
	bash -o pipefail -c '$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Running clang-format"
	bash -o pipefail -c '$(CMAKE_BUILD) $(BUILD_DIR) --target cas_format -- -s 2>&1 | $(PATH_FILTER_SCRIPT)'

tidy:
	echo "Configuring tidy target"
	bash -o pipefail -c '$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Running clang-tidy on src/ and include/"
	bash -o pipefail -c '$(CMAKE_BUILD) $(BUILD_DIR) --target cas_tidy -- -s 2>&1 | $(PATH_FILTER_SCRIPT)'

test:
	echo "Configuring test build"
	bash -o pipefail -c '$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Building tests"
	bash -o pipefail -c '$(CMAKE_BUILD) $(BUILD_DIR) -- -s 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Running tests"
	bash -o pipefail -c '$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure --quiet 2>&1 | $(PATH_FILTER_SCRIPT)'

coverage:
	echo "Configuring coverage build"
	bash -o pipefail -c '$(CMAKE_CONFIGURE) -S . -B $(COVERAGE_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON -DCAS_ENABLE_COVERAGE=ON 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Building coverage targets"
	bash -o pipefail -c '$(CMAKE_BUILD) $(COVERAGE_DIR) -- -s 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Running coverage"
	bash -o pipefail -c '$(CMAKE_BUILD) $(COVERAGE_DIR) --target cas_coverage -- -s 2>&1 | $(PATH_FILTER_SCRIPT)'

release:
	echo "Configuring release build"
	bash -o pipefail -c '$(CMAKE_CONFIGURE) -S . -B $(RELEASE_DIR) -DCMAKE_BUILD_TYPE=Release -DCAS_ENABLE_TESTS=OFF 2>&1 | $(PATH_FILTER_SCRIPT)'
	echo "Building release targets"
	bash -o pipefail -c '$(CMAKE_BUILD) $(RELEASE_DIR) -- -s 2>&1 | $(PATH_FILTER_SCRIPT)'
