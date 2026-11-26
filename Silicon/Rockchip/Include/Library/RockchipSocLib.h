/** @file
  Rockchip SoC abstraction.

  Implemented once per SoC package (e.g. Silicon/Rockchip/RK3399).
  Common drivers and board code must only use this interface, never SoC
  register offsets directly.

  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ROCKCHIP_SOC_LIB_H_
#define ROCKCHIP_SOC_LIB_H_

#include <Uefi/UefiBaseType.h>

#define ROCKCHIP_MAX_CPUS  8

typedef enum {
  RockchipSocRk3399 = 0x3399,
  RockchipSocRk356X = 0x3560,
  RockchipSocRk3588 = 0x3588,
} ROCKCHIP_SOC_ID;

typedef enum {
  RockchipUsbModeHost = 0,
  RockchipUsbModePeripheral = 1,
} ROCKCHIP_USB_MODE;

typedef struct {
  UINT64    Mpidr;
  UINT8     Cluster;
  UINT8     Core;
} ROCKCHIP_CPU_INFO;

typedef struct {
  UINT32    SocId;
  UINT64    DramBase;
  UINT64    DramSize;
  UINT32    CpuCount;
  ROCKCHIP_CPU_INFO  Cpu[ROCKCHIP_MAX_CPUS];
  UINT64    GicDBase;
  UINT64    GicRBase;
  UINT64    GicCBase;
  UINT64    GicHBase;
  UINT64    GicVBase;
  UINT64    GicItsBase;
  UINT32    GicVersion;
  UINT32    TimerSecEl1Gsiv;
  UINT32    TimerNonSecEl1Gsiv;
  UINT32    TimerVirtGsiv;
  UINT32    TimerHypGsiv;
} ROCKCHIP_SOC_INFO;

/**
  One-time early initialization, called from SEC before the HOB list exists.
  Must not allocate memory.
**/
EFI_STATUS
EFIAPI
RockchipSocEarlyInit (
  VOID
  );

/**
  Late initialization, called from DXE.
**/
EFI_STATUS
EFIAPI
RockchipSocLateInit (
  VOID
  );

VOID
EFIAPI
RockchipSocGetInfo (
  OUT ROCKCHIP_SOC_INFO  *Info
  );

EFI_STATUS
EFIAPI
RockchipSocSetUartClock (
  IN UINT8   Index,
  IN UINT32  Hz
  );

EFI_STATUS
EFIAPI
RockchipSocSetMmcClock (
  IN UINT8   Index,
  IN UINT32  Hz
  );

EFI_STATUS
EFIAPI
RockchipSocSetVopClock (
  IN UINT8   VopIndex,
  IN UINT32  PixelClockHz
  );

EFI_STATUS
EFIAPI
RockchipSocUsbPhyInit (
  IN UINT8              Index,
  IN ROCKCHIP_USB_MODE  Mode
  );

/**
  Set the IOMUX function of one pin.

  @param[in] Group  GPIO group number (0..4 on RK3399).
  @param[in] Pin    Pin number inside the group (0..31).
  @param[in] Func   Mux function (0 = GPIO).
**/
EFI_STATUS
EFIAPI
RockchipIomuxSet (
  IN UINT32  Group,
  IN UINT32  Pin,
  IN UINT32  Func
  );

/**
  Set a GPIO pin as an output and drive it.

  Implemented per SoC because GPIO0/GPIO1 live in the PMU GRF on RK3399.
**/
EFI_STATUS
EFIAPI
RockchipGpioSetOutput (
  IN UINT32   Group,
  IN UINT32   Pin,
  IN BOOLEAN  Value
  );

#endif // ROCKCHIP_SOC_LIB_H_
