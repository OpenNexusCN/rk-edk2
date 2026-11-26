/** @file
  RK3399 memory map and register bases.

  Copyright (c) 2019, Andrey Warkentin <andrey.warkentin@gmail.com>
  Copyright (c) 2026, OpenNexusCN contributors
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RK3399_H_
#define RK3399_H_

#include <Library/IoLib.h>

/* SOC peripherals */
#define RK3399_PERIPH_BASE        0xF8000000U
#define RK3399_PERIPH_SIZE        0x08000000U

#define RK3399_PMU_GRF_BASE       0xFF320000U
#define RK3399_PMU_CRU_BASE       0xFF750000U
#define RK3399_CRU_BASE           0xFF760000U
#define RK3399_GRF_BASE           0xFF770000U

#define RK3399_UART2_BASE         0xFF1A0000U
#define RK3399_I2C0_BASE          0xFF3C0000U
#define RK3399_PWM_BASE           0xFF420000U

#define RK3399_SDHCI_BASE         0xFE330000U
#define RK3399_SDMMC_BASE         0xFE320000U
#define RK3399_SDIO_BASE          0xFE310000U

#define RK3399_EHCI0_BASE         0xFE380000U
#define RK3399_OHCI0_BASE         0xFE3A0000U
#define RK3399_EHCI1_BASE         0xFE3C0000U
#define RK3399_OHCI1_BASE         0xFE3E0000U
#define RK3399_DWC3_OTG0_BASE     0xFE800000U
#define RK3399_DWC3_OTG1_BASE     0xFE900000U

#define RK3399_VOP_LIT_BASE       0xFF8F0000U
#define RK3399_VOP_BIG_BASE       0xFF900000U
#define RK3399_HDMI_BASE          0xFF940000U

/* GIC */
#define RK3399_GICD_BASE          0xFEE00000ULL
#define RK3399_GICR_BASE          0xFEF00000ULL
#define RK3399_GICC_BASE          0xFFF00000ULL
#define RK3399_GICH_BASE          0xFFF10000ULL
#define RK3399_GICV_BASE          0xFFF20000ULL
#define RK3399_GIC_ITS_BASE       0xFEE20000ULL

/* Timers (PPI -> GSIV) */
#define RK3399_TIMER_SEC_EL1_GSIV    29
#define RK3399_TIMER_NONSEC_EL1_GSIV 30
#define RK3399_TIMER_VIRT_GSIV       27
#define RK3399_TIMER_HYP_GSIV        26

#define GrfReadl(Offset)            MmioRead32 (RK3399_GRF_BASE + (Offset))
#define GrfWritel(Value, Offset)    MmioWrite32 (RK3399_GRF_BASE + (Offset), (Value))
#define GrfClrSet(Offset, Clear, Set) \
  MmioAndThenOr32 (RK3399_GRF_BASE + (Offset), ~(Clear), (Set))

#define PmuGrfReadl(Offset)         MmioRead32 (RK3399_PMU_GRF_BASE + (Offset))
#define PmuGrfWritel(Value, Offset) MmioWrite32 (RK3399_PMU_GRF_BASE + (Offset), (Value))

#endif /* RK3399_H_ */
