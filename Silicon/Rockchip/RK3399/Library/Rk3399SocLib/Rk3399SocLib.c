/** @file
  Rockchip RK3399 SoC implementation of RockchipSocLib.

  P0 scope: DRAM detection, CPU/GIC/timer description, pin mux and GPIO.
  Clock tree and PHY programming arrive with the storage/display/USB stages.

  Copyright (c) 2017, Rockchip Inc.
  Copyright (c) 2019, Andrey Warkentin <andrey.warkentin@gmail.com>
  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/RockchipSocLib.h>

#include <RK3399.h>
#include <Rk3399Grf.h>
#include <Rk3399PmuGrf.h>

STATIC UINT64  mRk3399DramSize;

STATIC CONST UINTN  mRk3399GpioBases[] = {
  0xFF720000,  /* GPIO0 */
  0xFF730000,  /* GPIO1 */
  0xFF780000,  /* GPIO2 */
  0xFF788000,  /* GPIO3 */
  0xFF790000,  /* GPIO4 */
};

#define RK3399_GPIO_SWPORT_DR_L   0x0000
#define RK3399_GPIO_SWPORT_DR_H   0x0004
#define RK3399_GPIO_SWPORT_DDR_L  0x0008
#define RK3399_GPIO_SWPORT_DDR_H  0x000C

/**
  Decode DRAM capacity from the value the miniloader left in PMU GRF OS_REG2.
**/
STATIC
UINT64
Rk3399DetectDramSize (
  VOID
  )
{
  UINT32  SysReg;
  UINT32  Rank;
  UINT32  Col;
  UINT32  Bank;
  UINT32  Cs0Row;
  UINT32  Cs1Row;
  UINT32  Bw;
  UINT32  Row34;
  UINT32  ChipSizeMb;
  UINT64  SizeMb;
  UINT32  Ch;
  UINT32  ChNum;

  SysReg = PmuGrfReadl (PMU_GRF_OS_REG2);
  ChNum  = 1 + ((SysReg >> SYS_REG_NUM_CH_SHIFT) & SYS_REG_NUM_CH_MASK);
  if (ChNum > 2) {
    ChNum = 2;
  }

  SizeMb = 0;
  for (Ch = 0; Ch < ChNum; Ch++) {
    Rank   = 1 + ((SysReg >> SYS_REG_RANK_SHIFT (Ch)) & SYS_REG_RANK_MASK);
    Col    = 9 + ((SysReg >> SYS_REG_COL_SHIFT (Ch)) & SYS_REG_COL_MASK);
    Bank   = 3 - ((SysReg >> SYS_REG_BK_SHIFT (Ch)) & SYS_REG_BK_MASK);
    Cs0Row = 13 + ((SysReg >> SYS_REG_CS0_ROW_SHIFT (Ch)) & SYS_REG_CS0_ROW_MASK);
    Cs1Row = 13 + ((SysReg >> SYS_REG_CS1_ROW_SHIFT (Ch)) & SYS_REG_CS1_ROW_MASK);
    Bw     = (2 >> ((SysReg >> SYS_REG_BW_SHIFT (Ch)) & SYS_REG_BW_MASK));
    Row34  = (SysReg >> SYS_REG_ROW_3_4_SHIFT (Ch)) & SYS_REG_ROW_3_4_MASK;

    if ((Cs0Row + Col + Bank + Bw) < 20) {
      continue;
    }

    ChipSizeMb = (1U << (Cs0Row + Col + Bank + Bw - 20));
    if (Rank > 1) {
      ChipSizeMb += ChipSizeMb >> (Cs0Row - Cs1Row);
    }

    if (Row34 != 0) {
      ChipSizeMb = ChipSizeMb * 3 / 4;
    }

    SizeMb += ChipSizeMb;
  }

  /* Usable DRAM stops at the 0xF8000000 peripheral window. */
  if (SizeMb >= (RK3399_PERIPH_BASE >> 20)) {
    return RK3399_PERIPH_BASE;
  }

  return SizeMb << 20;
}

