# bitcoin-s module

Upstream: [bitcoin-s](https://github.com/bitcoin-s/bitcoin-s)

## Build

```bash
cd modules/bitcoins
make
export CXXFLAGS="$CXXFLAGS -DBITCOINS"
```

## Notes

When fuzzing with BitcoinS, JVM initialization can cause LSan to report
process-lifetime allocations as leaks. If this happens, use the provided
suppression file:

```bash
LSAN_OPTIONS="suppressions=$(pwd)/lsan.supp" MODULES="BITCOINS" FUZZ=<target> ./bitcoinfuzz
```
