/** @file
  FMX1 Pro board description and early board bring-up.

  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/RockchipBoardLib.h>
#include <Library/RockchipSocLib.h>

/* FMX1 Pro uses UART2 on GPIO4_C3 (RX) / GPIO4_C4 (TX), mux function 1. */
#define FMX1_UART2_RX_PIN  19
#define FMX1_UART2_TX_PIN  20
#define FMX1_UART2_MUX     1

/* Power rails controlled by GPIO. */
#define FMX1_SD_PWR_GPIO_GROUP  0
#define FMX1_SD_PWR_GPIO_PIN    1   /* GPIO0_A1, vcc3v3_sd */
#define FMX1_USB_VBUS_GPIO_GROUP 4
#define FMX1_USB_VBUS_GPIO_PIN   25  /* GPIO4_D1, vcc5v0_host */

STATIC CONST ROCKCHIP_BOARD_INFO  mFmx1ProBoardInfoA = {
  .SocId          = RockchipSocRk3399,
  .BoardId        = 0x0001,
  .Variant        = 0,                 /* A board */
  .BoardName      = "FMX1 Pro A",
  .DtbName        = "rk3399-fmx1-pro",
  .AcpiOemId      = "FMX1",
  .AcpiOemTableId = "FMX1PROA",
};

STATIC CONST ROCKCHIP_BOARD_INFO  mFmx1ProBoardInfoB = {
  .SocId          = RockchipSocRk3399,
  .BoardId        = 0x0001,
  .Variant        = 1,                 /* B board */
  .BoardName      = "FMX1 Pro B",
  .DtbName        = "rk3399-fmx1-pro-b",
  .AcpiOemId      = "FMX1",
  .AcpiOemTableId = "FMX1PROB",
};

STATIC ROCKCHIP_MMC_DESC  mFmx1MmcDescs[] = {
  {
    .Id             = 0,
    .Base           = 0xFE320000,
    .InputClockHz   = 100000000,
    .MaxBusWidth    = 4,
    .Removable      = TRUE,
    .CardDetectGpio = 7,               /* GPIO0_A7, active low */
  },
  {
    .Id             = 1,
    .Base           = 0xFE330000,
    .InputClockHz   = 200000000,
    .MaxBusWidth    = 8,
    .Removable      = FALSE,
    .CardDetectGpio = -1,
  },
};

STATIC ROCKCHIP_USB_DESC  mFmx1UsbDescs[] = {
  {
    .Id       = 0,
    .Dwc3Base = 0xFE800000,
    .XhciBase = 0xFE800000,
    .Irq      = 105,
    .Mode     = RockchipUsbModeHost,
  },
  {
    .Id       = 1,
    .Dwc3Base = 0xFE900000,
    .XhciBase = 0xFE900000,
    .Irq      = 110,
    .Mode     = RockchipUsbModeHost,
  },
  {
    .Id       = 2,
    .EhciBase = 0xFE380000,
    .OhciBase = 0xFE3A0000,
    .Irq      = 26,
    .Mode     = RockchipUsbModeHost,
  },
  {
    .Id       = 3,
    .EhciBase = 0xFE3C0000,
    .OhciBase = 0xFE3E0000,
    .Irq      = 30,
    .Mode     = RockchipUsbModeHost,
  },
};

STATIC ROCKCHIP_DISPLAY_ROUTE  mFmx1DisplayRoutes[] = {
  {
    .VopId      = 0,                   /* VOPB */
    .Connector  = RockchipConnectorHdmi,
    .DdcMode    = RockchipDdcInternalHdmi,
    .DdcI2cBus  = 3,
  },
};

CONST ROCKCHIP_BOARD_INFO *
EFIAPI
RockchipBoardGetInfo (
  VOID
  )
{
  if (FixedPcdGet32 (PcdBoardVariant) == 1) {
    return &mFmx1ProBoardInfoB;
  }

  return &mFmx1ProBoardInfoA;
}

VOID
EFIAPI
RockchipBoardEarlyInit (
  VOID
  )
{
  EFI_STATUS  Status;

  /* UART2 console pins first: the SEC debug console depends on them. */
  Status = RockchipIomuxSet (4, FMX1_UART2_RX_PIN, FMX1_UART2_MUX);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FMX1: failed to mux UART2 RX: %r\n", Status));
  }

  Status = RockchipIomuxSet (4, FMX1_UART2_TX_PIN, FMX1_UART2_MUX);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FMX1: failed to mux UART2 TX: %r\n", Status));
  }

  /* Keep TF card and USB VBUS rails enabled. */
  RockchipGpioSetOutput (FMX1_SD_PWR_GPIO_GROUP, FMX1_SD_PWR_GPIO_PIN, TRUE);
  RockchipGpioSetOutput (FMX1_USB_VBUS_GPIO_GROUP, FMX1_USB_VBUS_GPIO_PIN, TRUE);
}

VOID
EFIAPI
RockchipBoardLateInit (
  VOID
  )
{
}

UINTN
EFIAPI
RockchipBoardGetMmcDescs (
  OUT ROCKCHIP_MMC_DESC  **Descs
  )
{
  *Descs = mFmx1MmcDescs;
  return ARRAY_SIZE (mFmx1MmcDescs);
}

UINTN
EFIAPI
RockchipBoardGetUsbDescs (
  OUT ROCKCHIP_USB_DESC  **Descs
  )
{
  *Descs = mFmx1UsbDescs;
  return ARRAY_SIZE (mFmx1UsbDescs);
}

UINTN
EFIAPI
RockchipBoardGetDisplayRoutes (
  OUT ROCKCHIP_DISPLAY_ROUTE  **Routes
  )
{
  *Routes = mFmx1DisplayRoutes;
  return ARRAY_SIZE (mFmx1DisplayRoutes);
}

EFI_STATUS
EFIAPI
RockchipBoardDetect (
  OUT UINT32  *BoardId,
  OUT UINT32  *Variant
  )
{
  if ((BoardId == NULL) || (Variant == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *BoardId = RockchipBoardGetInfo ()->BoardId;
  *Variant = RockchipBoardGetInfo ()->Variant;
  return EFI_SUCCESS;
}
