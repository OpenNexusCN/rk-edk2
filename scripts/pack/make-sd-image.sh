#!/usr/bin/env bash
#
# Assemble a raw SD/eMMC image with the RK3399 firmware layout.
#
# Usage: make-sd-image.sh <idbloader> <loader2> <trust> <output.img>
#
set -euo pipefail

IDB="${1:?usage: make-sd-image.sh <idbloader> <loader2> <trust> <output.img>}"
LOADER2="${2:?}"
TRUST="${3:?}"
OUT="${4:?}"

IDB_OFFSET=0x00008000      # sector 64
LOADER2_OFFSET=0x00800000  # sector 16384
TRUST_OFFSET=0x00C00000    # sector 24576
IMAGE_SIZE=0x01000000      # 16MB

for f in "${IDB}" "${LOADER2}" "${TRUST}"; do
  [[ -f "${f}" ]] || { echo "missing: ${f}" >&2; exit 1; }
done

for spec in "idbloader:${IDB}:${IDB_OFFSET}" \
            "loader2:${LOADER2}:${LOADER2_OFFSET}" \
            "trust:${TRUST}:${TRUST_OFFSET}"; do
  IFS=: read -r name file offset <<<"${spec}"
  size=$(stat -c%s "${file}")
  case "${name}" in
    idbloader) limit=$((LOADER2_OFFSET)) ;;
    loader2)   limit=$((TRUST_OFFSET)) ;;
    trust)     limit=$((IMAGE_SIZE)) ;;
  esac
  if (( size > limit - offset )); then
    echo "${name} (${size} bytes) does not fit at 0x$(printf '%x' "${offset}")" >&2
    exit 1
  fi
done

rm -f "${OUT}"
truncate -s "$((IMAGE_SIZE))" "${OUT}"
dd if="${IDB}"     of="${OUT}" bs=512 seek=$((IDB_OFFSET / 512))     conv=notrunc status=none
dd if="${LOADER2}" of="${OUT}" bs=512 seek=$((LOADER2_OFFSET / 512)) conv=notrunc status=none
dd if="${TRUST}"   of="${OUT}" bs=512 seek=$((TRUST_OFFSET / 512))   conv=notrunc status=none

echo "Image: ${OUT} ($(stat -c%s "${OUT}") bytes)"
