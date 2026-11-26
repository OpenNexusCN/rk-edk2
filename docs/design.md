# rk-edk2 设计文档（v0.1 草案）

> 目标：为 Rockchip RK 系列板卡提供一套可分层扩展的 EDK2 UEFI 固件，
> 对外提供 UEFI 规范接口、ACPI 表、帧缓冲显示，并加载 `BOOTAA64.EFI`（GRUB 等）。
> **最终目标是在 FMX1 上运行发行版 Ubuntu（arm64）和 Windows（arm64）**。
> 第一阶段只落地 **RK3399 + FMX1 Pro（A 版）**，B 版作为变体预留。
> 构建统一走 GitHub Actions。

---

## 1. 范围与目标

### 1.1 本期目标（RK3399 / FMX1 Pro A）

1. 固件在 FMX1 Pro 上从 eMMC / microSD 启动，串口 UART2 `1500000 8N1` 输出日志。
2. 提供：
   - UEFI 运行时/启动时服务；
   - ACPI（FADT/MADT/GTDT/SPCR/DSDT/SSDT，按需 IORT/PPTT/BGRT）；
   - 嵌入式 DTB + `EFI_DTB_TABLE_GUID` 配置表（Linux 默认走 DT）；
   - GOP 帧缓冲 + 文本控制台 + 开机 Logo；
   - eMMC / SD 块设备 + FAT/GPT；
   - USB 主机（xHCI/EHCI，OHCI 视需要）；
   - RK808 RTC；
   - BDS 启动菜单，自动加载 `\EFI\BOOT\BOOTAA64.EFI`。
3. 构建产物可直接烧录，并提供一键生成 SD 镜像、eMMC 烧写说明。
4. **操作系统目标**：
   - Ubuntu 发行版（arm64，22.04/24.04+）：DT 模式安装、启动、日常使用；
   - Windows（arm64，10/11）：ACPI + GOP 模式启动安装介质与已安装系统；
   - 两套配置表同时安装，由 OS 自行选择（Ubuntu 默认 DT，Windows 使用 ACPI）。

### 1.2 非本期目标

- Windows 下的完整外设驱动（GPU/WiFi/GMAC/音频）：受限于 Rockchip 未提供
  RK3399 的 Windows 驱动，本期只承诺 **UEFI 引导 + ACPI 枚举 + 基本存储/USB/显示**。
- NetBSD / VMware ESXi 等其它 OS。
- PCIe、WiFi/BT、S/PDIF、摄像头、GPU 加速。
- Capsule 固件升级、持久化 UEFI 变量（P0 用 RAM 变量，接口预留）。
- TPM 2.0 / Secure Boot（Windows 11 默认要求，后续阶段评估 fTPM）。
- RK356x / RK3588 的具体实现（只保留分层接口与 SoC 包骨架示例）。

### 1.3 已确认决策（2026-11-26）

1. edk2 基线：`edk2-stable202608`。
2. 本期板卡：FMX1 Pro A；B 版作为同一板级包的变体。
3. 默认 DT 启动；ACPI 同时实现、可切换。
4. eMMC 使用独立的 Arasan SDHCI host 驱动；TF 卡使用 DW MSHC 驱动。
5. 接受移植 GPL-2.0 代码，固件按 GPL-2.0 兼容方式分发。
6. FMX1 硬件：板载 32GB eMMC + 1 个 TF 卡座。
7. 所有板卡参考代码归档到 `demo/`；设计文档在 `docs/` 维护。
8. 目标 OS：发行版 Ubuntu arm64（DT）+ Windows arm64（ACPI）；ACPI/DT 两套表同时安装。

### 1.4 禁止熔断 CPU 保险丝 / 写锁（强制）

- 固件、打包脚本、CI **不得以任何形式写 eFuse/OTP**，不得熔断 CPU 保险丝，
  不得启用 Secure Boot 的 OTP 位。
- 不调用 `rk_sign_tool`、`SecureBootConsole`、`upgrade_tool` 的 fuse 命令等
  写号/签名工具。
- 目的：保留 MAC、序列号、未来密钥等一次性区域的写入能力；eFuse 熔断不可逆。
- FV 属性也不设置 `LOCK_CAP`/`LOCK_STATUS`/`WRITE_LOCK_*`/`READ_LOCK_*`，
  保持固件可写；`WRITE_ENABLED_CAP`、`WRITE_STATUS` 必须为 1。
- 后续实现 eMMC 时，不得设置 boot partition write-protect（BOOT_WP）。
- CI 强制检查：
  - `scripts/ci/check-no-fuse-lock.sh`：源码/脚本/工作流中不得出现 eFuse/OTP
    写入或 Secure Boot 开启引用；
  - `scripts/ci/check-fv-locks.py`：FV 头不得带锁位，且必须可写。

### 1.5 设计原则

- **分层解耦**：SoC 无关 IP 驱动 / SoC 相关时钟与 PHY / 板级数据与上电时序，三层严格单向依赖。
- **板卡可扩展**：新增板卡只新增 `Platform/<Vendor>/<Board>` 目录和一份配置，不改公共驱动。
- **SoC 可扩展**：新增 SoC 只新增 `Silicon/Rockchip/<SoC>` 包，实现统一接口。
- **默认 DT，ACPI 可选**：Linux 下 DT 是完整支持路径；ACPI 作为规范能力与可选启动模式。
- **可复现构建**：edk2、rkbin 全部锁定 commit/tag；CI 不依赖网络上的浮动版本。

---

## 2. 参考代码分析结论

