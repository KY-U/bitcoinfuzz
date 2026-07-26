# RUNNING

## Environment Preparation

Install Docker with the `docker compose` plugin, jq, and just before interacting with this repository:

```bash
# macOS (Homebrew)
brew install docker jq just

# Ubuntu / Debian
sudo apt-get update
sudo apt-get install -y docker.io jq just

docker --version
docker compose version
jq --version
just --version
```

## Environment variables

Create a project-local environment file to control libFuzzer flags:

```bash
cp .env.example .env
# edit LIBFUZZ_* entries as needed
```

Consult the upstream [libFuzzer](https://llvm.org/docs/LibFuzzer.html#options) docs to better understand the options: 

## Run Options

### Option 1 – Quick run via `just`

```bash
just run <fuzz target>

# e.g.
just run script
just run descriptor_parse
```

> [!NOTE]
> Each target relies on a writable `/app/data`. The default compose configuration binds `./docker` on the host. Ensure it exists (`mkdir -p docker`) or adjust the volume mapping before running.

### Option 2 – Custom run via Docker Compose

1. Modify `docker-compose.yml` (build args, env vars, new services, cpu quota, etc.).
2. Launch services:

```bash
docker compose up <fuzz target | compose service> --build --force-recreate

# e.g.
docker compose up script --build --force-recreate
docker compose up descriptor_parse miniscript_parse --build

# later, to stop and remove containers
docker compose down
```

> [!NOTE]
> Keep the `./docker:/app/data` bind (or swap for a named volume like `docker volume create bitcoinfuzz-data`) so
 crashes/corpora persist on the host.

### Option 3 – Manual `docker run`

```bash
mkdir docker
# See auto_build.py and Dockerfile to understand better the build args
docker build -t bitcoinfuzz:script \
  --build-arg FUZZ=script \
  --build-arg "CXXFLAGS=-DBITCOIN_CORE -DRUST_BITCOIN" .

docker run --rm \
  --name bitcoinfuzz-script \
  -e FUZZ=script \
  -e LIBFUZZ_TIMEOUT=300 \
  -v "$(pwd)/docker":/app/data \
  bitcoinfuzz:script

# You can also limit resources to the container
docker run --rm \
    --name bitcoinfuzz-transaction_eval \
    --env-file .env \ # use the .env file
    -v "$(pwd)/docker":/app/data # bind a volume
    --cpus 2 \ # limit cpus
    --memory 2g \ # limit memory
    --net none \ # disable internet connection
    bitcoinfuzz:transaction_eval
```

> [!NOTE]
> Replace the bind mount with a named volume if preferred: `docker run ... -v bitcoinfuzz-data:/app/data ...`. Be aware that folder will be created.

### Selective Module Loading

You can use the `MODULES` environment variable to load only specific modules at runtime, without rebuilding the image:

```bash
# Load only Bitcoin Core and rust-bitcoin
docker run --rm \
  -e MODULES="BITCOIN_CORE,RUST_BITCOIN" \
  -v "$(pwd)/docker":/app/data \
  bitcoinfuzz:script

# Load only Lightning implementations
docker run --rm \
  -e MODULES="LDK,LND,CLIGHTNING" \
  -v "$(pwd)/docker":/app/data \
  bitcoinfuzz:deserialize_invoice

# With docker compose
MODULES="BITCOIN_CORE,RUST_BITCOIN" docker compose up deserialize_block
```

### Option 4 – Running from Source

To build outside of Docker (useful for local debugging), execute `auto_build.py` with the required flags:

```bash
# pass in the modules you want to compile with -D<mod> in CXXFLAGS
CXXFLAGS="-DBITCOIN_CORE -DRUST_BITCOIN" ./auto_build.py
```

### Option 5 - AFL++ in Docker

The repository also ships an AFL++ image that builds the same target/module set
using `afl-clang-fast` and runs `afl-fuzz` inside the container.

```bash
mkdir -p docker-afl

docker build -f Dockerfile.afl -t bitcoinfuzz-afl:verify_script \
  --build-arg FUZZ=verify_script \
  --build-arg "CXXFLAGS=-DBITCOIN_CORE -DBTCD -DGOCOIN -DNBITCOIN -DLIBBITCOIN_SYSTEM" .

docker run --rm -it \
  -e FUZZ=verify_script \
  -e MODULES="BITCOIN_CORE,BTCD,LIBBITCOIN_SYSTEM" \
  -e AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
  -v "$(pwd)/docker-afl":/app/data \
  bitcoinfuzz-afl:verify_script
```

On many Linux hosts, AFL++ aborts if the kernel `core_pattern` pipes crashes to
an external utility. For container-based local fuzzing, the practical fix is
usually:

```bash
-e AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
```

If you want the stricter host-side fix instead, run:

```bash
echo core | sudo tee /proc/sys/kernel/core_pattern
```

By default the container stores AFL++ state under `/app/data/<target>/`:

- seeds: `/app/data/<target>/in`
- queue, crashes, hangs, stats: `/app/data/<target>/out`

If the input directory is empty, the entrypoint creates a single seed file
automatically. Resume is enabled by default via `AFL_AUTORESUME=1`; if
`/app/data/<target>/out/default` already exists, the container runs `afl-fuzz`
with `-i-` and continues the previous session.

You can pass additional AFL++ flags after the image name:

```bash
docker run --rm -it \
  -e FUZZ=verify_script \
  -v "$(pwd)/docker-afl":/app/data \
  bitcoinfuzz-afl:verify_script \
  -m none -t 2000+
```

This script cleans and builds modules based on `CXXFLAGS`, and then compiles the root project.

### Miniscript parser module pairings

For `miniscript_parse`, use **Bitcoin Core** and **Rust-Miniscript** together when you want a stricter
conformance-oriented comparison:

```bash
MODULES="BITCOIN_CORE,RUST_MINISCRIPT" FUZZ=miniscript_parse ./bitcoinfuzz ...
```

Use *Bitcoinerlab Miniscript* and *NBitcoin* together when you want to fuzz more
permissive miniscript parsers:

```bash
MODULES="BITCOINERLAB_MINISCRIPT,NBITCOIN" FUZZ=miniscript_parse ./bitcoinfuzz ...
```

Bitcoinerlab Miniscript and NBitcoin accept a broader set of miniscript strings
than Bitcoin Core and rust-miniscript. For example, they may accept arbitrary key names or
forms that stricter parsers reject at the parsing boundary. Because of that,
accept/reject mismatches between `BITCOIN_CORE` and `BITCOINERLAB_MINISCRIPT`
or `NBITCOIN` are often expected parser-policy differences rather than bugs in
the miniscript implementation. Pairing modules with similar permissiveness makes
crashes and result mismatches easier to interpret.
