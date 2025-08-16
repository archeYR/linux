// SPDX-License-Identifier: GPL-2.0
/**
 * Copyright (c) 2012 Alexey Makhalov (makhaloff@gmail.com)
 * Copyright (c) 2011 Richard Ian Taylor.
 * Copyright (c) 2018 Joey Hewitt.
 *
 * This file is part of the iDroid Project. (http://www.idroidproject.org).
 */


#include <asm/mach/arch.h>
#include <asm/system_misc.h>
#include <linux/io.h>

// From the clksrc
extern void s5l8930_timer_reboot(void);

static void s5l8930_restart(enum reboot_mode mode, const char *cmd)
{
	printk(KERN_INFO "%s\n", __func__);
	s5l8930_timer_reboot();
}

static const char *const s5l8930_compat[] __initconst = {
	"samsung,s5l8930",
	NULL
};

DT_MACHINE_START(S5L8930, "Samsung S5L8930")
	.dt_compat	= s5l8930_compat,
	.restart        = s5l8930_restart,
	// TODO CPU idle?
MACHINE_END
