## @file
#  FMX1 Pro A board DSC.
#
#  Copyright (c) 2026, OpenNexusCN contributors
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = Fmx1ProA
  PLATFORM_GUID                  = 3b7c9e12-5d4f-4a6b-8c90-1e2f3a4b5c6d
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001A
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Silicon/Rockchip/RK3399/RK3399.fdf
  RK_PLATFORM_FVMAIN_MODULES     = Platform/Fmx/Fmx1Pro/Fmx1Pro.Modules.fdf.inc

!include Silicon/Rockchip/Rockchip.dsc.inc
!include Silicon/Rockchip/RK3399/RK3399.dsc.inc
!include Platform/Fmx/Fmx1Pro/Fmx1Pro.dsc.inc

[PcdsFixedAtBuild.common]
  gRockchipTokenSpaceGuid.PcdBoardVariant|0
