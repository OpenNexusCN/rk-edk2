#!/usr/bin/env bash
#
# Build one board firmware image.
#
# Usage: scripts/build.sh <board-id> [DEBUG|RELEASE]
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOARD="${1:?usage: build.sh <board-id> [DEBUG|RELEASE]}"
TARGET="${2:-DEBUG}"

CONF="${ROOT}/configs/boards/${BOARD}.conf"
if [[ ! -f "${CONF}" ]]; then
  echo "unknown board: ${BOARD}" >&2
  exit 1
fi
# shellcheck disable=SC1090
source "${CONF}"

"${ROOT}/scripts/ci/setup.sh"

export PACKAGES_PATH="${ROOT}:${ROOT}/edk2"
export GCC_AARCH64_PREFIX="${GCC_AARCH64_PREFIX:-aarch64-linux-gnu-}"
export EDK_TOOLS_PATH="${ROOT}/edk2/BaseTools"
export WORKSPACE="${ROOT}"
export PYTHON_COMMAND="${PYTHON_COMMAND:-python3}"

# edksetup.sh is not `set -u` clean.
set +u
# shellcheck disable=SC1091
source "${ROOT}/edk2/edksetup.sh" BaseTools >/dev/null
set -u

build -a AARCH64 -t GCC -b "${TARGET}" -n "$(nproc)" -p "${DSC}"

"${ROOT}/scripts/gen-dtb.sh" "${BOARD}" "${ROOT}/out/${BOARD}"

echo "Firmware: ${ROOT}/Build/$(basename "${DSC}" .dsc)/${TARGET}_GCC/FV/"
