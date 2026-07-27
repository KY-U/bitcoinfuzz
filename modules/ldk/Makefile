all: module.a

CXXFLAGS += -Wall -Wextra -fsanitize=address,fuzzer -std=c++20 -I ../../include

TARGET := $(shell rustc --print host-tuple)

LDK_LIB_PATH := ./ldk_lib/target/$(TARGET)/release/libldk_lib.a

# Namespace this module's vendored libsecp256k1 so it can be linked alongside
# other Rust modules pinning the same secp256k1-sys version.
SYMBOL_PREFIX ?= ldk

$(LDK_LIB_PATH):
	cd ldk_lib && \
	RUSTFLAGS="\
		-Z sanitizer=address \
		-C passes=sancov-module \
		-C llvm-args=-sanitizer-coverage-inline-8bit-counters \
		-C llvm-args=-sanitizer-coverage-trace-compares \
		-C llvm-args=-sanitizer-coverage-pc-table \
		-C llvm-args=-sanitizer-coverage-level=4 \
		-C llvm-args=-simplifycfg-branch-fold-threshold=0" \
	cargo +nightly build --release --target $(TARGET)

cargo: $(LDK_LIB_PATH)

module.a: module.o $(LDK_LIB_PATH)
	bash ../../scripts/merge.sh module.a $(LDK_LIB_PATH) module.o
	bash ../../scripts/rename_rust_secp_symbols.sh module.a $(SYMBOL_PREFIX)
	ranlib module.a

module.o: module.cpp module.h
	$(CXX) $(CXXFLAGS) -I . -fPIC -c module.cpp -o module.o

format:
	clang-format -i ./module.cpp ./module.h ./ldk_lib/ldk_lib.h
	cd ldk_lib && cargo fmt

check-format:
	clang-format -Werror --fail-on-incomplete-format -n ./module.cpp ./module.h ./ldk_lib/ldk_lib.h
	cd ldk_lib && cargo fmt --check

clean:
	rm -rf *.o module.a
	cd ldk_lib && cargo clean

.PHONY: all cargo clean format check-format $(LDK_LIB_PATH)
