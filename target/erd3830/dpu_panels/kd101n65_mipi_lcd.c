#include <dev/dpu/lcd_ctrl.h>
#include <dev/dpu/exynos_panel.h>
#include <dev/dpu/dsim.h>

#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0
#define DEFAULT_BRIGHTNESS 0

/* PANEL_ID : ID3[23:16]-ID2[15:8]-ID1[7:0] */
#define PANEL_ID        0xff /* value was confirmed when bringup */
extern struct exynos_panel_info kd101n65_lcd_info;
extern void kd101n65_lcd_init(unsigned int id, struct exynos_panel_info *lcd);

static int kd101n65_get_id(struct dsim_device *dsim)
{
	dsim_info("%s panel ID is (0x%08x)\n", __func__, PANEL_ID);
	return PANEL_ID;
}

static struct exynos_panel_info *kd101n65_get_lcd_info(void)
{
	dsim_info("%s \n", __func__);
	return &kd101n65_lcd_info;
}

static int kd101n65_probe(struct dsim_device *dsim)
{
	printf("%s \n", __func__);
	//kd101n65_lcd_init(dsim->id, dsim->lcd_info);
	return 1;
}

static int kd101n65_displayon(struct dsim_device *dsim)
{
	printf("%s \n", __func__);
	//kd101n65_lcd_init(dsim->id, dsim->lcd_info);
	//kd101n65_lcd_enable_exynos(dsim->id);
	return 1;
}

static int kd101n65_suspend(struct dsim_device *dsim)
{
	return 1;
}

static int kd101n65_resume(struct dsim_device *dsim)
{
	return 1;
}

struct dsim_lcd_driver kd101n65_mipi_lcd_driver = {
	.get_id		= kd101n65_get_id,
	.get_lcd_info	= kd101n65_get_lcd_info,
	.probe		= kd101n65_probe,
	.displayon	= kd101n65_displayon,
	.suspend	= kd101n65_suspend,
	.resume		= kd101n65_resume,
};
