all: module.a

TS_DIR := ./ts
NODE_MODULES_DIR := $(TS_DIR)/node_modules
ESBUILD := $(NODE_MODULES_DIR)/.bin/esbuild
PRETTIER := $(NODE_MODULES_DIR)/.bin/prettier
DIST_DIR := $(TS_DIR)/dist
RUNNER_JS := $(DIST_DIR)/index.js
TS_SOURCES := $(TS_DIR)/src/index.ts

# The library runs on an unmodified Node process rather than an embedded engine,
# so the runner is located at runtime by absolute path.
RUNNER_PATH := $(shell pwd)/$(RUNNER_JS)

CXXFLAGS += -Wall -Wextra -fsanitize=address,fuzzer -std=c++20 -I ../../include \
	-DBLUEWALLET_SP_RUNNER=\"$(RUNNER_PATH)\"

module.a: module.o $(RUNNER_JS)
	$(AR) rcs $@ module.o
	ranlib $@

$(NODE_MODULES_DIR): $(TS_DIR)/package.json $(TS_DIR)/package-lock.json
	cd $(TS_DIR) && npm ci

# silent-payments ships its entry point as raw TypeScript, which Node refuses to
# load from node_modules, so the bundler compiles it. tiny-secp256k1 stays
# external because it resolves its .wasm file relative to its own directory;
# Node loads the real package, WASM and all. Building for the node platform also
# leaves the builtins alone, so nothing here is shimmed.
$(RUNNER_JS): $(NODE_MODULES_DIR) $(TS_SOURCES)
	$(ESBUILD) $(TS_SOURCES) \
		--bundle \
		--platform=node \
		--format=cjs \
		--external:tiny-secp256k1 \
		--outdir=$(DIST_DIR)

module.o: module.cpp module.h
	$(CXX) $(CXXFLAGS) -fPIC -c module.cpp -o module.o

format: $(NODE_MODULES_DIR)
	clang-format -i ./module.cpp ./module.h
	$(PRETTIER) $(TS_SOURCES) --write --config $(TS_DIR)/.prettier.json

check-format: $(NODE_MODULES_DIR)
	clang-format -Werror --fail-on-incomplete-format -n ./module.cpp ./module.h
	$(PRETTIER) $(TS_SOURCES) --check --config $(TS_DIR)/.prettier.json

clean:
	rm -rf *.o module.a $(DIST_DIR)

clean-all: clean
	# Also removes the installed JS dependencies in ts/node_modules.
	rm -rf $(NODE_MODULES_DIR)

.PHONY: all clean clean-all format check-format
