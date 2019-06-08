/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include <debug.h>
#include <stdlib.h>
#include <lib/sysparam.h>
#include <platform/lock.h>

void lock(int state)
{
	unsigned int env_val = 0;

	if (sysparam_read(LOCK_SYSPARAM_NAME, &env_val, sizeof(env_val)) > 0) {
		/* The device should prompt users to warn them that
		  they may encounter problems with unofficial images.
		  After acknowledging, a factory data reset should be
		  done to prevent unauthorized data access. */
		if ((env_val == 1) && (state == 0)) {

		}

		env_val = state;
		sysparam_remove(LOCK_SYSPARAM_NAME);
	} else {
		env_val = state;
	}
	sysparam_add(LOCK_SYSPARAM_NAME, &env_val, sizeof(env_val));

	sysparam_write();
}

int get_lock_state(void)
{
	unsigned int env_val = 0;

	if (sysparam_read(LOCK_SYSPARAM_NAME, &env_val, sizeof(env_val)) <= 0)
		return env_val;

	return (int) env_val;
}

void lock_critical(int state)
{
	unsigned int env_val = 0;

	if (sysparam_read(LOCK_CRITICAL_SYSPARAM_NAME, &env_val, sizeof(env_val)) > 0) {
		/* Transitioning from locked to unlocked state should
		  require a physical interaction with the device. */
		if ((env_val == 1) && (state == 0)) {

		}
		sysparam_remove(LOCK_CRITICAL_SYSPARAM_NAME);
	}
	env_val = state;

	sysparam_add(LOCK_CRITICAL_SYSPARAM_NAME, &env_val, sizeof(env_val));
	sysparam_write();
}

int get_unlock_ability(void)
{
	/* If get_unlock_ability is "0" the user needs to boot
	  to the home screen, go into the Settings > System > Developer
	  options menu and enable the OEM unlocking option to set
	  unlock_ability to: "1" */
	int unlock_ability = 1;

	return unlock_ability;
}
