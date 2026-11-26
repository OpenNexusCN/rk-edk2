# demo —— 板卡参考代码与资料

本目录存放 rk-edk2 使用的**全部板卡参考代码与硬件资料**，是构建与移植的只读取输入。
固件实现代码不在这里，见 `Silicon/` 与 `Platform/`。

## 目录

| 路径 | 内容 |
|---|---|
| `upstream/rk3399-edk2/` | `andreiw/rk3399-edk2` 源码快照（去掉 `.git`），历史参考 |
| `rk3399-edk2/` | 本地原始 checkout（带 `.git`，已被 `.gitignore` 忽略） |
| `soc/rk3399/dts/` | RK3399 基础设备树：`rk3399.dtsi`、`rk3399-base.dtsi`、`rk3399-op1.dtsi` |
| `soc/rk3399/dt-bindings/` | DTS 所需的最小 `dt-bindings` 子集（已裁剪） |
| `boards/fmx1-pro-a/` | FMX1 Pro A 版 DTS、硬件笔记、one-kvm 板配置 |
| `boards/fmx1-pro-b/` | FMX1 Pro B 版 DTS、硬件笔记、one-kvm 板配置 |
| `blobs/rkbin/` | RK3399 DDR/miniloader/usbplug/BL31/BL32 及 `RKBOOT`/`RKTRUST` ini |
| `tools/` | Rockchip 打包/烧录工具说明 |
| `reference/` | 现代 edk2-rockchip 架构要点摘录 |

## 约束

- `demo/` 只读：构建脚本可以读取（DTS、blob），但不得写回。
- 所有二进制 blob 的来源、版本与许可证见 `SOURCES.lock` 与各子目录说明。
- 板级硬件差异一律写入 `boards/<board>/hardware.md`，实现时以该文件为准。

## 来源

参考代码来源于：

- `OpenNexusCN/rk-linux-6.18.y`（FMX1 设备树）
- `OpenNexusCN/rkbin`（闭源启动二进制）
- `OpenNexusCN/one-kvm-build`（FMX1 板配置）
- `OpenNexusCN/rk-tools`（打包/烧录工具）
- `andreiw/rk3399-edk2`（参考固件）
- `edk2-porting/edk2-rk3588` / Rockchip `edk2-rockchip`（现代分层实现参考）

精确 commit 见 `SOURCES.lock`。
