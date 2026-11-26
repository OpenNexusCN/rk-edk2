## @file
#  RK3399 SoC DSC include.
#
#  Copyright (c) 2026, OpenNexusCN contributors
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses.common]
  ArmPlatformLib|Silicon/Rockchip/RK3399/Library/Rk3399PlatformLib/Rk3399PlatformLib.inf
  RockchipSocLib|Silicon/Rockchip/RK3399/Library/Rk3399SocLib/Rk3399SocLib.inf

[PcdsFixedAtBuild.common]
  # GICv3
  gArmTokenSpaceGuid.PcdGicDistributorBase|0xFEE00000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0xFFF00000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0xFEF00000

  # 4x A53 + 2x A72
  gArmPlatformTokenSpaceGuid.PcdCoreCount|6
  gArmPlatformTokenSpaceGuid.PcdClusterCount|2
  gArmTokenSpaceGuid.PcdArmPrimaryCore|0
  gArmTokenSpaceGuid.PcdArmPrimaryCoreMask|0xF03

  # Console clock source (xin24m)
  gRockchipTokenSpaceGuid.PcdUartClockInHz|24000000

  # SoC defaults; a board may override.
  gRK3399TokenSpaceGuid.PcdRk3399CpuCount|6
  gRK3399TokenSpaceGuid.PcdRk3399DramSizeOverride|0
  gRK3399TokenSpaceGuid.PcdRk3399Uart2Base|0xFF1A0000
  gRK3399TokenSpaceGuid.PcdRk3399Uart2Index|2
