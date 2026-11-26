/** @file
  Rockchip platform runtime protocol.

  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ROCKCHIP_PLATFORM_PROTOCOL_H_
#define ROCKCHIP_PLATFORM_PROTOCOL_H_

#include <Library/RockchipBoardLib.h>

#define ROCKCHIP_PLATFORM_PROTOCOL_REVISION  0x00010000

typedef struct _ROCKCHIP_PLATFORM_PROTOCOL ROCKCHIP_PLATFORM_PROTOCOL;

struct _ROCKCHIP_PLATFORM_PROTOCOL {
  UINT32                    Revision;
  ROCKCHIP_BOARD_INFO       Info;
  ROCKCHIP_DISPLAY_ROUTE    DisplayRoute;
  UINT32                    ConfigTableMode;  // 0=DT, 1=ACPI, 2=Both
};

extern EFI_GUID  gRockchipPlatformProtocolGuid;

#endif // ROCKCHIP_PLATFORM_PROTOCOL_H_
