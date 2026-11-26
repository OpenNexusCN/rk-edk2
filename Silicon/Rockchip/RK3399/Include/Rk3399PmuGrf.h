/** @file
  RK3399 PMU GRF offsets and DRAM size bitfields.

  Copyright (c) 2017, Rockchip Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RK3399_PMU_GRF_H_
#define RK3399_PMU_GRF_H_

/* PMU GRF GPIO IOMUX */
#define PMU_GRF_GPIO0A_IOMUX    0x0000
#define PMU_GRF_GPIO0B_IOMUX    0x0004
#define PMU_GRF_GPIO1A_IOMUX    0x0010
#define PMU_GRF_GPIO1B_IOMUX    0x0014
#define PMU_GRF_GPIO1C_IOMUX    0x0018
#define PMU_GRF_GPIO1D_IOMUX    0x001C

/* PMU GRF GPIO pull */
#define PMU_GRF_GPIO0A_P        0x0040
#define PMU_GRF_GPIO0B_P        0x0044
#define PMU_GRF_GPIO1A_P        0x0050
#define PMU_GRF_GPIO1B_P        0x0054
#define PMU_GRF_GPIO1C_P        0x0058
#define PMU_GRF_GPIO1D_P        0x005C

#define PMU_GRF_SOC_CON0        0x0180
#define PMU_GRF_SOC_CON10       0x01A8
#define PMU_GRF_SOC_CON11       0x01AC

#define PMU_GRF_OS_REG0         0x0300
#define PMU_GRF_OS_REG1         0x0304
#define PMU_GRF_OS_REG2         0x0308
#define PMU_GRF_OS_REG3         0x030C

/*
 * OS_REG2 DRAM layout (see RK3399 TRM):
 * [31]    row_3_4_ch1      [30]    row_3_4_ch0
 * [29:28] chinfo           [27]    rank_ch1
 * [26:25] col_ch1          [24]    bk_ch1
 * [23:22] cs0_row_ch1      [21:20] cs1_row_ch1
 * [19:18] bw_ch1           [17:16] dbw_ch1
 * [15:13] ddrtype          [12]    channelnum
 * [11]    rank_ch0         [10:9]  col_ch0
 * [8]     bk_ch0           [7:6]   cs0_row_ch0
 * [5:4]   cs1_row_ch0      [3:2]   bw_ch0
 * [1:0]   dbw_ch0
 */
#define SYS_REG_NUM_CH_SHIFT        12
#define SYS_REG_NUM_CH_MASK         1
#define SYS_REG_ROW_3_4_SHIFT(Ch)   (30 + (Ch))
#define SYS_REG_ROW_3_4_MASK        1
#define SYS_REG_RANK_SHIFT(Ch)      (11 + (Ch) * 16)
#define SYS_REG_RANK_MASK           1
#define SYS_REG_COL_SHIFT(Ch)       (9 + (Ch) * 16)
#define SYS_REG_COL_MASK            3
#define SYS_REG_BK_SHIFT(Ch)        (8 + (Ch) * 16)
#define SYS_REG_BK_MASK             1
#define SYS_REG_CS0_ROW_SHIFT(Ch)   (6 + (Ch) * 16)
#define SYS_REG_CS0_ROW_MASK        3
#define SYS_REG_CS1_ROW_SHIFT(Ch)   (4 + (Ch) * 16)
#define SYS_REG_CS1_ROW_MASK        3
#define SYS_REG_BW_SHIFT(Ch)        (2 + (Ch) * 16)
#define SYS_REG_BW_MASK             3

#endif /* RK3399_PMU_GRF_H_ */
