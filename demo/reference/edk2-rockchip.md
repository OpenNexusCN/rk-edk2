# 现代 edk2-rockchip 架构要点

参考：`edk2-porting/edk2-rk3588` 内置的 `edk2-rockchip`（Rockchip 维护）。

## 分层

```text
Silicon/Rockchip/                 # 公共库/驱动 + 协议
Silicon/Rockchip/RK3588/          # SoC 实现、ACPI、FDT、PlatformLib
Silicon/Synopsys/DesignWare/      # 通用 IP（GMAC 等）
Platform/<Vendor>/<Board>/        # 板级 DSC/FDF、BoardLib、DSDT、DTB、Logo
```

## 关键做法

1. **PEI-less SEC**：FV 里放 `ArmPlatformPkg/PeilessSec/PeilessSec.inf`，
   由 `ArmPlatformLib` 提供 MMU/HOB/MP core 信息；不再使用旧的
   `ArmPlatformPkg/PrePi/PeiUniCore.inf`（现代 edk2 已移除）。
2. **公共 DSC/FDF 片段**：
   - `Silicon/Rockchip/Rockchip.dsc.inc`：库类、PCD、通用模块；
   - `Silicon/Rockchip/<SoC>/<SoC>Platform.dsc.inc`：SoC 库与驱动；
   - 板级 DSC 只 include 上述文件并覆盖板级 PCD/BoardLib。
3. **板级库**：`Platform/<Vendor>/<Board>/Library/RockchipPlatformLib`
   实现 `RockchipPlatformLib`，负责引脚复用、稳压器、USB/PCIe/显示路由。
4. **ACPI/DT 双模式**：板级配置 PCD 选择 `ACPI`/`DT`/`Both`；
   `AcpiPlatformDxe` 安装 ACPI 表，`FdtPlatformDxe` 安装嵌入的 DTB 配置表。
5. **显示**：`Silicon/Rockchip/Drivers/LcdGraphicsOutputDxe` 产出 GOP；
   `LcdPlatformLib` 封装 VOP2/HDMI/DP/DSI 细节。
6. **存储**：`DwcSdhciDxe`/`RkSdmmcDxe` 产出块设备；
   `EmbeddedPkg/Universal/MmcDxe` 统一 MMC 主机协议。
7. **变量**：`RkFvbDxe` + `BaseVariableLib`，变量存 SPI NOR 或保留分区。
8. **构建**：`build.sh --device <board>` + `configs/<board>.conf`；
   GitHub Actions 使用同一脚本矩阵构建。

## 对 rk-edk2 的取舍

- 采用相同分层与 `PeilessSec`、`LcdGraphicsOutputDxe`、`EmbeddedPkg/MmcDxe`。
- 不照搬 RK3588 的 SCMI/RK806/PCIe 复杂逻辑，RK3399 用 CRU + RK808。
- RK3399 的 eMMC 是 Arasan SDHCI，需要独立的 `SdhciHostDxe`；
  RK3588 的 `DwcSdhciDxe` 可作 SDHCI 寄存器层参考。
- 现代 edk2 已移除 `MdeModulePkg/Bus/Pci/OhciDxe`，如需 OHCI 从
  edk2-rockchip 的 `Silicon/Rockchip/Drivers/OhciDxe` 移植。
