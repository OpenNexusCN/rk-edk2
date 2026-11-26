/** @file
  RK3399 ArmPlatformLib implementation.

  Copyright (c) 2014-2017, Linaro Limited. All rights reserved.
  Copyright (c) 2017, Rockchip Inc. All rights reserved.
  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Pi/PiBootMode.h>
#include <Library/ArmPlatformLib.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/RockchipBoardLib.h>
#include <Library/RockchipSocLib.h>
#include <Ppi/ArmMpCoreInfo.h>

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  4

/*
 * RK3399: cluster 0 = 4x Cortex-A53 (MPIDR 0x0..0x3),
 *         cluster 1 = 2x Cortex-A72 (MPIDR 0x100, 0x101).
 */
STATIC ARM_CORE_INFO  mRk3399MpCoreInfoTable[] = {
  { 0x0,   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 },
  { 0x1,   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 },
  { 0x2,   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 },
  { 0x3,   0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 },
  { 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 },
  { 0x101, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 },
};

STATIC
EFI_STATUS
EFIAPI
Rk3399GetMpCoreInfo (
  OUT UINTN          *CoreCount,
  OUT ARM_CORE_INFO  **ArmCoreTable
  )
{
  *CoreCount    = ARRAY_SIZE (mRk3399MpCoreInfoTable);
  *ArmCoreTable = mRk3399MpCoreInfoTable;
  return EFI_SUCCESS;
}

STATIC ARM_MP_CORE_INFO_PPI    mMpCoreInfoPpi = {
  Rk3399GetMpCoreInfo
};

STATIC EFI_PEI_PPI_DESCRIPTOR  mPlatformPpiTable[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

RETURN_STATUS
ArmPlatformInitialize (
  IN UINTN  MpId
  )
{
  RockchipSocEarlyInit ();
  RockchipBoardEarlyInit ();
  return RETURN_SUCCESS;
}

VOID
ArmPlatformInitializeSystemMemory (
  VOID
  )
{
}

VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *Table;
  UINTN                         Index;
  UINT64                        DramBase;
  UINT64                        DramSize;

  ASSERT (VirtualMemoryMap != NULL);

  Table = AllocatePages (EFI_SIZE_TO_PAGES (
                                   sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                                   MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                                   ));
  ASSERT (Table != NULL);
  if (Table == NULL) {
    *VirtualMemoryMap = NULL;
    return;
  }

  Index = 0;

  /* 0x0 - 0xF8000000: DRAM, write-back cacheable. */
  DramBase = PcdGet64 (PcdSystemMemoryBase);
  DramSize = PcdGet64 (PcdSystemMemorySize);
  Table[Index].PhysicalBase = DramBase;
  Table[Index].VirtualBase  = DramBase;
  Table[Index].Length       = DramSize;
  Table[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

  /* 0xF8000000 - 0xFFFFFFFF: peripherals, device memory. */
  Index++;
  Table[Index].PhysicalBase = 0xF8000000;
  Table[Index].VirtualBase  = 0xF8000000;
  Table[Index].Length       = 0x08000000;
  Table[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  /* End of table. */
  Index++;
  Table[Index].PhysicalBase = 0;
  Table[Index].VirtualBase  = 0;
  Table[Index].Length       = 0;
  Table[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT ((Index + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);
  *VirtualMemoryMap = Table;
}

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = sizeof (mPlatformPpiTable);
  *PpiList     = mPlatformPpiTable;
}
