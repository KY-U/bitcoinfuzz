# electrum module

Upstream: [Electrum](https://github.com/spesmilo/electrum)

## Dependencies

To run the fuzzer with the `electrum` module, you need to install the
`electrum` library:

```bash
pip install -r modules/electrum/requirements.txt
```

## Supported targets

- `bip32_deserialize_extended_key`

## Build

```bash
make -C modules/electrum
export CXXFLAGS="$CXXFLAGS -DELECTRUM"
make
```