| 参考 | 关键结论 |
|---|---|
| `demo/rk3399-edk2` | 有可用的 RK3399 VOP/HDMI/EDID、DW MMC、DW I2C、RK808 RTC、CRU、串口、ACPI 初版。**但基于 2018 年 edk2，单 Pkg 平铺，无 GOP，USB 只初始化 DWC3 核心**。 |
| `rk-linux-6.18.y` | FMX1 A/B 设备树、RK3399 全量 `dtsi`、CPU/USB/MMC/HDMI/GIC 拓扑的权威来源。 |
| `rkbin` | RK3399 DDR/UsbPlug/miniloader/BL31/BL32 二进制及 `RKBOOT/RK3399MINIALL.ini`、`RKTRUST/RK3399TRUST.ini`。 |
| `rk-tools` | `loaderimage`、`boot_merger`、`trust_merger` 等打包工具来源。 |
| `one-kvm-build` | FMX1 A/B 板配置：RK3399、4GB RAM、32GB eMMC、USB-C OTG、板型差异。 |
| `edk2-rk3588` / `edk2-rockchip` | **现代分层参考**：`Silicon/Rockchip` 公共驱动 + `Silicon/Rockchip/<SoC>` + `Platform/<Vendor>/<Board>`；`PeilessSec`；ACPI/DT 双模式；`LcdGraphicsOutputDxe` + `LcdPlatformLib`；`EmbeddedPkg/Universal/MmcDxe`；GitHub Actions 风格构建。 |
| 现代 edk2（`edk2-stable202608`） | `ArmPlatformPkg/PeilessSec` 替代旧 `PrePi`；`EmbeddedPkg/Universal/MmcDxe` 仍支持多 host；`ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe` 可直接复用；`MdeModulePkg/Bus/Pci/OhciDxe` 已被移除（OHCI 需自带或省略）。 |

### 2.1 demo 中必须修正的问题

1. **帧缓冲地址冲突**：`VopDxe` 把 framebuffer 固定到 `0x0020_0000`，与 FD/FV 基址相同。
   → 改为 DXE 阶段 `AllocatePages` + `EFI_MEMORY_WC` 动态分配并标记保留。
2. **没有 GOP**：demo 只点亮 HDMI，不产生 `EFI_GRAPHICS_OUTPUT_PROTOCOL`，屏幕没有 UEFI 文本/图形。
   → 采用 `LcdGraphicsOutputDxe` + 板级 `LcdPlatformLib`。
3. **“DwEmmcDxe” 实际是 SD 控制器**（`0xFE32_0000`，DW MSHC）；RK3399 的 eMMC 是
   Arasan SDHCI 5.1（`0xFE33_0000`）。→ 拆分为 `DwMmcHostDxe`（SD）与 `SdhciHostDxe`（eMMC）。
4. **USB 未完成**：只对 DWC3 核心做软复位/模式设置，缺少 u2phy/tcphy、时钟、复位、VBUS。
   → 增加 `Rk3399UsbLib` 与 `Dwc3HostDxe` 的 SoC 回调。
5. **ACPI 硬编码**：串口 115200、CPU `_UID` 与 MPIDR 不一致（`0x101/0x102`）、只有 1 个 UEFI 核。
   → 由 SoC 信息结构统一生成；FMX1 用 `1500000`，MPIDR 为 `0x0..0x3, 0x100, 0x101`。
6. **内存预留与 ramoops 冲突**：demo 预留 `0x0-0x200000`，FMX1 DTS 的 ramoops 位于
   `0x110000-0x200000`。→ 只预留 BL31/FV 实际占用区间，或在 DT fixup 中调整 ramoops。
7. **单核限制**：demo `PcdCoreCount=1`，但 DSDT 写了 6 个 CPU。
   → 采用 `PeilessSec` + PSCI 拉起 6 核（参考 RK3588），失败时回退单核。

---

## 3. FMX1 Pro 关键硬件事实（来自 6.18 DTS）

| 项目 | 值 |
|---|---|
| SoC / 内存 / 存储 | RK3399；4GB LPDDR3；32GB eMMC；microSD |
| CPU | 4×Cortex-A53 MPIDR `0x0..0x3`；2×Cortex-A72 `0x100,0x101` |
| GIC | GICv3：GICD `0xFEE0_0000`，GICR `0xFEF0_0000`，GICC `0xFFF0_0000`，ITS `0xFEE2_0000` |
| 串口 | UART2 `0xFF1A_0000`，IRQ 100，`1500000 8N1`，引脚 GPIO4_C3/C4（func 1） |
| eMMC | Arasan SDHCI 5.1 `0xFE33_0000`，IRQ 11，8-bit，不可插拔 |
| SD | DW MSHC `0xFE32_0000`，IRQ 65，4-bit，CD=GPIO0_A7（低有效） |
| SDIO | DW MSHC `0xFE31_0000`，IRQ 64（UEFI 不用） |
| USB2 HOST0/1 | EHCI `0xFE38_0000` IRQ 26 / OHCI `0xFE3A_0000` IRQ 28；<br>EHCI `0xFE3C_0000` IRQ 30 / OHCI `0xFE3E_0000` IRQ 32 |
| USB3 OTG0/1 | DWC3 `0xFE80_0000` IRQ 105、`0xFE90_0000` IRQ 110（xHCI） |
| USB PHY | u2phy0/u2phy1、tcphy0/tcphy1；VBUS 使能 GPIO4_D1；Type-C VBUS GPIO1_A3 |
| 显示 | VOPB（大核）`0xFF90_0000` + DW-HDMI `0xFF94_0000`；VOPL `0xFF8F_0000` 备用 |
| HDMI DDC | 与 I2C3 共用 GPIO4_C0/C1；可用 HDMI 内部 DDC（func 3）或 I2C3（func 1） |
| PMIC | RK808 @ I2C0 `0xFF3C_0000` 地址 `0x1B`；SYR827/828 @ `0x40/0x41` |
| GMAC | RGMII，125MHz 输入时钟，复位 GPIO3_B7（后续阶段） |
| 指示灯 | GPIO1_B0 |
| 板型差异 | A：`rk3399-fmx1-pro.dts`；B：`rk3399-fmx1-pro-b.dts` + `rk3399-op1.dtsi` |

---

## 4. 总体架构

