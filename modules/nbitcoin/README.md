# NBitcoin module

Upstream: [NBitcoin](https://github.com/MetacoSA/NBitcoin)

## Build

```bash
cd modules/nbitcoin
make
export CXXFLAGS="$CXXFLAGS -DNBITCOIN"
```

## Notes

When fuzzing with NBitcoin, .NET NativeAOT runtime initialization can cause
LSan to report a small false-positive leak from CoreLib's internal
WaitSubsystem. Use the provided suppressions file if this happens:

```bash
LSAN_OPTIONS="suppressions=$(pwd)/lsan.supp" MODULES="NBITCOIN" FUZZ=<target> ./bitcoinfuzz
```
