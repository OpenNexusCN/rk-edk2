# P0 上板验证清单

> P0 已完成本地构建（edk2-stable202608 / GCC / DEBUG），**尚未在 FMX1 实机验证**。
> 本文记录首次上板的打包、烧写与观察步骤。

## 0. 使用 Release 预编译包（推荐）

从 GitHub Releases 下载 `v0.0.1`：

| 资产 | 用途 |
|---|---|
| `rk-edk2-v0.0.1-fmx1-pro-a-sd.img.gz` | 16MB 裸镜像，直接写 TF 卡 |
| `rk-edk2-v0.0.1-fmx1-pro-a.tar.gz` | 分组件包（fd/loader2/dtb/idbloader/trust/README） |
| `SHA256SUMS` | 校验 |

```bash
gzip -dc rk-edk2-v0.0.1-fmx1-pro-a-sd.img.gz | \
  sudo dd of=/dev/sdX bs=4M conv=fsync status=progress
sync
```

> `/dev/sdX` 必须是整张 TF 卡，不是分区；该镜像没有分区表，会清空卡上数据。
> 这是 P0 固件，**不能启动 GRUB/Ubuntu/Windows**。

串口按第 4 节连接，上电观察。

## 1. 从源码构建

```bash
scripts/build.sh fmx1-pro-a DEBUG
```

| 产物 | 路径 |
|---|---|
| UEFI FD | `Build/Fmx1ProA/DEBUG_GCC/FV/RK3399_UEFI.fd` |
| DTB | `out/fmx1-pro-a/rk3399-fmx1-pro.dtb` |
| 构建日志 | `/tmp/buildscript*.log` 或 CI 的 `build-fmx1-pro-a.log` |

## 2. 打包 loader2

P0 尚未集成 Python 打包器，使用 Rockchip `loaderimage`：

```bash
# 任选其一：rkbin/tools/loaderimage 或 demo/upstream 中的参考工具
./loaderimage --pack --uboot \
  Build/Fmx1ProA/DEBUG_GCC/FV/RK3399_UEFI.fd \
  out/fmx1-pro-a/uefi-loader2.img
```

`trust.img` 使用 `demo/blobs/rkbin/RK3399_NO_BL32.ini` 与
`demo/blobs/rkbin/rk33/rk3399_bl31_v1.36.elf` 生成（`trust_merger`），
或直接使用 demo 中已验证的 `trust.img`。

## 3. 烧写到 TF 卡（推荐先用 TF）

> 会覆盖 TF 卡数据。建议先备份。

```bash
# 以 /dev/sdX 为目标卡，务必确认设备名
# 1) idbloader（DDR + miniloader 合成镜像）
sudo dd if=demo/upstream/rk3399-edk2/Rk3399Pkg/Tools/Bin/idbloader.img \
        of=/dev/sdX bs=512 seek=64 conv=notrunc
# 2) UEFI loader2（8MB 偏移）
sudo dd if=out/fmx1-pro-a/uefi-loader2.img \
        of=/dev/sdX bs=512 seek=16384 conv=notrunc
# 3) trust.img（BL31，12MB 偏移）
sudo dd if=trust.img of=/dev/sdX bs=512 seek=24576 conv=notrunc
```

说明：

- `idbloader.img` 必须包含 DDR 初始化 + miniloader；可用
  `boot_merger` 按 `demo/blobs/rkbin/RKBOOT/RK3399MINIALL.ini` 合成，
  或直接使用参考固件中已验证的那一份。
- 更完整的烧写顺序见 `demo/tools/README.md`。

## 4. 串口观察

| 项 | 值 |
|---|---|
| 引脚 | FMX1 调试串口（UART2，GPIO4_C3/C4） |
| 参数 | `1500000 8N1`，无流控 |
| 工具 | `picocom -b 1500000 /dev/ttyUSB0` 或 `minicom` |

预期：

1. 上电后先出现 ATF/miniloader 日志（若保留了它们的输出）；
2. 出现 UEFI DebugLib 输出，其中包含：
   - `RK3399: DRAM ... MB, SOC_REG2=...`
   - PEI/DXE 加载信息；
3. 最终进入 UEFI Shell（提示符 `Shell>`）。

若没有输出：

- 确认串口线序、波特率 1500000；
- 确认 `idbloader` / `trust.img` 写入偏移正确；
- 确认 UART2 引脚复用（`RockchipBoardEarlyInit`）；
- 检查 `trust.img` 的 BL31 是否与 rkbin 版本匹配。

## 5. P0 验收记录

| 项 | 结果 | 备注 |
|---|---|---|
| 串口有输出 | ☐ | |
| DRAM 检测值 | ☐ | 预期约 3968MB |
| 进入 Shell | ☐ | |
| 连续冷启动 10 次 | ☐ | |

## 6. 已知限制

- 无显示输出（P1 实现 GOP/HDMI）。
- 无块设备（P1 实现 TF，P2 实现 eMMC）。
- 无 ACPI/DT 配置表（P1）。
- 无 USB（P2）。
- 变量为 RAM 模拟，断电丢失。
- UART 固定 8N1；`PcdUartDefaultParity/StopBits` 的非零值仅用于满足
  ArmPkg BDS 的静态断言，不改变线格式。

## 7. 安全边界：不熔断 CPU 保险丝

- 固件不读写 eFuse/OTP，不熔断 CPU 保险丝，不启用 Secure Boot。
- 不写保护 FV、eMMC/SPI、vendor storage 或 OTP 区域。
- 目的：始终保留 MAC、序列号、密钥等一次性区域的写入能力。
- CI 通过 `scripts/ci/check-no-fuse-lock.sh` 和
  `scripts/ci/check-fv-locks.py` 强制检查。