```
┌─────────────────────────────────────────────────────────────┐
│ L4 Board   Platform/<Vendor>/<Board>/                       │
│    BoardLib（引脚/上电/稳压/显示路由/DTB/ACPI OEM）         │
│    DSDT/SSDT、DeviceTree、Logo、DSC/FDF 片段                │
├─────────────────────────────────────────────────────────────┤
│ L3 SoC     Silicon/Rockchip/<SoC>/                          │
│    Rk<Soc>SocLib / ClockLib / IomuxLib / UsbLib / DisplayLib│
│    ArmPlatformLib、SoC ACPI 表、SoC 驱动                    │
├─────────────────────────────────────────────────────────────┤
│ L2 Common  Silicon/Rockchip/                                │
│    DW UART/I2C/MMC、SDHCI、DWC3、VOP/HDMI、ACPI/FDT 框架、  │
│    GOP(LcdPlatformLib)、BDS、变量、协议定义                  │
├─────────────────────────────────────────────────────────────┤
│ L1 Core    edk2/（git submodule，锁定 edk2-stable202608）    │
└─────────────────────────────────────────────────────────────┘
```

依赖方向只能向上：L2 不包含任何 SoC 寄存器地址；L3 不包含任何板级 GPIO/电压；
L4 只描述“这块板子”。

---

## 5. 仓库目录规划

```text
rk-edk2/
├── edk2/                                  # 由 scripts/ci/setup.sh 拉取并锁定 edk2-stable202608
├── Silicon/Rockchip/
│   ├── RockchipPkg.dec                    # 公共 PCD / GUID / 协议
│   ├── Rockchip.dsc.inc                   # 公共库类、PCD、BuildOptions
│   ├── Rockchip.fdf.inc                   # 公共 FV/FD 规则
│   ├── Include/
│   │   ├── Library/RockchipSocLib.h
│   │   ├── Library/RockchipBoardLib.h
│   │   ├── Library/RockchipDisplayLib.h
│   │   ├── Library/RockchipAcpiLib.h
│   │   ├── Library/RockchipFdtLib.h
│   │   └── Protocol/RockchipPlatformProtocol.h
│   ├── Library/
│   │   ├── Dw8250SerialPortLib/           # DW APB UART（含 1.5Mbps）
│   │   ├── DwI2cLib/                      # DW I2C 主机
│   │   ├── Rk808RtcLib/                   # RK808 RTC → EFI_RUNTIME_SERVICES
│   │   ├── RockchipAcpiLib/               # 公共 ACPI 表构造
│   │   ├── RockchipFdtLib/                # DTB 修补 / DT 配置表
│   │   ├── RockchipLcdPlatformLib/        # 实现 LcdPlatformLib，桥接 DisplayLib
│   │   └── PlatformBootManagerLib/        # 启动策略
│   └── Drivers/
│       ├── DwMmcHostDxe/                  # DW MSHC → EFI_MMC_HOST_PROTOCOL
│       ├── SdhciHostDxe/                  # SDHCI 3.0 → EFI_MMC_HOST_PROTOCOL
│       ├── Dwc3HostDxe/                   # DWC3 核心初始化 + xHCI 注册
│       ├── EhciOhciInitDxe/               # USB2 主机 PHY/时钟 + 非发现设备注册
│       ├── VopDxe/                        # VOP 控制器（公共寄存器层）
│       ├── DwHdmiDxe/                     # DW-HDMI TX + EDID
│       ├── PlatformDxe/                   # 平台协议、SMBIOS、板级后初始化
│       ├── AcpiPlatformDxe/               # ACPI 表安装/选择
│       ├── FdtPlatformDxe/                # 嵌入 DTB、fixup、配置表
│       ├── RkFvbDxe/                      # 持久变量（阶段 3）
│       └── LogoDxe/                       # 开机 Logo
├── Silicon/Rockchip/RK3399/
│   ├── RK3399.dec                         # SoC PCD：基址、IRQ、时钟 ID
│   ├── RK3399.dsc.inc                     # SoC 库/驱动装配
│   ├── Include/RK3399.h / RK3399Cru.h / RK3399Grf.h / RK3399Usb.h
│   ├── Library/
│   │   ├── Rk3399SocLib/                  # RockchipSocLib 实现
│   │   ├── Rk3399ClockLib/                # CRU/PLL/门控/复位（移植 demo CRULib）
│   │   ├── Rk3399IomuxLib/                # GRF/PMUGRF 引脚复用
│   │   ├── Rk3399 PlatformLib/            # ArmPlatformLib（MMU/HOB/MP core）
│   │   ├── Rk3399UsbLib/                  # u2phy/tcphy/时钟/复位
│   │   └── Rk3399DisplayLib/              # VOP/HDMI 时钟与引脚
│   ├── Drivers/Rk3399Dxe/                 # SoC 级 DXE 杂项初始化
│   └── AcpiTables/                        # FADT/MADT/GTDT/SPCR 模板
├── Platform/Fmx/Fmx1Pro/
│   ├── Fmx1ProA.dsc / Fmx1ProB.dsc        # A/B 变体，共用 Fmx1Pro.dsc.inc
│   ├── Fmx1Pro.dsc.inc
│   ├── Fmx1Pro.Modules.fdf.inc
│   ├── Library/Fmx1ProBoardLib/           # RockchipBoardLib 实现
│   ├── AcpiTables/Dsdt.asl + AcpiTables.inf
│   ├── DeviceTree/Fmx1Pro.inf             # 嵌入由 demo 参考 DTS 生成的 DTB
│   └── Drivers/LogoDxe/ + logo.bmp
├── configs/boards/fmx1-pro-a.conf         # CI 矩阵输入
├── configs/boards/fmx1-pro-b.conf
├── demo/                                  # 只读参考代码/资料，见 §6
├── scripts/
│   ├── build.sh                           # 本地等价于 CI 的构建
│   ├── gen-dtb.sh                         # 从 demo DTS 生成 DTB
│   ├── pack/
│   │   ├── rk_loader.py                   # loaderimage 兼容实现（阶段 2）
│   │   └── make-sd-image.sh               # 生成可 dd 的整盘镜像
│   └── ci/{setup.sh, build-board.sh, package.sh}
├── tests/                                 # 主机侧单测（GCC + pytest）
├── .github/workflows/build.yml
└── docs/design.md
```

