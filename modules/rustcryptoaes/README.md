# rustcryptoaes module

Upstream: [RustCrypto AES](https://github.com/RustCrypto/block-ciphers/tree/master/aes) and [RustCrypto CBC](https://github.com/RustCrypto/block-modes/tree/master/cbc)

Used by the `aes256_cbc` target for differential fuzzing against Bitcoin Core's
`src/crypto/aes.{h,cpp}` (ctaes) implementation.

## Build

```bash
cd modules/rustcryptoaes
make
export CXXFLAGS="$CXXFLAGS -DRUSTCRYPTO_AES"
```
