/** @file
  RK3399 General Register File offsets.

  Copyright (c) 2017, Rockchip Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RK3399_GRF_H_
#define RK3399_GRF_H_

/* GPIO IOMUX (GRF) */
#define GRF_GPIO2A_IOMUX        0xE000
#define GRF_GPIO2B_IOMUX        0xE004
#define GRF_GPIO2C_IOMUX        0xE008
#define GRF_GPIO2D_IOMUX        0xE00C
#define GRF_GPIO3A_IOMUX        0xE010
#define GRF_GPIO3B_IOMUX        0xE014
#define GRF_GPIO3C_IOMUX        0xE018
#define GRF_GPIO3D_IOMUX        0xE01C
#define GRF_GPIO4A_IOMUX        0xE020
#define GRF_GPIO4B_IOMUX        0xE024
#define GRF_GPIO4C_IOMUX        0xE028
#define GRF_GPIO4D_IOMUX        0xE02C

/* GPIO pull control (GRF) */
#define GRF_GPIO2A_P            0xE040
#define GRF_GPIO2B_P            0xE044
#define GRF_GPIO2C_P            0xE048
#define GRF_GPIO2D_P            0xE04C
#define GRF_GPIO3A_P            0xE050
#define GRF_GPIO3B_P            0xE054
#define GRF_GPIO3C_P            0xE058
#define GRF_GPIO3D_P            0xE05C
#define GRF_GPIO4A_P            0xE060
#define GRF_GPIO4B_P            0xE064
#define GRF_GPIO4C_P            0xE068
#define GRF_GPIO4D_P            0xE06C

/* Secure GRF */
#define SGRF_SOC_CON20          0x8050

#define GRF_RK3399_HDMI_VOP_SEL_MASK  (1U << 6)
#define GRF_RK3399_HDMI_VOP_SEL_B     (0U << 6)
#define GRF_RK3399_HDMI_VOP_SEL_L     (1U << 6)

#endif /* RK3399_GRF_H_ */