> 说明：`demo/` 存放**所有板卡的参考代码与资料**，是构建的只读输入
> （DTS/DTB、rkbin 二进制、工具说明），不参与固件源码编译；固件代码只在
> `Silicon/`、`Platform/` 下。

---

## 6. `demo/` 参考目录规划

```text
demo/
├── README.md                         # 来源、commit、许可证、校验
├── SOURCES.lock                      # rk-linux / rkbin / edk2 参考 commit
├── upstream/
│   └── rk3399-edk2/                  # 现有 demo，保持只读
├── soc/rk3399/
│   ├── dts/                          # rk3399.dtsi、rk3399-base.dtsi、rk3399-op1.dtsi
│   ├── dt-bindings/                  # DTS 实际引用到的最小头文件子集
│   └── notes.md                      # 基址/IRQ/时钟/启动链笔记
├── boards/
│   ├── fmx1-pro-a/
│   │   ├── rk3399-fmx1-pro.dts
│   │   ├── hardware.md               # 引脚、电源、显示、USB、存储
│   │   └── one-kvm.conf              # 从 one-kvm-build 拷贝
│   └── fmx1-pro-b/
│       ├── rk3399-fmx1-pro-b.dts
│       ├── rk3399-op1.dtsi
│       ├── hardware.md
│       └── one-kvm.conf
├── blobs/rkbin/rk33/                 # RK3399 DDR/miniloader/usbplug/BL31/BL32
├── blobs/rkbin/RKBOOT|RKTRUST/       # 对应 ini（含 NO_BL32）
├── tools/rk-tools/                   # loaderimage/boot_merger/trust_merger 说明
└── reference/edk2-rockchip.md        # 现代分层实现要点摘录（不整仓拷贝）
```

DTS 只拷贝解析所需文件，构建时用 `cpp + dtc` 生成 DTB，保证与内核 6.18 源码一致。

---

## 7. 分层接口设计

### 7.1 公共数据结构（`RockchipPkg.dec` / 头文件）

```c
typedef enum {
  ROCKCHIP_SOC_RK3399 = 0x3399,
  ROCKCHIP_SOC_RK356X = 0x3560,
  ROCKCHIP_SOC_RK3588 = 0x3588,
} ROCKCHIP_SOC_ID;

typedef struct {
  UINT32  SocId;
  UINT32  BoardId;          // 板厂自定义
  UINT32  Variant;          // FMX1: A=0, B=1
  CHAR8   *BoardName;
  CHAR8   *DtbName;
  CHAR8   *AcpiOemId;       // 4 字符，如 "FMX1"
  CHAR8   *AcpiOemTableId;
} ROCKCHIP_BOARD_INFO;

typedef struct {
  UINT8   Id;               // 0=SD, 1=eMMC, ...
  UINTN   Base;
  UINT32  InputClockHz;
  UINT32  MaxBusWidth;
  BOOLEAN Removable;
  INT32   CardDetectGpio;   // -1 = 无
} ROCKCHIP_MMC_DESC;

typedef enum { ROCKCHIP_USB_MODE_HOST, ROCKCHIP_USB_MODE_PERIPHERAL } ROCKCHIP_USB_MODE;
typedef struct {
  UINT8   Id;               // OTG0/OTG1/HOST0/HOST1
  UINTN   Dwc3Base;         // 0 = 无 DWC3
  UINTN   XhciBase;
  UINTN   EhciBase;
  UINTN   OhciBase;
  UINT32  Irq;
  ROCKCHIP_USB_MODE Mode;
} ROCKCHIP_USB_DESC;

typedef enum { ROCKCHIP_CONN_HDMI, ROCKCHIP_CONN_DP, ROCKCHIP_CONN_EDP,
               ROCKCHIP_CONN_DSI, ROCKCHIP_CONN_LVDS } ROCKCHIP_CONNECTOR;
typedef struct {
  UINT8   VopId;            // 0=VOPB, 1=VOPL
  ROCKCHIP_CONNECTOR Connector;
  UINT8   DdcMode;          // 0=HDMI 内部 DDC, 1=外部 I2C
  UINT8   DdcI2cBus;
} ROCKCHIP_DISPLAY_ROUTE;
```

### 7.2 SoC 接口 `RockchipSocLib.h`（L3 实现，L2/L4 调用）

```c
typedef struct {
  UINT64  DramBase;
  UINT64  DramSize;
  UINT32  CpuCount;
  struct { UINT64 Mpidr; UINT8 Cluster; UINT8 Core; } Cpu[8];
  UINT64  GicDBase, GicRBase, GicCBase, GicHBase, GicVBase, GicItsBase;
  UINT32  GicVersion;
  UINT32  TimerSecEl1Gsiv, TimerNonSecEl1Gsiv, TimerVirtGsiv, TimerHypGsiv;
} ROCKCHIP_SOC_INFO;

EFI_STATUS  RockchipSocEarlyInit (VOID);      // SEC/PEI：时钟、串口、DDR 检测
EFI_STATUS  RockchipSocLateInit  (VOID);      // DXE：PHY、显示、USB
VOID        RockchipSocGetInfo   (OUT ROCKCHIP_SOC_INFO *Info);
EFI_STATUS  RockchipSocSetUartClock (UINT8 Id, UINT32 Hz);
EFI_STATUS  RockchipSocSetMmcClock  (UINT8 Id, UINT32 Hz);
EFI_STATUS  RockchipSocSetVopClock  (UINT8 Vop, UINT32 PixelHz);
EFI_STATUS  RockchipSocUsbPhyInit   (UINT8 Id, ROCKCHIP_USB_MODE Mode);
```

SoC 包内再拆 `RockchipClockLib` / `RockchipIomuxLib` / `RockchipUsbLib`，
L2 驱动只通过 PCD 和上述抽象拿资源，不直接写 CRU/GRF。

