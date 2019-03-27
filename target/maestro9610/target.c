
#include <platform/if_pmic_s2mu004.h>

void target_init(void)
{
}

void target_init_for_usb(void)
{
	muic_sw_usb();
}
