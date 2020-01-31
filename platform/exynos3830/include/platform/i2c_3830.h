#ifndef __I2C_3830_H__
#define __I2C_3830_H__

#include <platform/sfr.h>
#include <dev/exynos_i2c.h>

#define MAX_I2C_CHAN	(7)
#define CLK_I2C_PCLK	(400000000)

enum {
	I2C_0,
	I2C_1,
	I2C_2,
	I2C_3,
	I2C_4,
	I2C_5,
	I2C_6,
};

static struct i2c_chan_info exynos3830_i2c_chan_info[] = {
	{
		/* I2C0 */
		.i2c_base = EXYNOS3830_I2C0,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPP0CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPP0CON,
		.scl_pin = 0,
		.sda_pin = 1,
		.clk = CLK_I2C_PCLK,
	}, {
		/* I2C1 */
		.i2c_base = EXYNOS3830_I2C1,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPP0CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPP0CON,
		.scl_pin = 2,
		.sda_pin = 3,
		.clk = CLK_I2C_PCLK,
	}, {
		/* I2C2 */
		.i2c_base = EXYNOS3830_I2C2,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPP0CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPP0CON,
		.scl_pin = 4,
		.sda_pin = 5,
		.clk = CLK_I2C_PCLK,
	}, {
		/* I2C3 */
		.i2c_base = EXYNOS3830_I2C3,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPP1CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPP1CON,
		.scl_pin = 0,
		.sda_pin = 1,
		.clk = CLK_I2C_PCLK,
	}, {
		/* I2C4 */
		.i2c_base = EXYNOS3830_I2C4,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPP1CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPP1CON,
		.scl_pin = 2,
		.sda_pin = 3,
		.clk = CLK_I2C_PCLK,
	}, {
		/* I2C5 */
		.i2c_base = EXYNOS3830_I2C5,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPA3CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPA3CON,
		.scl_pin = 5,
		.sda_pin = 6,
		.clk = CLK_I2C_PCLK,
	}, {
		/* I2C6 */
		.i2c_base = EXYNOS3830_I2C6,
		.gpio_scl = (unsigned int *)EXYNOS3830_GPA3CON,
		.gpio_sda = (unsigned int *)EXYNOS3830_GPA4CON,
		.scl_pin = 7,
		.sda_pin = 0,
		.clk = CLK_I2C_PCLK,
	},
};

static struct i2c_chan_info *match_i2c_chan_info(void)
{
	return exynos3830_i2c_chan_info;
}

#endif // __I2C_3830_H__