### 7.3 板级接口 `RockchipBoardLib.h`（L4 实现）

```c
CONST ROCKCHIP_BOARD_INFO *RockchipBoardGetInfo (VOID);
VOID        RockchipBoardEarlyInit (VOID);    // UART iomux、SD/eMMC 电源、USB VBUS
VOID        RockchipBoardLateInit  (VOID);    // 稳压器、LED、网卡复位
UINTN       RockchipBoardGetMmcDescs      (OUT ROCKCHIP_MMC_DESC **Descs);
UINTN       RockchipBoardGetUsbDescs      (OUT ROCKCHIP_USB_DESC **Descs);
UINTN       RockchipBoardGetDisplayRoutes (OUT ROCKCHIP_DISPLAY_ROUTE **Routes);
EFI_STATUS  RockchipBoardFixupFdt  (IN OUT VOID *Fdt);
EFI_STATUS  RockchipBoardPatchDsdt (IN OUT EFI_ACPI_DESCRIPTION_HEADER *Dsdt);
EFI_STATUS  RockchipBoardDetect    (OUT UINT32 *BoardId, OUT UINT32 *Variant); // 预留
```

约定：

- 板级库**只做数据与板级动作**，不出现 SoC 寄存器偏移（用 Iomux/Clock API）。
- 所有板级动作必须幂等，便于 SEC/DXE 重复调用。
- `RockchipBoardDetect` 本期返回编译期常量；将来可做一镜像多板（ADC/GPIO/EEPROM 识别）。

### 7.4 显示接口 `RockchipDisplayLib.h`

```c
typedef struct {
  UINT32  PixelClockHz;
  UINT16  HActive, HFrontPorch, HSync, HBackPorch;
  UINT16  VActive, VFrontPorch, VSync, VBackPorch;
  BOOLEAN HSyncActiveHigh, VSyncActiveHigh;
  UINT8   Bpp;              // 32
} ROCKCHIP_VIDEO_TIMING;

typedef struct {
  EFI_STATUS (*GetTiming) (IN ROCKCHIP_DISPLAY_ROUTE *Route, OUT ROCKCHIP_VIDEO_TIMING *Timing);
  EFI_STATUS (*SetMode)   (IN ROCKCHIP_DISPLAY_ROUTE *Route, IN ROCKCHIP_VIDEO_TIMING *Timing);
  EFI_STATUS (*Enable)    (IN ROCKCHIP_DISPLAY_ROUTE *Route, IN EFI_PHYSICAL_ADDRESS Fb,
                           IN UINT32 Stride, IN UINT8 Bpp);
  EFI_STATUS (*Disable)   (IN ROCKCHIP_DISPLAY_ROUTE *Route);
} ROCKCHIP_DISPLAY_OPS;
```

- `RockchipLcdPlatformLib` 实现 edk2 `LcdPlatformLib`，由
  `ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe` 产出标准 GOP + 文本控制台。
- HDMI 路径：EDID 读取 → 选择模式 → 无 EDID 回退 1920×1080@60。
- 显示路由、VOP 选择、DDC 方式全部来自板级描述。

### 7.5 运行时协议 `RockchipPlatformProtocol.h`

```c
#define ROCKCHIP_PLATFORM_PROTOCOL_GUID ...
typedef struct {
  ROCKCHIP_BOARD_INFO       Info;
  ROCKCHIP_DISPLAY_ROUTE    DisplayRoute;
  UINT32                    ConfigTableMode; // 0=DT, 1=ACPI, 2=Both
} ROCKCHIP_PLATFORM_PROTOCOL;
```

供 Setup UI、BootManager、SMBIOS、未来的固件升级应用查询。

---

## 8. 启动链与镜像布局

### 8.1 BootROM 链（RK3399）

```text
BootROM
  └─ idbloader（rkbin: DDR 初始化 + miniloader）
       ├─ 加载 trust.img（BL31，可选 BL32）→ 0x0004_0000 / 0x0840_0000
       └─ 加载 loader2（UEFI FD，loaderimage 打包）→ 0x0020_0000
            └─ BL31 跳到 UEFI BL33
                 └─ PeilessSec → DXE → BDS → \EFI\BOOT\BOOTAA64.EFI (GRUB)
```

### 8.2 存储布局（eMMC / SD 相同）

| 偏移（扇区） | 偏移（字节） | 内容 | 大小 |
|---|---|---|---|
| 64 | 0x8000 | `idbloader.img` | ≤ 4MB |
| 16384 | 8MB | `uefi-loader2.img` | 4MB 分区 |
| 24576 | 12MB | `trust.img`（BL31） | 4MB 分区 |
| 32768 | 16MB | GPT：ESP / boot（FAT32） | 剩余 |

- ESP 内放 `\EFI\BOOT\BOOTAA64.EFI`（GRUB）、`\dtb\rockchip\*.dtb`、`grub.cfg`。
- 固件自身也可读取 Armbian 现有 boot 分区中的 `BOOTAA64.EFI`，无需重新分区。

### 8.3 FD/FV

| PCD | 值 | 说明 |
|---|---|---|
| `PcdFdBaseAddress` | `0x0020_0000` | loader2 载入地址 |
| `PcdFdSize` | `0x000F_0000` | 960KB，受 `loaderimage --uboot` 的 `0xFF800` 限制 |
| `PcdFvBaseAddress` | `0x0020_0000` | FV 压缩 |
| `PcdFvSize` | `0x000F_0000` | 压缩后约 0.5MB；P1 增加模块后需改用 FIT 或其他打包方式 |

首次不启用持久变量；阶段 3 再增加 FVB 区域（eMMC 保留区或专用分区），
避免与 loader2/trust 分区冲突。

---

## 9. 内存与 ACPI/DT

### 9.1 内存映射（4GB 板）

