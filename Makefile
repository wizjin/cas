.PHONY: clean build format tidy test coverage release
.NOTPARALLEL:
.SILENT:

CMAKE ?= cmake
CTEST ?= ctest
BUILD_DIR := build/debug
RELEASE_DIR := build/release
CMAKE_CONFIGURE := $(CMAKE) --log-level=NOTICE
CMAKE_BUILD := $(CMAKE) --build

clean:
	echo "Cleaning build directory"
	test ! -d build || find build -type f -delete
	test ! -d build || find build -depth -type d -empty -delete

build:
	echo "Configuring debug build"
	$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON
	echo "Building debug targets"
	$(CMAKE_BUILD) $(BUILD_DIR) -- -s

format:
	echo "Configuring format target"
	$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON
	echo "Running clang-format"
	$(CMAKE_BUILD) $(BUILD_DIR) --target cas_format -- -s

tidy:
	echo "Configuring tidy target"
	$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON
	echo "Running clang-tidy on src/ and include/"
	$(CMAKE_BUILD) $(BUILD_DIR) --target cas_tidy -- -s

test:
	echo "Configuring test build"
	$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON
	echo "Building tests"
	$(CMAKE_BUILD) $(BUILD_DIR) -- -s
	echo "Running tests"
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure --quiet

coverage:
	echo "Configuring coverage build"
	$(CMAKE_CONFIGURE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCAS_ENABLE_TESTS=ON -DCAS_ENABLE_COVERAGE=ON
	echo "Building coverage targets"
	$(CMAKE_BUILD) $(BUILD_DIR) -- -s
	echo "Running coverage"
	$(CMAKE_BUILD) $(BUILD_DIR) --target cas_coverage -- -s

release:
	echo "Configuring release build"
	$(CMAKE_CONFIGURE) -S . -B $(RELEASE_DIR) -DCMAKE_BUILD_TYPE=Release -DCAS_ENABLE_TESTS=OFF
	echo "Building release targets"
	$(CMAKE_BUILD) $(RELEASE_DIR) -- -s
