# Rockchip 打包/烧录工具

本目录不直接提交闭源工具二进制，构建/烧写时从 `rkbin` 或 `rk-tools` 获取，
并按下表校验来源。

## 固件打包（CI/构建机）

| 工具 | 来源 | 用途 |
|---|---|---|
| `loaderimage` | `rkbin/tools/loaderimage` 或参考固件 `demo/upstream/rk3399-edk2/Rk3399Pkg/Tools/loaderimage` | 把 UEFI FD 打包为 loader2（`--pack --uboot`） |
| `boot_merger` | `rkbin/tools/boot_merger` | 合成 idbloader（DDR + miniloader） |
| `trust_merger` | `rkbin/tools/trust_merger` | 合成 trust.img（BL31/BL32） |
| `mkimage` | `rkbin/tools/mkimage` | 生成 FIT（后续 RK3588 等） |

CI 使用 `rkbin` 固定 commit（见 `../SOURCES.lock`）中的工具；如需要可把工具
二进制加入构建缓存，不提交到本仓库。

## 烧写（开发机）

| 工具 | 平台 | 用途 |
|---|---|---|
| `rkdeveloptool` | Linux/macOS | MaskROM/loader 模式读写 eMMC、TF |
| `upgrade_tool` | Linux | Rockchip 官方升级工具 |
| `RKDevTool` | Windows | 图形化烧写 |

FMX1 进入 MaskROM：按住刷机键上电，或短接 eMMC 时钟/数据脚（以板厂说明为准）。

## 推荐烧写顺序（eMMC / TF 相同）

```text
0x00008000 (32KB)   idbloader.img
0x00800000 (8MB)    uefi-loader2.img
0x00C00000 (12MB)   trust.img
0x01000000 (16MB)   GPT / ESP（BOOTAA64.EFI、DTB、grub.cfg）
```

对应 `rkdeveloptool` 命令示例：

```bash
rkdeveloptool db  idbloader.img
rkdeveloptool ul  idbloader.img
rkdeveloptool wl  0x4000 uefi-loader2.img   # 8MB / 512
rkdeveloptool wl  0x6000 trust.img          # 12MB / 512
rkdeveloptool rd
```

> `wl` 的偏移单位是扇区（512B）。执行前确认板子的分区布局，避免覆盖数据分区。