| 区域 | 地址 | 属性 |
|---|---|---|
| BL31 | `0x0004_0000` + 实际大小 | Reserved |
| UEFI FD/FV | `0x0020_0000 - 0x0060_0000` | Firmware |
| BL32（可选） | `0x0840_0000` + 实际大小 | Reserved |
| DDR 可用 | 其余至 `0xF800_0000` | WB Cacheable |
| 外设 MMIO | `0xF800_0000 - 0xFFFF_FFFF` | Device |
| Framebuffer | 动态分配 | WC + Reserved |

DRAM 容量从 PMU GRF `OS_REG2` 解析（移植 demo `Rk3399Mem.c`），PCD 作为兜底。

### 9.2 ACPI

SoC 通用表（`Silicon/Rockchip/RK3399/AcpiTables`）：

- `FADT`：ARM SBBR，HW_REDUCED_ACPI，PSCI=SMC，无 legacy 设备。
- `MADT`：6 个 GICC（MPIDR `0x0..0x3,0x100,0x101`）、GICD/GICR、GIC ITS。
- `GTDT`：GSIV 29/30/27/26，CNTControlBase=0（无 MMIO 时基）。
- `SPCR`：`0xFF1A_0000`，`1500000`，reg-shift 2，reg-width 4。

板级 DSDT（`Platform/Fmx/Fmx1Pro/AcpiTables/Dsdt.asl`）：

- `CPU0..CPU5`（`ACPI0007`，`_UID` 与 MADT 一致）。
- `XHC0/XHC1`（`PNP0D15`，`_CCA=0`，IRQ 105/110）。
- `EMMC0`（`PNP0D40`，SDHCI/Arasan，IRQ 11）。
- `COM1`（`RKCP8250`/`8250dw`，IRQ 100，SPCR 一致）。
- `RTC0`（RK808）、`SD0`（可选；Linux dw_mmc 无 ACPI 绑定，默认不放）。
- GPIO 按键/LED、GMAC（阶段 3）。

> 事实：Linux 6.18 中 `dw_mmc` 没有 ACPI 绑定，`sdhci-acpi` 支持 `PNP0D40`。
> 因此 ACPI 模式下 eMMC 可工作、SD 不工作；**Linux 默认走 DT 模式**。

### 9.3 DT 模式

- 由 `demo/boards/fmx1-pro-a/rk3399-fmx1-pro.dts` 生成 DTB，嵌入 FV。
- `FdtPlatformDxe`：
  - 安装 `gFdtTableGuid` 配置表；
  - 修补 `/chosen/stdout-path`、`/chosen/bootargs`（可选）；
  - 修补 `/reserved-memory`（framebuffer、BL31/FV 实际区间、ramoops 冲突）；
  - 可选：把 EFI 内存映射信息写入 `/chosen`。
- 模式选择：`PcdConfigTableModeDefault = Both`（同时安装 ACPI 与 DTB）。
  - Ubuntu/GRUB：默认使用 DTB；需要 ACPI 实验时加 `acpi=force`。
  - Windows：自动使用 ACPI，忽略 DTB。
  - Setup 中可切换 ACPI / DT / Both（阶段 P3）。

### 9.4 目标 OS 启动矩阵（Ubuntu arm64 / Windows arm64）

| 能力 | Ubuntu arm64（DT） | Ubuntu arm64（ACPI） | Windows arm64（ACPI） |
|---|---|---|---|
| 启动方式 | GRUB → Image + DTB | GRUB → Image + `acpi=force` | Bootmgr → `\EFI\Microsoft\Boot\bootmgfw.efi` |
| ACPI 表 | 不使用 | 需要 | **必需** |
| GOP | 需要（安装器/GRUB 菜单） | 需要 | **必需** |
| eMMC | dw_mmc（DT） | `PNP0D40` SDHCI | 依赖 Windows SD 主机驱动 |
| TF 卡 | dw_mmc（DT） | 不可用 | 不可用（无 ACPI 绑定） |
| USB | xHCI/EHCI | xHCI/EHCI | **必需**（安装介质/键鼠） |
| 显示 | VOP+HDMI（DT，内核驱动） | 仅固件帧缓冲/ efifb | 仅固件帧缓冲，无 GPU 驱动 |
| RTC | RK808（DT） | RK808（ACPI） | 视驱动 |
| 网络 | GMAC（DT） | 未实现 | 无驱动 |

#### Windows arm64 的现实约束（必须提前说明）

1. **Rockchip 未提供 RK3399 的 Windows 驱动**：GPU（Mali）、GMAC、WiFi/BT、
   音频、PMIC 都没有 Windows 驱动。固件侧只能保证 UEFI + ACPI + GOP +
   基本 USB/SDHCI 枚举；进入 Windows 后大概率只有基本显示和 USB 存储。
2. **Windows 11 默认要求 TPM 2.0 + Secure Boot**：FMX1 没有 TPM。
   短期目标应为 **Windows 10 ARM64**，或使用 Windows 11 的绕过选项；
   fTPM/Secure Boot 作为后续独立课题。
3. **安装介质**：优先用 USB 存储安装到 USB SSD（xHCI 路径）；eMMC 安装依赖
   Windows 的 SDHCI 驱动能否绑定 `PNP0D40`，需要实机验证。
4. **ACPI 正确性要求高**：FADT/MADT/GTDT/DSDT 必须严格符合 SBBR；CPU 设备、
   GICv3、定时器、xHCI、SDHCI、UART 的资源描述要与硬件一致。
5. **结论**：把“Windows arm64 能启动到安装界面/桌面”作为 P4 的探索性目标，
   不在 P1 承诺；Ubuntu 发行版是主支持路径。

---

## 10. 驱动与模块清单

### 10.1 阶段 P0（能启动到串口 Shell）

- `PeilessSec`、`DxeMain`、`PcdDxe`、`CpuDxe`、`RuntimeDxe`、`ArmGicV3Dxe`、
  `TimerDxe`、`WatchdogTimer`、`SerialDxe`、`ConSplitter/Terminal`。
