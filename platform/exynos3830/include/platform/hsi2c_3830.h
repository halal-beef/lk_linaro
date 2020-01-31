#ifndef __HSI2C_3830_H__
#define __HSI2C_3830_H__

#include <dev/exynos_hsi2c.h>
#include <platform/sfr.h>

#define MAX_HSI2C_CHAN	(5)
#define CLK_PERI_HSI2C	(200000000)
#define CLK_CMGP_USI	(200000000)

enum {
	USI_HSI2C_0,
	USI_HSI2C_1,
	USI_HSI2C_2,
	USI_CMGP_00,
	USI_CMGP_01,
};

static struct hsi2c_chan_info exynos3830_hsi2c_chan_info[] = {
	{
		/* USI_HSI2C_0 */
		.hsi2c_addr = EXYNOS3830_BLK_HSI2C_0,
		.sysreg_addr = EXYNOS3830_HSI2C_0_SW_CONF,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPC1CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPC1CON,
		.scl_pin = 0,
		.sda_pin = 1,
		.con_val = 2,
		.clk = CLK_PERI_HSI2C,
	}, {
		/* USI_HSI2C_1 */
		.hsi2c_addr = EXYNOS3830_BLK_HSI2C_1,
		.sysreg_addr = EXYNOS3830_HSI2C_1_SW_CONF,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPC1CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPC1CON,
		.scl_pin = 2,
		.sda_pin = 3,
		.con_val = 2,
		.clk = CLK_PERI_HSI2C,
	}, {
		/* USI_HSI2C_2 */
		.hsi2c_addr = EXYNOS3830_BLK_HSI2C_2,
		.sysreg_addr = EXYNOS3830_HSI2C_2_SW_CONF,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPC1CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPC1CON,
		.scl_pin = 4,
		.sda_pin = 5,
		.con_val = 2,
		.clk = CLK_PERI_HSI2C,
	}, {
		/* USI_CMGP_00 */
		.hsi2c_addr = EXYNOS3830_BLK_USI_CMGP00,
		.sysreg_addr = EXYNOS3830_USI_CMGP0_SW_CONF,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPM0CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPM1CON,
		.scl_pin = 0,
		.sda_pin = 0,
		.con_val = 2,
		.clk = CLK_CMGP_USI,
	}, {
		/* USI_CMGP_01 */
		.hsi2c_addr = EXYNOS3830_BLK_USI_CMGP01,
		.sysreg_addr = EXYNOS3830_USI_CMGP1_SW_CONF,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPM4CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPM5CON,
		.scl_pin = 0,
		.sda_pin = 0,
		.con_val = 2,
		.clk = CLK_CMGP_USI,
	}
};

static struct hsi2c_chan_info *match_hsi2c_chan_info(void)
{
	return exynos3830_hsi2c_chan_info;
}
#endif // __HSI2C_3830_H__
