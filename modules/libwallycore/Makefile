all: module.a

CXXFLAGS += -Wall -Wextra -std=c++20 \
            -fsanitize=fuzzer,address \
            -I . \
            -I ../../include \
            -I ../../external/libwally-core/include \
						-I ../../external/libwally-core/src/ccan \
						-I ../../external/libwally-core/_cmake \
						-I ../../external/libwally-core/src

LIBWALLY_CORE_LIB = ../../external/libwally-core/src/.libs/libwallycore.a
LIBWALLY_CORE_SECP_LIB = ../../external/libwally-core/src/secp256k1/.libs/libsecp256k1.a
MAKE = make
SYMBOL_PREFIX ?= libwallycore
REDEFINE_SYMS = /tmp/libwallycore_redefine.syms

# Detect objcopy command (required for symbol remapping to avoid secp256k1
# collisions with the standalone secp256k1 module).
OBJCOPY := $(shell command -v objcopy 2> /dev/null)
ifeq ($(OBJCOPY),)
$(error objcopy not found. Install with: apt-get install binutils)
endif

# Build libwally_core with sanitizer flags
$(LIBWALLY_CORE_LIB):
	cd ../../external/libwally-core && \
	./tools/autogen.sh && \
	./configure \
		--disable-shared \
		--enable-export-all \
		--disable-elements \
		--disable-elements-abi \
		--disable-tests \
		CFLAGS="-fsanitize=address,fuzzer-no-link" && \
	$(MAKE)

# Generate symbol redefine map: prefix every global symbol defined in
# libwally's vendored libsecp256k1.a (not just secp256k1_*) so they don't
# collide with the standalone secp256k1 module (or any other module bundling
# libsecp256k1) when linked together. This also catches the
# `__odr_asan_gen_*` ODR-detection symbols that ASAN auto-generates.
# Same pattern as modules/libbitcoinsystem/Makefile.
secp256k1_symbols.map: $(LIBWALLY_CORE_SECP_LIB)
	@echo "Generating secp256k1 symbol map for $(SYMBOL_PREFIX)..."
	@{ \
		nm -g --defined-only $(LIBWALLY_CORE_SECP_LIB) | awk '{print $$3}'; \
		nm --defined-only $(LIBWALLY_CORE_SECP_LIB) | awk '$$2 == "n" {print $$3}'; \
	} | grep -Ev '^$$' | sort -u | while read sym; do \
		echo "$$sym $(SYMBOL_PREFIX)_$$sym"; \
	done > $@
	@sort -u -o $@ $@
	@echo "Generated $$(wc -l < $@) symbol mappings"

module.a: $(LIBWALLY_CORE_LIB) $(LIBWALLY_CORE_SECP_LIB) secp256k1_symbols.map module.o
	rm -f module.a libwallycore_renamed.a libsecp256k1_renamed.a
	# Prefix every secp256k1 symbol (definitions in libsecp256k1.a AND
	# references in libwallycore.a) using objcopy --redefine-syms, so the
	# vendored secp256k1 cannot collide with modules/secp256k1/module.a.
	@echo "Renaming secp256k1 symbols in libwallycore and secp256k1 archives..."
	$(OBJCOPY) --redefine-syms=$(abspath secp256k1_symbols.map) $(LIBWALLY_CORE_LIB) libwallycore_renamed.a
	$(OBJCOPY) --redefine-syms=$(abspath secp256k1_symbols.map) $(LIBWALLY_CORE_SECP_LIB) libsecp256k1_renamed.a
	printf 'CREATE module.a\nADDLIB libwallycore_renamed.a\nADDLIB libsecp256k1_renamed.a\nADDMOD module.o\nSAVE\nEND\n' | $(AR) -M
	rm -f libwallycore_renamed.a libsecp256k1_renamed.a
	ranlib module.a
	# Prefix libwally's own remaining ASAN ODR globals so they don't collide
	# with other modules that bundle libwally (e.g. clightning).
	@nm module.a | grep '__odr_asan_gen_' | awk '{print $$3}' | sort -u | \
		grep -v '^$(SYMBOL_PREFIX)_' | \
		awk '{print $$1, "$(SYMBOL_PREFIX)_" $$1}' > $(REDEFINE_SYMS)
	@if [ -s $(REDEFINE_SYMS) ]; then \
		$(OBJCOPY) --redefine-syms=$(REDEFINE_SYMS) module.a; \
	fi
	@rm -f $(REDEFINE_SYMS)
	@echo "Verifying no unprefixed secp256k1 symbols remain in module.a:"
	@if nm -g --defined-only module.a | awk '{print $$3}' | grep -q '^secp256k1_'; then \
		echo "ERROR: unprefixed secp256k1 symbols found in module.a"; exit 1; \
	fi

module.o: module.cpp module.h
	$(CXX) $(CXXFLAGS) -I . -fPIC -c module.cpp -o module.o

format:
	clang-format -i ./module.cpp ./module.h

check-format:
	clang-format -Werror --fail-on-incomplete-format -n ./module.cpp ./module.h

clean:
	rm -f *.o *.a secp256k1_symbols.map
	cd ../../external/libwally-core && $(MAKE) clean || true

.PHONY: all clean format check-format
