#!/usr/bin/env bash
#
# Compile the board DTB from the vendored reference DTS in demo/.
#
# Usage: scripts/gen-dtb.sh <board-id> [output-dir]
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOARD="${1:?usage: gen-dtb.sh <board-id> [output-dir]}"
OUT_DIR="${2:-${ROOT}/out/${BOARD}}"

CONF="${ROOT}/configs/boards/${BOARD}.conf"
if [[ ! -f "${CONF}" ]]; then
  echo "unknown board: ${BOARD}" >&2
  exit 1
fi
# shellcheck disable=SC1090
source "${CONF}"

DTS="${ROOT}/${DTS}"
if [[ ! -f "${DTS}" ]]; then
  echo "DTS not found: ${DTS}" >&2
  exit 1
fi

for tool in cpp dtc; do
  command -v "${tool}" >/dev/null || { echo "missing tool: ${tool}" >&2; exit 1; }
done

mkdir -p "${OUT_DIR}"

CPP_FLAGS=(
  -nostdinc
  -undef
  -x assembler-with-cpp
  -I "${ROOT}/demo/soc/rk3399"
  -I "${ROOT}/demo/soc/rk3399/dts"
  -I "${ROOT}/demo/boards/${BOARD}"
)

PREPROCESSED="${OUT_DIR}/${BOARD}.dts.pre"
OUT_DTB="${OUT_DIR}/${DTB_NAME}"

cpp "${CPP_FLAGS[@]}" "${DTS}" -o "${PREPROCESSED}"
dtc -I dts -O dtb -@ -o "${OUT_DTB}" "${PREPROCESSED}"

echo "DTB: ${OUT_DTB}"
