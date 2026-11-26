#!/usr/bin/env bash
#
# Guard against code/scripts that program eFuse/OTP or enable Secure Boot.
#
# The CPU "insurance" fuses on Rockchip SoCs are one-time programmable. Once
# burned, MAC/serial/key provisioning may become impossible. This firmware and
# its build must never touch them.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# eFuse/OTP write paths and Rockchip signing/fuse tools. Word boundaries keep
# clock-gate names such as PCLK_EFUSE1024NS out of the results.
PATTERN='(^|[^A-Za-z0-9_])(efuse|otp)[^A-Za-z0-9_]*(write|burn|program|lock)|rk_sign_tool|SecureBootConsole|secure[_-]?boot'

HITS="$(
  grep -RInE \
    --exclude=check-no-fuse-lock.sh \
    --exclude-dir=.git \
    --exclude-dir=demo \
    --exclude-dir=edk2 \
    --exclude-dir=Build \
    --exclude-dir=out \
    "${PATTERN}" \
    "${ROOT}/Silicon" "${ROOT}/Platform" "${ROOT}/scripts" \
    "${ROOT}/configs" "${ROOT}/.github" 2>/dev/null || true
)"

if [[ -n "${HITS}" ]]; then
  echo "ERROR: eFuse/OTP/SecureBoot programming reference found:" >&2
  echo "${HITS}" >&2
  echo >&2
  echo "This firmware must never burn CPU fuses or lock OTP." >&2
  exit 1
fi

echo "OK: no eFuse/OTP/SecureBoot programming references in firmware or CI."
