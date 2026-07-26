#!/bin/sh
set -eu

if [ -z "${FUZZ:-}" ]; then
  echo "FUZZ environment variable must be set" >&2
  exit 1
fi

AFL_TARGET_ROOT="${AFL_TARGET_ROOT:-/app/data/${FUZZ}}"
AFL_INPUT_DIR="${AFL_INPUT_DIR:-${AFL_TARGET_ROOT}/in}"
AFL_OUTPUT_DIR="${AFL_OUTPUT_DIR:-${AFL_TARGET_ROOT}/out}"
AFL_AUTORESUME="${AFL_AUTORESUME:-1}"

mkdir -p "${AFL_INPUT_DIR}" "${AFL_OUTPUT_DIR}"

if [ -z "$(ls -A "${AFL_INPUT_DIR}" 2>/dev/null)" ]; then
  printf 'A' > "${AFL_INPUT_DIR}/seed"
fi

if [ "${AFL_AUTORESUME}" = "1" ] && [ -d "${AFL_OUTPUT_DIR}/default" ]; then
  set -- -i- -o "${AFL_OUTPUT_DIR}" "$@"
else
  set -- -i "${AFL_INPUT_DIR}" -o "${AFL_OUTPUT_DIR}" "$@"
fi

exec afl-fuzz "$@" -- /app/bitcoinfuzz
