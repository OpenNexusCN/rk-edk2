/** @file
  Rockchip board abstraction.

  Implemented once per board package (e.g. Platform/Fmx/Fmx1Pro).
  Board code describes hardware: pin mux, rail enables, storage/USB/display
  instances, DTB name and ACPI identifiers. It must not contain SoC register
  offsets; use RockchipSocLib / RockchipIomuxLib instead.

  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ROCKCHIP_BOARD_LIB_H
#define ROCKCHIP_BOARD_LIB_H

#include <Uefi/UefiBaseType.h>
#include <Library/RockchipSocLib.h>

typedef enum {
  RockchipConnectorHdmi = 0,
  RockchipConnectorDp,
  RockchipConnectorEdp,
  RockchipConnectorDsi,
  RockchipConnectorLvds,
} ROCKCHIP_CONNECTOR;

typedef enum {
  RockchipDdcInternalHdmi = 0,
  RockchipDdcExternalI2c,
} ROCKCHIP_DDC_MODE;

typedef struct {
  UINT32    SocId;
  UINT32    BoardId;
  UINT32    Variant;
  CHAR8     *BoardName;
  CHAR8     *DtbName;
  CHAR8     *AcpiOemId;
  CHAR8     *AcpiOemTableId;
} ROCKCHIP_BOARD_INFO;

typedef struct {
  UINT8     Id;
  UINTN     Base;
  UINT32    InputClockHz;
  UINT32    MaxBusWidth;
  BOOLEAN   Removable;
  INT32     CardDetectGpio;   // -1 = none
} ROCKCHIP_MMC_DESC;

typedef struct {
  UINT8              Id;
  UINTN              Dwc3Base;
  UINTN              XhciBase;
  UINTN              EhciBase;
  UINTN              OhciBase;
  UINT32             Irq;
  ROCKCHIP_USB_MODE  Mode;
} ROCKCHIP_USB_DESC;

typedef struct {
  UINT8                VopId;
  ROCKCHIP_CONNECTOR   Connector;
  ROCKCHIP_DDC_MODE    DdcMode;
  UINT8                DdcI2cBus;
} ROCKCHIP_DISPLAY_ROUTE;

CONST ROCKCHIP_BOARD_INFO *
EFIAPI
RockchipBoardGetInfo (
  VOID
  );

/**
  Called from SEC before the serial port and HOBs are available.
  Must only touch clock gates, pin mux and GPIO rails that are safe early.
**/
VOID
EFIAPI
RockchipBoardEarlyInit (
  VOID
  );

VOID
EFIAPI
RockchipBoardLateInit (
  VOID
  );

UINTN
EFIAPI
RockchipBoardGetMmcDescs (
  OUT ROCKCHIP_MMC_DESC  **Descs
  );

UINTN
EFIAPI
RockchipBoardGetUsbDescs (
  OUT ROCKCHIP_USB_DESC  **Descs
  );

UINTN
EFIAPI
RockchipBoardGetDisplayRoutes (
  OUT ROCKCHIP_DISPLAY_ROUTE  **Routes
  );

/**
  Optional runtime board detection. Returns compile-time values in P0.
**/
EFI_STATUS
EFIAPI
RockchipBoardDetect (
  OUT UINT32  *BoardId,
  OUT UINT32  *Variant
  );

#endif // ROCKCHIP_BOARD_LIB_H
