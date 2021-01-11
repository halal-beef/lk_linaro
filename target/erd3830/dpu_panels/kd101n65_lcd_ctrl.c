/*
 * drivers/video/decon/panels/kd101n65_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <dev/dpu/lcd_ctrl.h>
#include <dev/dpu/mipi_dsi_cmd.h>
#include <dev/dpu/dsim.h>

#define FRAME_WIDTH  (1920) //(800)
#define FRAME_HEIGHT (1080) //(1280)

struct exynos_panel_info kd101n65_lcd_info = {
	.mode = 0,//video mode

	.vbp = 36,
	.vfp = 4,
	.vsa = 5,
	.hbp = 148,
	.hfp = 88,
	.hsa = 44,

	.xres = FRAME_WIDTH,
	.yres = FRAME_HEIGHT,

	/* Mhz */
	.hs_clk = 1200,
	.esc_clk = 20,

	.dphy_pms = {2, 185, 1, 0x9d8a}, /* pmsk xxxxx for testing */
	/* Maybe, width and height will be removed */
	.fps = 60,
	.data_lane = 4,
};

//lrh add start
#define REGFLAG_DELAY 0xFD
#define REGFLAG_END_OF_TABLE 0xFE
/*
 * kd101n65 lcd init sequence
 *
 * Parameters
 *	- mic_enabled : if mic is enabled, MIC_ENABLE command must be sent
 *	- mode : LCD init sequence depends on command or video mode
 */

void kd101n65_lcd_init(unsigned int id, struct exynos_panel_info *lcd)
{

	//unsigned char data[4] = {0xb0,0x5a,0,0};
	//unsigned char buf[4] = {0};
//	unsigned long wr_d = 0;

	dsim_err("%s +\n", __func__);

#if 0
	wr_d = 0x11;
	if (dsim_wr_data(id, 0x15, wr_d, 2) < 0)
		dsim_err("fail to send 0x11 display on command.\n");
	mdelay(300);

	wr_d = 0x29;
	if (dsim_wr_data(id, 0x15, wr_d, 2) < 0)
		dsim_err("fail to send 0x29display on command.\n");
#endif
	dsim_err("%s -\n", __func__);
}

void kd101n65_lcd_enable_exynos(unsigned int id)
{
	dsim_dbg("%s -\n", __func__);
	/*
	if (dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE,
				(unsigned long)SEQ_DISPLAY_ON[0], 0) < 0)
		dsim_err("fail to send SEQ_DISPLAY_ON command.\n");
	*/
}

void kd101n65_lcd_disable(unsigned int id)
{
	dsim_dbg("%s -\n", __func__);
	/* This function needs to implement */
}

/*
 * Set gamma values
 *
 * Parameter
 *	- backlightlevel : It is from 0 to 26.
 */
int kd101n65_lcd_gamma_ctrl(unsigned int id, unsigned int backlightlevel)
{
	/* This will be implemented */
	return 0;
}

int kd101n65_lcd_gamma_update(int id)
{
	/* This will be implemented */
	return 0;
}

static int kd101n65_wqhd_dump(int dsim)
{
	int ret = 0;

	dsim_info(" + %s\n", __func__);
	dsim_info(" - %s\n", __func__);
	return ret;

}

int kd101n65_lcd_dump(int id)
{
	kd101n65_wqhd_dump(id);
	return 0;
}