- `Dw8250SerialPortLib`（UART2 1.5M）。
- `Rk3399SocLib` + `Rk3399ClockLib` + `Rk3399IomuxLib` + `PlatformLib`（6 核）。
- `Fmx1ProBoardLib`（UART 引脚、SD/eMMC 电源、基础时钟）。
- BDS + UEFI Shell（串口）。
- GitHub Actions 构建 + 产物。

### 10.2 阶段 P1（屏幕 + SD 启动 Linux）

- `LcdGraphicsOutputDxe` + `RockchipLcdPlatformLib` + `VopDxe` + `DwHdmiDxe`（含 EDID）。
- `DwMmcHostDxe`（SD）+ `EmbeddedPkg/Universal/MmcDxe` + Fat/Partition/DiskIo。
- `Rk808RtcLib` + `DwI2cLib`（i2c0）。
- ACPI 表 + `AcpiPlatformDxe`；DTB + `FdtPlatformDxe`。
- LogoDxe、BootManager（自动 `BOOTAA64.EFI`）。
- 验收：SD 卡上的 GRUB 启动 FMX1 Linux，HDMI 显示，6 核，RTC 正常。

### 10.3 阶段 P2（eMMC + USB + 持久变量）

- `SdhciHostDxe`（Arasan eMMC，PIO/ADMA2，先 50MHz）。
- `Rk3399UsbLib` + `Dwc3HostDxe` + `EhciOhciInitDxe`：
  - u2phy0/1、tcphy0/1 上电与模式；
  - CRU 时钟/复位；
  - VBUS GPIO；
  - NonDiscoverable 注册 xHCI/EHCI/OHCI；
  - OHCI 驱动从 edk2-rockchip 参考移植（现代 edk2 已移除）。
- `UsbBusDxe`、`UsbMassStorageDxe`、`UsbKbDxe`。
- `RkFvbDxe` + 持久变量（eMMC 保留区或 ESP 文件）。
- 验收：eMMC 中的 GRUB/Linux 启动；USB 键盘、U 盘可用。

### 10.4 阶段 P3（完善）

- 网络：DWC EQOS SNP + UEFI 网络栈 + PXE/HTTP Boot。
- Setup UI：显示模式、ACPI/DT 切换、串口波特率、启动顺序。
- Capsule 升级、Secure Boot（可选）。
- 多 SoC 骨架：`RK356x`/`RK3588` 实现同一组接口（只编译验证）。
- 板卡自动识别接口落地（一镜像多板）。

---

## 11. 多板 / 多 SoC 扩展流程

新增一块 RK3399 板：

1. `demo/boards/<board>/` 放入 DTS、硬件笔记、上游配置。
2. 新建 `Platform/<Vendor>/<Board>/`：
   - 复制 `Fmx1Pro.dsc.inc` 与板级 `RockchipBoardLib`；
   - 填 `ROCKCHIP_BOARD_INFO`、MMC/USB/显示描述、引脚与电源动作；
   - 写板级 `Dsdt.asl`（若与 FMX1 不同）；
   - 添加 `DeviceTree/<Board>.inf`（引用 demo DTS）。
3. 在 `configs/boards/<board>.conf` 增加一行；CI 矩阵自动构建。
4. 不改 `Silicon/Rockchip/**`；若需改，说明是公共缺陷，单独提交。

新增 SoC：

1. 新建 `Silicon/Rockchip/<SoC>/`，实现 `RockchipSocLib` 及 `Clock/Iomux/Usb/Display`。
2. 提供 SoC `AcpiTables` 与 `RK<SoC>.dsc.inc`。
3. 板级包只依赖接口，不依赖具体 SoC 头文件。

---

## 12. 构建系统与 GitHub Actions

### 12.1 本地构建

```bash
# 1. 拉取并准备 edk2（固定 tag，按需拉取所需 submodule）
scripts/ci/setup.sh

export PACKAGES_PATH="$PWD:$PWD/edk2"
export GCC_AARCH64_PREFIX=aarch64-linux-gnu-
export WORKSPACE="$PWD"
source edk2/edksetup.sh BaseTools
build -a AARCH64 -t GCC -b DEBUG \
      -p Platform/Fmx/Fmx1Pro/Fmx1ProA.dsc
scripts/gen-dtb.sh fmx1-pro-a
scripts/pack/make-sd-image.sh fmx1-pro-a
```

或直接：`scripts/build.sh fmx1-pro-a DEBUG`。

依赖：`gcc-aarch64-linux-gnu`、`nasm`、`acpica-tools`、`uuid-dev`、
`python3`、`device-tree-compiler`、`mtools`、`dosfstools`。

### 12.2 CI（`.github/workflows/build.yml`）

- 触发：`push`、`pull_request`、`tag v*`、`workflow_dispatch`。
- Job `test`：Python 工具单测 + 主机侧 C 单测 + shellcheck。
- Job `build`（matrix 来自 `configs/boards/*.conf`）：
  1. `ubuntu-24.04`，安装依赖；
  2. `actions/checkout`（edk2 由 `scripts/ci/setup.sh` 拉取固定 tag）；
  3. 缓存 `edk2/BaseTools`（key = edk2 commit + runner OS）；
  4. `scripts/ci/build-board.sh <board>`；
  5. `scripts/gen-dtb.sh`、`scripts/pack/make-sd-image.sh`；
  6. 产物：`uefi-loader2.img`、`idbloader.img`、`trust.img`、`*.dtb`、
     `*.img.gz`、`SHA256SUMS`、构建日志与 map。
- Job `release`（仅 tag）：汇总所有板卡产物，创建 Release。
- 安全：Actions 固定到 commit SHA；`permissions` 最小化；
  release job 才申请 `contents: write`；不使用任何密钥。
- 不做硬件在环：CI 只保证可编译、可打包、ACPI/DTB 可解析；
  硬件验收按 §13 手工执行。

### 12.3 产物命名

```text
rk-edk2-v<ver>-<board>.tar.gz        # 组件包（fd/loader2/dtb/idbloader/trust/README）
rk-edk2-v<ver>-<board>-sd.img.gz     # 16MB TF 卡裸镜像
README.txt
SHA256SUMS
build-<board>.log
```

