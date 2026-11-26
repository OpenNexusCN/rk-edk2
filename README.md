# rk-edk2

Rockchip RK 系列板卡的 EDK2 UEFI 固件，分层支持多 SoC / 多板卡。
当前目标：**RK3399 + FMX1 Pro**（板载 32GB eMMC + TF 卡座）。
最终目标：在 FMX1 上运行 **发行版 Ubuntu（arm64）** 与 **Windows（arm64）**。

固件提供：

- UEFI 规范接口（启动时/运行时服务、GOP、块设备、USB、RTC）；
- ACPI 表（FADT/MADT/GTDT/SPCR/DSDT/SSDT）与嵌入式 DTB 双模式；
- HDMI 帧缓冲与开机 Logo；
- 自动加载 `\EFI\BOOT\BOOTAA64.EFI`（如 GRUB）。

## 状态

| 阶段 | 内容 | 状态 |
|---|---|---|
| P0 | 仓库骨架、参考归档、RK3399 平台库、串口控制台、CI | ✅ 构建通过，待硬件验证 |
| P1 | HDMI/GOP、TF 卡启动、ACPI/DT、RTC、Ubuntu 安装/启动 | 计划 |
| P2 | eMMC、USB、持久变量、B 版 | 计划 |
| P3 | 网络、Setup、Capsule、多 SoC | 计划 |
| P4 | Windows arm64 启动/安装探索（驱动缺口需单独评估） | 探索 |

> P0 当前只包含 SEC/DXE/串口 Shell，尚无显示、存储、ACPI、USB。
> 首次上板请按 [docs/bringup.md](docs/bringup.md) 执行。

## 预编译固件

GitHub Releases 提供按板卡打包的固件：

- `v0.0.1`：FMX1 Pro A P0 串口预览版，含 16MB TF 卡裸镜像；
- 刷写与串口观察步骤见 [docs/bringup.md](docs/bringup.md#0-使用-release-预编译包推荐)。

发布由 tag 触发：`git tag vX.Y.Z && git push origin vX.Y.Z`。

## 文档

- [设计文档](docs/design.md)
- [板卡参考代码](demo/README.md)
- [参考来源锁定](demo/SOURCES.lock)

## 快速开始

```bash
# 1. 获取依赖（edk2-stable202608，按需拉取 submodule）
scripts/ci/setup.sh

# 2. 构建（含 DTB 生成）
scripts/build.sh fmx1-pro-a
```

本机构建依赖：`gcc-aarch64-linux-gnu`、`nasm`、`acpica-tools`、`uuid-dev`、
`python3`、`device-tree-compiler`。CI 使用 GitHub Actions（`.github/workflows/build.yml`）。

## 目录

```text
Silicon/Rockchip/      公共 IP 驱动与板级/SoC 抽象接口
Silicon/Rockchip/RK3399/  RK3399 SoC 实现
Platform/Fmx/Fmx1Pro/  FMX1 Pro 板级包（A/B 变体）
demo/                  板卡参考代码与资料（只读）
configs/boards/        CI 构建矩阵
scripts/               构建、DTB、打包脚本
docs/                  设计文档
```

## 许可证

本项目新增代码采用 BSD-2-Clause-Patent；从参考固件/内核移植的 GPL-2.0
代码在文件头保留原声明，最终固件按 GPL-2.0 兼容方式分发。
详见 `LICENSE` 与 `NOTICE`。