EFI_STATUS
EFIAPI
RockchipSocEarlyInit (
  VOID
  )
{
  mRk3399DramSize = Rk3399DetectDramSize ();
  if (mRk3399DramSize == 0) {
    mRk3399DramSize = PcdGet64 (PcdRk3399DramSizeOverride);
  }

  DEBUG ((
    DEBUG_INFO,
    "RK3399: DRAM %Lu MB, SOC_REG2=0x%08x\n",
    DivU64x32 (mRk3399DramSize, SIZE_1MB),
    PmuGrfReadl (PMU_GRF_OS_REG2)
    ));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RockchipSocLateInit (
  VOID
  )
{
  return EFI_SUCCESS;
}

VOID
EFIAPI
RockchipSocGetInfo (
  OUT ROCKCHIP_SOC_INFO  *Info
  )
{
  UINT32  Index;

  if (Info == NULL) {
    return;
  }

  ZeroMem (Info, sizeof (*Info));
  Info->SocId              = RockchipSocRk3399;
  Info->DramBase           = 0;
  Info->DramSize           = mRk3399DramSize;
  Info->GicDBase           = RK3399_GICD_BASE;
  Info->GicRBase           = RK3399_GICR_BASE;
  Info->GicCBase           = RK3399_GICC_BASE;
  Info->GicHBase           = RK3399_GICH_BASE;
  Info->GicVBase           = RK3399_GICV_BASE;
  Info->GicItsBase         = RK3399_GIC_ITS_BASE;
  Info->GicVersion         = 3;
  Info->TimerSecEl1Gsiv    = RK3399_TIMER_SEC_EL1_GSIV;
  Info->TimerNonSecEl1Gsiv = RK3399_TIMER_NONSEC_EL1_GSIV;
  Info->TimerVirtGsiv      = RK3399_TIMER_VIRT_GSIV;
  Info->TimerHypGsiv       = RK3399_TIMER_HYP_GSIV;

  /*
   * RK3399: cluster 0 = 4x Cortex-A53 (MPIDR 0x0..0x3),
   *         cluster 1 = 2x Cortex-A72 (MPIDR 0x100, 0x101).
   */
  Info->CpuCount = 6;
  for (Index = 0; Index < 4; Index++) {
    Info->Cpu[Index].Mpidr   = Index;
    Info->Cpu[Index].Cluster = 0;
    Info->Cpu[Index].Core    = (UINT8)Index;
  }

  Info->Cpu[4].Mpidr   = 0x100;
  Info->Cpu[4].Cluster = 1;
  Info->Cpu[4].Core    = 0;
  Info->Cpu[5].Mpidr   = 0x101;
  Info->Cpu[5].Cluster = 1;
  Info->Cpu[5].Core    = 1;
}

EFI_STATUS
EFIAPI
RockchipSocSetUartClock (
  IN UINT8   Index,
  IN UINT32  Hz
  )
{
  /* UART source is the fixed 24 MHz xin24m; nothing to program. */
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RockchipSocSetMmcClock (
  IN UINT8   Index,
  IN UINT32  Hz
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
RockchipSocSetVopClock (
  IN UINT8   VopIndex,
  IN UINT32  PixelClockHz
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
RockchipSocUsbPhyInit (
  IN UINT8              Index,
  IN ROCKCHIP_USB_MODE  Mode
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
Rk3399IomuxGetRegister (
  IN  UINT32  Group,
  IN  UINT32  Pin,
  OUT UINTN   *Register
  )
{
  UINT32  Bank;
  UINT32  Base;

  if ((Group > 4) || (Pin > 31)) {
    return EFI_INVALID_PARAMETER;
  }

  Bank = Pin / 8;

  switch (Group) {
    case 0:
      if (Bank > 1) {
        return EFI_INVALID_PARAMETER;
      }
      Base     = PcdGet32 (PcdRk3399PmuGrfBase);
      *Register = Base + PMU_GRF_GPIO0A_IOMUX + Bank * 4;
      return EFI_SUCCESS;
    case 1:
      Base      = PcdGet32 (PcdRk3399PmuGrfBase);
      *Register = Base + PMU_GRF_GPIO1A_IOMUX + Bank * 4;
      return EFI_SUCCESS;
    case 2:
      *Register = PcdGet32 (PcdRk3399GrfBase) + GRF_GPIO2A_IOMUX + Bank * 4;
      return EFI_SUCCESS;
    case 3:
      *Register = PcdGet32 (PcdRk3399GrfBase) + GRF_GPIO3A_IOMUX + Bank * 4;
      return EFI_SUCCESS;
    case 4:
      *Register = PcdGet32 (PcdRk3399GrfBase) + GRF_GPIO4A_IOMUX + Bank * 4;
      return EFI_SUCCESS;
    default:
      return EFI_INVALID_PARAMETER;
  }
}

EFI_STATUS
EFIAPI
RockchipIomuxSet (
  IN UINT32  Group,
  IN UINT32  Pin,
  IN UINT32  Func
  )
{
  EFI_STATUS  Status;
  UINTN       Register;
  UINT32      Shift;
  UINT32      Value;

  Status = Rk3399IomuxGetRegister (Group, Pin, &Register);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Shift = (Pin % 8) * 2;
  Value = MmioRead32 (Register);
  Value &= ~(0x3U << Shift);
  Value |= (Func & 0x3) << Shift;
  Value |= 0x3U << (Shift + 16);
  MmioWrite32 (Register, Value);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RockchipGpioSetOutput (
  IN UINT32   Group,
  IN UINT32   Pin,
  IN BOOLEAN  Value
  )
{
  UINTN       Base;
  UINTN       Reg;
  UINT32      Bit;
  EFI_STATUS  Status;

  if ((Group > 4) || (Pin > 31)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = RockchipIomuxSet (Group, Pin, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Base = mRk3399GpioBases[Group];
  Bit  = Pin % 16;
  Reg  = Base + ((Pin < 16) ? RK3399_GPIO_SWPORT_DDR_L : RK3399_GPIO_SWPORT_DDR_H);

  MmioWrite32 (Reg, MmioRead32 (Reg) | (1U << Bit));

  Reg = Base + ((Pin < 16) ? RK3399_GPIO_SWPORT_DR_L : RK3399_GPIO_SWPORT_DR_H);
  if (Value) {
    MmioWrite32 (Reg, MmioRead32 (Reg) | (1U << Bit));
  } else {
    MmioWrite32 (Reg, MmioRead32 (Reg) & ~(1U << Bit));
  }

  return EFI_SUCCESS;
}