---

## 13. 测试与验收

### 13.1 主机侧自动化

- 单元测试（80% 覆盖目标）：
  - UART 分频计算；PLL/时钟选择；MADT/DSDT 生成器；
  - DTB fixup（reserved-memory、chosen）；
  - loader2 打包/解包（Python 实现与 `loaderimage` 产物逐字节对比）。
- 静态检查：`iasl -d` 反汇编 ACPI 并检查表头/长度；`dtc -I dtb -O dts` 回读；
  `GenFv` 产物结构检查。

### 13.2 硬件验收（FMX1 Pro A）

| 项 | 判定 |
|---|---|
| 串口 | 上电输出 UEFI banner，UART2 1.5M 可交互 |
| 内存 | Shell `memmap` 显示约 4GB，无 MMIO 冲突 |
| 显示 | HDMI 出 Logo、GOP 文本控制台，`mode` 可列模式 |
| SD 启动 | GRUB 从 SD ESP 启动 Linux；`nproc=6` |
| Ubuntu | 发行版安装器可启动；安装到 eMMC/TF 后可正常启动、桌面/网络可用（DT） |
| Windows | arm64 安装介质能进入安装界面；ACPI 枚举出 CPU/xHCI/SDHCI；记录驱动缺口 |
| eMMC | `map -r` 能看到 eMMC BlockIo；Linux 根文件系统在 eMMC |
| USB | USB 键盘可用；U 盘可读 |
| RTC | UEFI Shell `time` 与 Linux `hwclock` 一致 |
| ACPI | 切换 ACPI 模式后 `acpidump` 表完整；eMMC 可识别 |
| DT | `cat /proc/device-tree/model` = FMX1；HDMI/PMIC 正常 |
| 稳定性 | 连续冷启动 20 次无挂起；高低温/长时间运行（可选） |

---

## 14. 分阶段计划（相对工作量）

| 阶段 | 内容 | 预估 |
|---|---|---|
| P0 | 仓库骨架、edk2 锁定、RK3399 平台库、串口、CI、Shell | 1–2 周 |
| P1 | GOP/HDMI、SD、ACPI/DT、RTC、Ubuntu 安装/启动 | 3–5 周 |
| P2 | eMMC、USB、持久变量、B 变体 | 3–5 周 |
| P3 | 网络、Setup、Capsule、多 SoC 骨架 | 按需 |
| P4 | Windows arm64 启动/安装探索（ACPI 正确性、SDHCI、TPM/Secure Boot 评估） | 探索性 |

> 工作量为单人投入的粗估，硬件调试（尤其 USB PHY/HDMI/eMMC）是主要不确定项。

---

## 15. 风险与对策

| 风险 | 影响 | 对策 |
|---|---|---|
| loader2/trust 打包格式 | 无法启动 | 先原样使用 rkbin 的 `idbloader/trust.img` + demo 验证过的 `loaderimage`；阶段 2 用 Python 重实现并逐字节比对 |
| eMMC Arasan SDHCI 驱动 | 无法从 eMMC 启动 | 先 PIO + 低速 50MHz；参考 RPi `ArasanMmcHostDxe` 与 SDK 代码；SD 路径先通 |
| USB PHY 无公开文档 | USB 不可用 | 以 6.18 内核 `u2phy/tcphy` 寄存器序列为准；按端口逐个验证；先保证 xHCI |
| HDMI/EDID | 无显示 | EDID 失败回退 1080p；先复用 demo 已验证的 dw-hdmi 代码 |
| ACPI 下 SD 不可用 | 模式受限 | 默认 DT；文档明确 ACPI 仅承诺 eMMC/UART/xHCI |
| Windows arm64 无 RK3399 驱动 | 无法日常使用 | 只承诺引导/枚举；GPU/网络/音频不在范围；先做 Windows 10 ARM64 |
| Windows 11 TPM/Secure Boot | 安装被拒 | 后续评估 fTPM；先用 Win10 或绕过选项 |
| Windows 安装器对大内存/ACPI 敏感 | 安装失败 | 严格按 SBBR 生成表；逐项比对硬件；保留串口日志 |
| edk2 API 变动 | 编译失败 | 锁定 `edk2-stable202608` 子模块；升级单独 PR |
| 许可证混合（GPL-2.0 显示/USB 代码 + BSD EDK2） | 分发合规 | 文件级标注；最终固件按 GPL-2.0 分发；在 `LICENSE`/`NOTICE` 说明 |
| CI 无法验证硬件 | 回归漏检 | 发布前手工跑 §13 表格；保留串口日志模板 |

---

## 16. 需要确认的默认决策

1. **edk2 基线**：使用 `edk2-stable202608`（与 edk2-rk3588 参考一致），
   以 git submodule 锁定；不沿用 demo 的 2018 版本。
2. **板卡范围**：本期实现 FMX1 Pro A；B 作为同一板级包的变体（不同 DTB），
   共用全部驱动。
3. **配置表模式**：默认同时安装 ACPI 与 DTB（`Both`）；Ubuntu 默认走 DT，
   Windows 走 ACPI；Setup 可切换。
4. **eMMC 方案**：新增 Arasan SDHCI host 驱动（`EFI_MMC_HOST_PROTOCOL`），
   SD 继续用 DW MSHC 驱动；两者统一由 EmbeddedPkg `MmcDxe` 产出 BlockIo。
5. **许可证**：接受移植 demo/内核中的 GPL-2.0 代码（显示、USB、MMC），
   固件整体以 GPL-2.0 兼容方式分发；如必须纯 BSD，显示/USB 需要重写，周期显著增加。
6. **OS 目标**：Ubuntu arm64 为主支持路径；Windows arm64 为 P4 探索目标，
   只承诺引导/ACPI 枚举，不承诺 GPU/网络/音频驱动。

确认以上 5 项后即可进入 P0 实施。
