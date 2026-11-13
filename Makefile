SDK_ROOT ?= $(CURDIR)/.deps/sdk
DEP_CACHE ?= $(CURDIR)/.deps/cache
TOOLCHAIN = $(SDK_ROOT)/aarch64-linux-gnu-7.5.0-linaro
ARM_CXX = $(TOOLCHAIN)/bin/aarch64-linux-gnu-g++
SYS_LIB = $(TOOLCHAIN)/aarch64-linux-gnu/libc/lib
CPPFLAGS = -Ithird_party -Isrc -I$(SDK_ROOT)/usr/include/SDL2
CXXFLAGS = -std=c++14 -Wall -Wextra -Werror -O2
ARM_LINK = -L$(SYS_LIB) -L$(SDK_ROOT)/usr/lib -Wl,-rpath-link,$(SYS_LIB):$(SDK_ROOT)/usr/lib -static-libstdc++ -static-libgcc
NATIVE_TTF ?= $(SDK_ROOT)/libSDL2_ttf-native.so
RCLONE_NATIVE = $(SDK_ROOT)/rclone-v1.69.1-linux-amd64/rclone
RCLONE_ARM = $(SDK_ROOT)/rclone-v1.69.1-linux-arm64/rclone
QEMU = $(DEP_CACHE)/qemu-aarch64-static

.PHONY: all deps arm64 native test test-fallback test-arm64 test-input test-input-arm64 test-ui test-ui-arm64 smoke-native smoke-arm64 package clean
all: arm64
deps:
	python3 scripts/fetch_deps.py --sdk "$(SDK_ROOT)" --cache "$(DEP_CACHE)"
build:
	mkdir -p build
build/drive-browser-arm64: src/main.cpp src/core.cpp src/core.hpp src/navigation.hpp third_party/json.hpp | build
	$(ARM_CXX) $(CPPFLAGS) $(CXXFLAGS) -mcpu=cortex-a53 src/main.cpp src/core.cpp $(ARM_LINK) -lSDL2 -lSDL2_ttf -o $@
arm64: build/drive-browser-arm64
build/drive-browser-native: src/main.cpp src/core.cpp src/core.hpp src/navigation.hpp third_party/json.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp src/core.cpp $(NATIVE_TTF) -l:libSDL2-2.0.so.0 -o $@
native: build/drive-browser-native
build/core-test: tests/core_test.cpp src/core.cpp src/core.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/core_test.cpp src/core.cpp -o $@
test: build/core-test
	./build/core-test "$(RCLONE_NATIVE)"
build/core-test-fallback: tests/core_test.cpp src/core.cpp src/core.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DDRIVEBROWSER_TEST_COPY_COMMIT tests/core_test.cpp src/core.cpp -o $@
test-fallback: build/core-test-fallback
	./build/core-test-fallback "$(RCLONE_NATIVE)"
build/core-test-arm64: tests/core_test.cpp src/core.cpp src/core.hpp | build
	$(ARM_CXX) $(CPPFLAGS) $(CXXFLAGS) tests/core_test.cpp src/core.cpp -static-libstdc++ -static-libgcc -o $@
test-arm64: build/core-test-arm64
	python3 scripts/arm_test.py --sdk "$(SDK_ROOT)" --qemu "$(QEMU)"
build/navigation-test: tests/navigation_test.cpp src/navigation.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/navigation_test.cpp -o $@
test-input: build/navigation-test
	./build/navigation-test
build/navigation-test-arm64: tests/navigation_test.cpp src/navigation.hpp | build
	$(ARM_CXX) $(CPPFLAGS) $(CXXFLAGS) tests/navigation_test.cpp -static-libstdc++ -static-libgcc -o $@
test-input-arm64: build/navigation-test-arm64
	$(QEMU) -B 0x100000000 $(SYS_LIB)/ld-linux-aarch64.so.1 --library-path $(SYS_LIB) ./build/navigation-test-arm64
build/ui-test-native: tests/ui_test.cpp src/main.cpp src/core.cpp src/core.hpp src/navigation.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/ui_test.cpp src/core.cpp $(NATIVE_TTF) -l:libSDL2-2.0.so.0 -o $@
build/ui-test-arm64: tests/ui_test.cpp src/main.cpp src/core.cpp src/core.hpp src/navigation.hpp | build
	$(ARM_CXX) $(CPPFLAGS) $(CXXFLAGS) -mcpu=cortex-a53 tests/ui_test.cpp src/core.cpp $(ARM_LINK) -lSDL2 -lSDL2_ttf -o $@
test-ui: build/ui-test-native
	python3 scripts/ui_test.py --sdk "$(SDK_ROOT)" --arch native
test-ui-arm64: build/ui-test-arm64
	python3 scripts/ui_test.py --sdk "$(SDK_ROOT)" --arch arm64 --qemu "$(QEMU)"
smoke-native: native
	python3 scripts/smoke.py --sdk "$(SDK_ROOT)" --arch native
smoke-arm64: arm64
	python3 scripts/smoke.py --sdk "$(SDK_ROOT)" --arch arm64 --qemu "$(QEMU)" --driver offscreen
package: arm64
	python3 scripts/package.py --sdk "$(SDK_ROOT)"
clean:
	rm -rf build
