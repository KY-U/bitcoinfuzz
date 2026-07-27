#!/bin/bash
# rename_rust_secp_symbols.sh - Rename the vendored libsecp256k1 symbols in a Rust
# module archive to avoid duplicate symbol conflicts when linking multiple Rust
# modules together.
#
# Usage: rename_rust_secp_symbols.sh <archive> <prefix>
#
# Rust crates depending on secp256k1-sys statically embed their own copy of
# libsecp256k1. Its C symbols are namespaced by the crate version rather than by
# the consuming crate, e.g. secp256k1-sys 0.10.1 exports
# rustsecp256k1_v0_10_0_context_create. Two modules that pin the same
# secp256k1-sys version therefore export byte-identical symbols, and linking both
# into one binary fails with "multiple definition".
#
# This script renames every rustsecp256k1_* symbol to PREFIX_<original_name>
# using objcopy --redefine-syms. Both the definitions and all references to them
# are rewritten, so each module keeps a self-contained copy of libsecp256k1 under
# its own namespace.
#
# Requires: nm, objcopy (override with the OBJCOPY environment variable), ranlib

set -euo pipefail

ARCHIVE="${1:-}"
PREFIX="${2:-}"

if [ -z "$ARCHIVE" ] || [ -z "$PREFIX" ]; then
    echo "Usage: $0 <archive> <prefix>" >&2
    exit 1
fi

if [ ! -f "$ARCHIVE" ]; then
    echo "Error: Archive '$ARCHIVE' not found" >&2
    exit 1
fi

OBJCOPY="${OBJCOPY:-objcopy}"
if ! command -v "$OBJCOPY" >/dev/null 2>&1; then
    echo "Error: $OBJCOPY not found. Install with: apt-get install binutils" >&2
    exit 1
fi

WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT
MAP_FILE="$WORK_DIR/rust_secp.map"

# Collect every rustsecp256k1_* symbol in the archive, including undefined ones:
# a reference from one member to a definition in another must be renamed too, or
# the archive stops linking internally. The version component is matched
# generically (v0_10_0, v0_12_, ...) so a secp256k1-sys bump needs no change here.
# Read the symbol table up front. nm's exit status is not a reliable signal (it
# reports non-zero for members that carry no symbols), so distinguish the two
# cases by output instead: no output at all means the archive could not be read,
# which must fail loudly rather than silently skip the rename.
SYMBOLS="$(nm "$ARCHIVE" 2>/dev/null || true)"
if [ -z "$SYMBOLS" ]; then
    echo "Error: no symbols read from '$ARCHIVE'; is it a valid archive?" >&2
    exit 1
fi

# The `|| true` keeps a no-match grep from tripping `pipefail`, so an archive
# without vendored secp256k1 symbols reports cleanly instead of failing the build.
printf '%s\n' "$SYMBOLS" \
    | awk '{ print $NF }' \
    | { grep '^rustsecp256k1' || true; } \
    | sort -u \
    | while IFS= read -r sym; do
        printf '%s %s_%s\n' "$sym" "$PREFIX" "$sym"
    done > "$MAP_FILE"

if [ ! -s "$MAP_FILE" ]; then
    echo "No rustsecp256k1 symbols found in $(basename "$ARCHIVE"), nothing to rename."
    exit 0
fi

COUNT=$(wc -l < "$MAP_FILE" | tr -d ' ')
echo "Renaming $COUNT rustsecp256k1 symbols with prefix '${PREFIX}_' in $(basename "$ARCHIVE") ..."

# objcopy rewrites each archive member in place, preserving the member layout.
"$OBJCOPY" --redefine-syms="$MAP_FILE" "$ARCHIVE"
ranlib "$ARCHIVE"

echo "Done. rustsecp256k1 symbols in $(basename "$ARCHIVE") now prefixed with '${PREFIX}_'."
