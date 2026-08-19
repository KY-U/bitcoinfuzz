# bitcoinerlab miniscript module

Upstream: [bitcoinerlab miniscript](https://github.com/bitcoinerlab/miniscript)

## Dependencies

Requires QuickJS installed. By default, the module looks for a complete QuickJS
install under `/usr/local` or `/usr`.

On Debian/Ubuntu:

```bash
sudo apt-get install quickjs libquickjs
```

## Build

```bash
cd modules/bitcoinerlabminiscript
make
export CXXFLAGS="$CXXFLAGS -DBITCOINERLAB_MINISCRIPT"
```
