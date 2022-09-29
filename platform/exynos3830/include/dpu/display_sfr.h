/* Copyright (c) 2018 Samsung Electronics Co, Ltd.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * Copyright@ Samsung Electronics Co. LTD
 * Manseok Kim <manseoks.kim@samsung.com>

 * Alternatively, Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __DISPLAY_SFR_H__
#define __DISPLAY_SFR_H__


/* DMA & DPP SFR Base Address */
#if defined(CONFIG_DISPLAY_DRAWLOGO)
#define DPP_IDMAG0_BASE_ADDR		0x13054000
#define DPP_IDMAG0_DMA_ADDR		0x130B4000
#define DPP_IDMAG0_DMA_COM_ADDR		0x130B0000

#define DPP_IDMAG1_BASE_ADDR		0x13052000
#define DPP_IDMAG1_DMA_ADDR		0x130B2000
#else
#define DPP_IDMAG0_BASE_ADDR		0x13052000
#define DPP_IDMAG0_DMA_ADDR		0x130B2000
#define DPP_IDMAG0_DMA_COM_ADDR		0x130B0000

#define DPP_IDMAG1_BASE_ADDR		0x13054000
#define DPP_IDMAG1_DMA_ADDR		0x130B4000
#endif

/* DECON SFR Base Address */
#define DECON0_BASE_ADDR		0x13060000
#define SYSMMU_DPU1_BASE_ADDR		0x130C0000
#define DPU_SYSREG_BASE_ADDR		0x13020000

/* DSIM & PHY SFR Base Address */
#define DSIM0_BASE_ADDR			0x13070000
#define DPHY_BASE_ADDR			0x13080000
#define DPHY_EX_BASE_ADDR		0x148E0000

#endif /* __DISPLAY_SFR_H__ */
