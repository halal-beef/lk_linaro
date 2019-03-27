/*
 * usb.c
 *
 *  Created on: 2019. 3. 25.
 *      Author: sunghyun.na
 */

#include <debug.h>
#include <string.h>
#include <reg.h>
#include <malloc.h>
#include <lk/init.h>
#include <list.h>
#include <err.h>

#include <usb-def.h>
#include "dev/usb/gadget.h"
#include "dev/usb/dwc3-config.h"
#include "dev/usb/phy-samsung-usb-cal.h"
#include "platform/exynos9610.h"

void gadget_probe_pid_vid_version(unsigned short *vid, unsigned short *pid, unsigned short *bcd_version)
{
	*vid = 0x18D1;
	*pid = 0x0002;
	*bcd_version = 0x0100;
}

static const char vendor_str[] = "Samsung Semiconductor, S.LSI Division";
static const char product_str[] = "Exynos9610 LK Bootloader";
static char serial_id[16] = "No Serial";

int gadget_get_vendor_string(void)
{
	return get_str_id(vendor_str, strlen(vendor_str));
}

int gadget_get_product_string(void)
{

	return get_str_id(product_str, strlen(product_str));
}

static const char *make_serial_string(void)
{
	u8 i, j;
	int chip_id[2];

	if (strcmp(serial_id, "No Serial"))
		return serial_id;

	chip_id[0] = readl(EXYNOS9610_PRO_ID + CHIPID0_OFFSET);
	chip_id[1] = readl(EXYNOS9610_PRO_ID + CHIPID1_OFFSET) & 0xFFFF;
	for (j = 0; j < 2; j++) {
		u32 hex;
		char *str;

		hex = chip_id[j];
		str = &serial_id[j * 8];
		for (i = 0; i < 8; i++) {
			if ((hex & 0xF) > 9)
				*str++ = 'a' + (hex & 0xF) - 10;
			else
				*str++ = '0' + (hex & 0xF);
			hex >>= 4;
		}
	}
	return serial_id;
}

int gadget_get_serial_string(void)
{
	return get_str_id(make_serial_string(), 12);
}

const char *fastboot_get_product_string(void)
{
	return product_str;
}

const char *fastboot_get_serialno_string(void)
{
	return make_serial_string();
}

static unsigned int dwc3_isr_num = (186 + 32);

int dwc3_plat_init(struct dwc3_plat_config *plat_config)
{
	plat_config->base = (void *) 0x13200000;
	plat_config->num_hs_phy = 1;
	plat_config->array_intr = &dwc3_isr_num;
	plat_config->num_intr = 1;
	strcpy(plat_config->ssphy_type, "snps_gen1");

	return 0;
}

static struct dwc3_dev_config dwc3_dev_config = {
	.speed = "high",
	.m_uEventBufDepth = 64,
	.m_uCtrlBufSize = 128,
	.m_ucU1ExitValue = 10,
	.m_usU2ExitValue = 257,
};

int dwc3_dev_plat_init(void **base_addr, struct dwc3_dev_config **plat_config)
{
	*base_addr = (void *) (0x13200000);
	*plat_config = &dwc3_dev_config;
	return 0;
}

static struct exynos_usb_tune_param usbcal_20phy_tune[] = {
	{ .name = "tx_pre_emp", .value = 0x3, },
	{ .name = "tx_vref", .value = 0x7, },
	{ .name = "rx_sqrx", .value = 0x5, },
	{ .name = "utim_clk", .value = USBPHY_UTMI_PHYCLOCK, },
	{ .value = EXYNOS_USB_TUNE_LAST, },
};

static struct exynos_usbphy_info usbphy_cal_info = {
	.version = EXYNOS_USBCON_VER_03_0_0,
	.refclk = USBPHY_REFCLK_DIFF_26MHZ,
	.refsel = USBPHY_REFSEL_CLKCORE,
	.not_used_vbus_pad = true,
	.regs_base = (void *) 0x131D0000,
	.tune_param = usbcal_20phy_tune,
	.hs_rewa = 1,
};

static void register_phy_cal_infor(uint level)
{
	phy_usb_exynos_register_cal_infor(&usbphy_cal_info);
}
LK_INIT_HOOK(register_phy_cal_infor, &register_phy_cal_infor, LK_INIT_LEVEL_KERNEL);

void phy_usb_exynos_system_init(int num_phy_port, bool en)
{
	u32 reg;

	if (num_phy_port == 0) {
		/* 2.0 HS PHY */
		/* PMU Isolation release */
		reg = readl((void *)(0x11860000 + 0x704));
		if (en)
			reg |= 0x2;
		else
			reg &= ~0x2;
		writel(reg, (void *)(0x11860000 + 0x704));

		/* CCI Enable */
		reg = readl((void *)(0x13010000 + 0x700));
		if (en)
			reg |= (0x3 << 12);
		else
			reg &= ~(0x3 << 12);
		writel(reg, (void *)(0x13010000 + 0x700));
	} else {
		/* 3.0 PHY */
		reg = readl((void *)(0x11860000 + 0x704));
		if (en)
			reg |= 0x1;
		else
			reg &= ~0x1;
		writel(reg, (void *)(0x11860000 + 0x704));
	}
}

/* Fastboot command related function */
#include <pit.h>
#include <dev/rpmb.h>
#include <dev/scsi.h>
void platform_prepare_reboot(void)
{
	bdev_t *dev;
	status_t ret;

	/* Send SSU to UFS */
	dev = bio_open("scsissu");
	if (!dev) {
		printf("error opening block device\n");
	} else {
		ret = scsi_start_stop_unit(dev);
		if (ret != NO_ERROR)
			printf("scsi ssu error!\n");

		bio_close(dev);
	}
}

void platform_do_reboot(const char *cmd_buf)
{
	if(!memcmp(cmd_buf, "reboot-bootloader", strlen("reboot-bootloader")))
		writel(CONFIG_RAMDUMP_MODE, CONFIG_RAMDUMP_SCRATCH);
	else
		writel(0, CONFIG_RAMDUMP_SCRATCH);

	writel(0x1, EXYNOS9610_SWRESET);
	return;
}
