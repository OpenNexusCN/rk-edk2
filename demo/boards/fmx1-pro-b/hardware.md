# FMX1 Pro（B 版）硬件资料

- 来源：`rk-linux-6.18.y` `arch/arm64/boot/dts/rockchip/rk3399-fmx1-pro-b.dts`
- 板配置：`one-kvm.conf`（one-kvm-build）
- SoC：RK3399（B 版设备树引入 `rk3399-op1.dtsi`）
- 内存：4GB
- 存储：**板载 32GB eMMC + 1 个 TF（microSD）卡座**

## 与 A 版的差异（以 DTS 为准）

| 项 | A 版 | B 版 |
|---|---|---|
| 设备树 | `rk3399-fmx1-pro.dts` | `rk3399-fmx1-pro-b.dts` |
| 基础 dtsi | `rk3399.dtsi` | `rk3399.dtsi` + `rk3399-op1.dtsi` |
| model | `fmx1_pro Board` | `RK3399 FMX1` |
| compatible | `rockchip,fmx1_pro` | `rk3399,fmx1` |
| ramoops | 有（`0x110000-0x200000`） | 无 |
| fiq-debugger | 有 | 无 |
| CMA | 128MB | 无显式节点 |
| GMAC 时钟 | 外部 125MHz `clkin_gmac` | 无显式节点 |
| USB host VBUS 引脚名 | `host_vbus_drv` | `vcc5v0_host_en` |
| 其他外设 | eMMC/TF/USB/HDMI/RK808 基本相同 | 同左 |

## 固件处理

- UEFI 驱动与 A 版共用，仅 `Fmx1ProB.dsc` 选择不同 DTB。
- B 版若个别单元无法启动，优先核对 DDR/CPU OPP 差异；UEFI 阶段不修改 CPU 电压/频率。
- 是否需要在运行时区分 A/B：本期不做；可用 ADC/GPIO/EEPROM 预留 `RockchipBoardDetect()`。

## 共性硬件

串口、eMMC/TF、USB、HDMI、PMIC、按键等与 A 版一致，详见
[`../fmx1-pro-a/hardware.md`](../fmx1-pro-a/hardware.md)。
