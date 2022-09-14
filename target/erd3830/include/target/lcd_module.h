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
#ifndef __LCD_MODULE_H__
#define __LCD_MODULE_H__

#include <dpu/dsim.h>

int cm_fill_id(struct dsim_device *dsim);
int cm_read_id(struct dsim_device *dsim);
struct dsim_lcd_driver *cm_get_panel_info(struct dsim_device *dsim);
struct lcd_driver *get_lcd_drv_ops(void);
struct exynos_panel_info *common_get_lcd_info(void);

extern struct dsim_lcd_driver s6e3fa0_mipi_lcd_driver;
extern struct dsim_lcd_driver nt36672a_mipi_lcd_driver;
extern struct dsim_lcd_driver s6e3ha8_mipi_lcd_driver;
extern struct dsim_lcd_driver s6e3ha9_mipi_lcd_driver;


extern struct exynos_panel_info s6e3fa0_lcd_info;
extern struct exynos_panel_info nt36672a_lcd_info;
extern struct exynos_panel_info s6e3ha8_lcd_info;
extern struct exynos_panel_info s6e3ha9_lcd_info;


#endif /*__LCD_MODULE_H__ */
