# FMX1 Pro（A 版）硬件资料

- 来源：`rk-linux-6.18.y` `arch/arm64/boot/dts/rockchip/rk3399-fmx1-pro.dts`
- 板配置：`one-kvm.conf`（one-kvm-build）
- SoC：RK3399
- 内存：4GB
- 存储：**板载 32GB eMMC + 1 个 TF（microSD）卡座**
- 启动介质：eMMC 或 TF 卡均可（BootROM 由 strapping 选择）

## 串口

| 项 | 值 |
|---|---|
| 控制器 | UART2 `0xFF1A0000`，IRQ 100 |
| 参数 | `1500000 8N1` |
| 引脚 | GPIO4_C3（RX）/ GPIO4_C4（TX），func 1（`uart2c_xfer`） |
| 用途 | UEFI 控制台、内核 `console=ttyS2,1500000` |

## 存储

| 项 | 值 |
|---|---|
| eMMC | Arasan SDHCI 5.1 `0xFE330000`，IRQ 11，8-bit，non-removable |
| TF 卡 | DW MSHC `0xFE320000`，IRQ 65，4-bit |
| TF 卡检测 | GPIO0_A7，低有效 |
| TF 卡电源 | `vcc3v3_sd` 使能 GPIO0_A1；`vqmmc` 为 `vcc_sdio` |
| SDIO | `0xFE310000`，IRQ 64（UEFI 不使用） |

## 显示

| 项 | 值 |
|---|---|
| VOP | VOPB（大核）`0xFF900000`；VOPL `0xFF8F0000` |
| HDMI | DW-HDMI `0xFF940000` |
| DDC | 与 I2C3 共用 GPIO4_C0/C1（I2C3 `0xFF130000`） |
| 内核配置 | `ddc-i2c-bus = <&i2c3>`；UEFI 可用内部 DDC（func 3）或 I2C3（func 1） |
| 供电 | `vcca1v8_hdmi`、`vcca0v9_hdmi`（RK808 LDO） |

## USB

| 控制器 | 地址 | IRQ | 说明 |
|---|---|---|---|
| EHCI0 | `0xFE380000` | 26 | USB2 host0 |
| OHCI0 | `0xFE3A0000` | 28 | USB1.1 host0 |
| EHCI1 | `0xFE3C0000` | 30 | USB2 host1 |
| OHCI1 | `0xFE3E0000` | 32 | USB1.1 host1 |
| DWC3 OTG0 | `0xFE800000` | 105 | Type-C（FUSB302），`dr_mode=host` |
| DWC3 OTG1 | `0xFE900000` | 110 | `dr_mode=host` |
| PHY | u2phy0/u2phy1、tcphy0/tcphy1 | — | 需 SoC 级时钟/复位/PHY 初始化 |
| VBUS | GPIO4_D1（`vcc5v0_host`）、GPIO1_A3（Type-C） | — | 高有效 |

## 电源 / PMIC

| 项 | 值 |
|---|---|
| PMIC | RK808 @ I2C0 `0xFF3C0000`，地址 `0x1B` |
| DCDC | SYR827 `0x40`（VDD_CPU_B）、SYR828 `0x41`（VDD_GPU） |
| RTC | RK808 内置 |
| 关键电源 | `vcc12v_dc`、`vcc5v0_sys`、`vcc3v3_sys`、`vcc1v8_s0`、`vcc_3v0` |
| LED | GPIO1_B0（`r88:green:led`） |

## 其他

| 项 | 值 |
|---|---|
| GMAC | RGMII，125MHz `clkin_gmac`，复位 GPIO3_B7，`tx_delay=0x28`、`rx_delay=0x11` |
| PCIe | disabled |
| 按键 | ADC 按键（SARADC ch1），电源键 GPIO0_A5 |
| 音频 | S/PDIF、I2S2（UEFI 不使用） |
| WiFi/BT | AP6354（SDIO + UART0，UEFI 不使用） |

## UEFI 启动要点

1. 固件镜像放在 `loader2`（8MB 偏移），`trust.img` 在 12MB，`idbloader` 在 32KB。
2. UEFI 默认 DT 模式，使用本目录 DTS 生成的 DTB。
3. ACPI 模式下：eMMC（`PNP0D40`）、xHCI（`PNP0D15`）、UART 可用；TF 卡（dw_mmc）无 ACPI 绑定。
