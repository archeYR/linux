// SPDX-License-Identifier: GPL-2.0-only
/*
 * linux/arch/arm/mach-omap1/board-nokia770.c
 *
 * Modified from board-generic.c
 */
#include <linux/clkdev.h>
#include <linux/input-event-codes.h>
#include <linux/irq.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/machine.h>
#include <linux/gpio/property.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/input.h>
#include <linux/omapfb.h>

#include <linux/spi/spi.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#include <linux/platform_data/keypad-omap.h>
#include <linux/platform_data/omap1_bl.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include "mux.h"
#include "hardware.h"
#include "usb.h"
#include "common.h"
#include "clock.h"
#include "mmc.h"

static const unsigned int sghi600_keymap[] = {
	KEY(0, 0, KEY_Q),
	KEY(1, 0, KEY_W),
	KEY(2, 0, KEY_E),
	KEY(3, 0, KEY_R),
	KEY(4, 0, KEY_T),
	KEY(0, 4, KEY_Y),
	KEY(1, 4, KEY_U),
	KEY(2, 4, KEY_I),
	KEY(3, 4, KEY_O),
	KEY(4, 4, KEY_P),
	KEY(0, 1, KEY_A),
	KEY(1, 1, KEY_S),
	KEY(2, 1, KEY_D),
	KEY(3, 1, KEY_F),
	KEY(4, 1, KEY_G),
	KEY(0, 5, KEY_H),
	KEY(1, 5, KEY_J),
	KEY(2, 5, KEY_K),
	KEY(3, 5, KEY_L),
	KEY(4, 5, KEY_BACKSPACE),
	KEY(0, 2, KEY_FN),
	KEY(1, 2, KEY_Z),
	KEY(2, 2, KEY_X),
	KEY(3, 2, KEY_C),
	KEY(4, 2, KEY_V),
	KEY(0, 6, KEY_B),
	KEY(1, 6, KEY_N),
	KEY(2, 6, KEY_M),
	KEY(3, 6, KEY_DOT),
	KEY(4, 6, KEY_ENTER),
	KEY(0, 3, KEY_CAPSLOCK),
	KEY(2, 3, KEY_LEFTMETA), // asterisk?
	KEY(3, 3, KEY_SPACE),
	KEY(4, 3, KEY_MUTE),
	KEY(5, 1, KEY_MAIL),
	KEY(5, 2, KEY_LEFTCTRL), // left button nex to circle pad
	KEY(5, 3, KEY_RIGHTSHIFT), // right button next to circle pad
	KEY(5, 4, KEY_TAB), // KEY_HOME
	KEY(5, 5, KEY_SLASH), // Answer phone
	KEY(5, 6, KEY_BACK), // Returning arrow
	KEY(6, 0, KEY_UP),
	KEY(6, 1, KEY_DOWN),
	KEY(6, 2, KEY_LEFT),
	KEY(6, 3, KEY_RIGHT),
	KEY(6, 4, KEY_OK),
	KEY(6, 6, KEY_ESC), // Button on right side
	KEY(7, 0, KEY_MINUS), // Decline phone
	KEY(7, 3, KEY_VOLUMEDOWN),
	KEY(7, 4, KEY_VOLUMEUP),
	KEY(7, 6, KEY_COMMA),
	// Where is the wheel on right side?
};

static struct resource sghi600_kp_resources[] = {
	[0] = {
		.start	= INT_KEYBOARD,
		.end	= INT_KEYBOARD,
		.flags	= IORESOURCE_IRQ,
	},
};

static const struct matrix_keymap_data sghi600_keymap_data = {
	.keymap		= sghi600_keymap,
	.keymap_size	= ARRAY_SIZE(sghi600_keymap),
};

static struct omap_kp_platform_data sghi600_kp_data = {
	.rows		= 8,
	.cols		= 8,
	.keymap_data	= &sghi600_keymap_data,
	.delay		= 4,
};

static struct platform_device sghi600_kp_device = {
	.name		= "omap-keypad",
	.id		= -1,
	.dev		= {
		.platform_data = &sghi600_kp_data,
	},
	.num_resources	= ARRAY_SIZE(sghi600_kp_resources),
	.resource	= sghi600_kp_resources,
};

static struct platform_device sghi600_lcd_device = {
	.name		= "lcd_sghi600",
	.id		= -1,
};

static struct omap_backlight_config sghi600_backlight_config = {
	.default_intensity	= 0x30,
};

static struct platform_device sghi600_backlight_device = {
	.name		= "omap-bl",
	.id		= -1,
	.dev		= {
		.platform_data	= &sghi600_backlight_config,
	},
};

static struct platform_device *sghi600_devices[] __initdata = {
	&sghi600_kp_device,
	&sghi600_lcd_device,
	&sghi600_backlight_device,
};

/* assume no Mini-AB port */

static struct omap_usb_config sghi600_usb_config __initdata = {
	.register_dev	= 1,	/* Mini-B only receptacle */
	.hmc_mode	= 0,
	.pins[0]	= 2,
};

static const struct omap_lcd_config sghi600_lcd_config __initconst = {
	.ctrl_name	= "internal",
};

#if IS_ENABLED(CONFIG_MMC_OMAP)
static struct omap_mmc_platform_data _sghi600_mmc_data = {
	.nr_slots                       = 1,
	.slots[0]       = {
		.ocr_mask               = MMC_VDD_29_30|MMC_VDD_30_31,
		.name                   = "mmcblk",
	},
};

static struct omap_mmc_platform_data *sghi600_mmc_data[OMAP16XX_NR_MMC] = {
	[0] = &_sghi600_mmc_data,
};

static void __init sghi600_mmc_init(void)
{
	/* Only the second MMC controller is used */
	omap1_init_mmc(sghi600_mmc_data, OMAP16XX_NR_MMC);
}

#else
static inline void sghi600_mmc_init(void)
{
}
#endif

static void __init omap_sghi600_init(void)
{
	platform_add_devices(sghi600_devices, ARRAY_SIZE(sghi600_devices));

	omapfb_set_lcd_config(&sghi600_lcd_config);
	omap1_usb_init(&sghi600_usb_config);

	sghi600_mmc_init();
}

MACHINE_START(SGH_I600, "Samsung SGH-i600")
	.atag_offset	= 0x100,
	.map_io		= omap1_map_io,
	.init_early     = omap1_init_early,
	.init_irq	= omap1_init_irq,
	.init_machine	= omap_sghi600_init,
	.init_late	= omap1_init_late,
	.init_time	= omap1_timer_init,
	.restart	= omap1_restart,
MACHINE_END
