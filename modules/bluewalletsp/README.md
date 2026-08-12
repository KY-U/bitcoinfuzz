# bluewalletsp module

Upstream: [SilentPayments](https://github.com/BlueWallet/SilentPayments)

## How it runs

The library needs a real Node runtime: its secp256k1 backend, `tiny-secp256k1`,
is a WebAssembly build, and its dependency graph uses Node builtins. Rather than
shim any of that to fit an embedded engine, `module.cpp` starts one `node`
process and talks to it over a pipe, one request line and one response line per
input. The runner is started lazily and kept for the whole session, so process
startup and the WASM instantiation are paid once rather than per input.

Every dependency is therefore the one upstream ships, unmodified. The bundler
compiles `silent-payments` only because it publishes its entry point as raw
TypeScript, which Node refuses to load from `node_modules`; `tiny-secp256k1`
stays external so it can find its `.wasm` file, and the Node builtins are left
alone.

`node` must be on `PATH` **at runtime**, not just at build time.

## Build

```bash
cd modules/bluewalletsp
make
export CXXFLAGS="$CXXFLAGS -DBLUEWALLET_SP"
```