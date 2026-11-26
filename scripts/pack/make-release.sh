#!/usr/bin/env bash
#
# Build the release package for one board.
#
# Usage: scripts/pack/make-release.sh <board-id> <version> [output-dir]
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BOARD="${1:?usage: make-release.sh <board-id> <version> [output-dir]}"
VERSION="${2:?usage: make-release.sh <board-id> <version> [output-dir]}"
OUT_DIR="${3:-${ROOT}/artifacts/${BOARD}}"

CONF="${ROOT}/configs/boards/${BOARD}.conf"
if [[ ! -f "${CONF}" ]]; then
  echo "unknown board: ${BOARD}" >&2
  exit 1
fi
# shellcheck disable=SC1090
source "${CONF}"

PLATFORM="$(basename "${DSC}" .dsc)"
FD="$(find "${ROOT}/Build/${PLATFORM}" -path '*/FV/*.fd' -type f | head -1)"
DTB="${ROOT}/out/${BOARD}/${DTB_NAME}"
LOADERIMAGE="${ROOT}/demo/upstream/rk3399-edk2/Rk3399Pkg/Tools/loaderimage"
IDBLOADER="${ROOT}/demo/upstream/rk3399-edk2/Rk3399Pkg/Tools/Bin/idbloader.img"
TRUST="${ROOT}/demo/upstream/rk3399-edk2/Rk3399Pkg/Tools/Bin/trust.img"

for f in "${FD}" "${DTB}" "${LOADERIMAGE}" "${IDBLOADER}" "${TRUST}"; do
  if [[ ! -f "${f}" ]]; then
    echo "missing input: ${f}" >&2
    exit 1
  fi
done

chmod +x "${LOADERIMAGE}"
"${ROOT}/scripts/gen-dtb.sh" "${BOARD}" "${ROOT}/out/${BOARD}" >/dev/null

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK}"' EXIT
PKG="${WORK}/rk-edk2-v${VERSION}-${BOARD}"
mkdir -p "${PKG}" "${OUT_DIR}"

cp "${FD}" "${PKG}/uefi.fd"
cp "${DTB}" "${PKG}/${DTB_NAME}"
cp "${IDBLOADER}" "${PKG}/idbloader.img"
cp "${TRUST}" "${PKG}/trust.img"

"${LOADERIMAGE}" --pack --uboot "${FD}" "${PKG}/uefi-loader2.img" 0x200000 \
  >"${WORK}/loaderimage.log"

"${ROOT}/scripts/pack/make-sd-image.sh" \
  "${PKG}/idbloader.img" "${PKG}/uefi-loader2.img" "${PKG}/trust.img" \
  "${PKG}/rk-edk2-v${VERSION}-${BOARD}-sd.img"

gzip -9 -c "${PKG}/rk-edk2-v${VERSION}-${BOARD}-sd.img" \
  > "${OUT_DIR}/rk-edk2-v${VERSION}-${BOARD}-sd.img.gz"
rm -f "${PKG}/rk-edk2-v${VERSION}-${BOARD}-sd.img"

cat > "${PKG}/README.txt" <<EOF
rk-edk2 ${VERSION} - FMX1 Pro A (${BOARD})
================================================

重要：这是 P0 固件，仅用于串口启动验证。
- 无 HDMI 显示、无 TF/eMMC 块设备、无 ACPI/DT、无 USB
- 不能启动 GRUB / Ubuntu / Windows
- 串口：UART2，1500000 8N1

文件
----
uefi.fd                  UEFI FD（loader2 内容）
uefi-loader2.img         loaderimage --uboot 打包后的 loader2
${DTB_NAME}              FMX1 设备树（P1 使用）
idbloader.img            DDR + miniloader
trust.img                BL31
rk-edk2-v${VERSION}-${BOARD}-sd.img
                         16MB 裸镜像（见下）

烧写到 TF 卡
------------
方式一：整盘镜像
  sudo dd if=rk-edk2-v${VERSION}-${BOARD}-sd.img of=/dev/sdX bs=4M conv=fsync
  （/dev/sdX 是整张卡，不是分区；会清空卡上数据）

方式二：分步写入（偏移单位：字节）
  0x00008000  idbloader.img
  0x00800000  uefi-loader2.img
  0x00C00000  trust.img

  例：
  sudo dd if=idbloader.img     of=/dev/sdX bs=512 seek=64    conv=notrunc
  sudo dd if=uefi-loader2.img  of=/dev/sdX bs=512 seek=16384 conv=notrunc
  sudo dd if=trust.img         of=/dev/sdX bs=512 seek=24576 conv=notrunc

eMMC（可选，使用 rkdeveloptool，偏移单位：扇区）
  rkdeveloptool db  idbloader.img
  rkdeveloptool ul  idbloader.img
  rkdeveloptool wl  0x4000 uefi-loader2.img
  rkdeveloptool wl  0x6000 trust.img
  rkdeveloptool rd

串口观察
--------
参数：1500000 8N1，无流控（FMX1 UART2，GPIO4_C3/C4）

预期：
  1. 上电后出现 UEFI DebugLib 日志；
  2. 其中一行类似：
       RK3399: DRAM 3968 MB, SOC_REG2=0x...
  3. 随后加载 DXE，最终出现 UEFI Shell 提示符：
       Shell>

若无输出：
  - 确认 TF 卡从 FMX1 启动（eMMC 上若有其他固件，可能需要清空/切换启动介质）；
  - 确认波特率 1500000；
  - 确认 idbloader/trust/loader2 偏移；
  - 用示波器/逻辑分析仪确认 UART2 TX 有波形。

更多：https://github.com/OpenNexusCN/rk-edk2/blob/main/docs/bringup.md
EOF

cat > "${PKG}/RELEASE_NOTES.md" <<EOF
# rk-edk2 ${VERSION} — FMX1 Pro P0（串口预览）

首个可构建产物。**仅验证 UEFI 启动链与串口控制台，不能启动操作系统。**

## 包含

- RK3399 FMX1 Pro A UEFI 固件（PeilessSec + DXE + UEFI Shell）
- UART2 1500000 8N1 控制台
- DRAM 容量检测（PMU GRF OS_REG2）
- 板级 UART2 引脚复用、TF 电源、USB VBUS 上电
- FMX1 A 设备树（供 P1 使用）
- 16MB TF 卡裸镜像 + 分步烧写说明

## 已知限制

- 无 HDMI/GOP、无块设备、无 ACPI/DT、无 USB、无 RTC。
- 变量为 RAM 模拟，断电丢失。
- 未在实机验证；首次上板请记录串口日志。

## 校验

\`SHA256SUMS\` 列出所有发布资产；压缩包内组件也有校验值。
EOF

# Component checksums (inside the package tarball).
( cd "${PKG}" && sha256sum * > SHA256SUMS )

tar -C "${WORK}" -czf "${OUT_DIR}/rk-edk2-v${VERSION}-${BOARD}.tar.gz" \
  "rk-edk2-v${VERSION}-${BOARD}"

cp "${PKG}/README.txt" "${PKG}/RELEASE_NOTES.md" "${OUT_DIR}/"

( cd "${OUT_DIR}" && sha256sum * > SHA256SUMS )

echo "Release package written to ${OUT_DIR}"
ls -lh "${OUT_DIR}"
