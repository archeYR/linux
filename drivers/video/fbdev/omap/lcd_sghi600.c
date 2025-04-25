// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * LCD panel support for the Palm Tungsten E
 *
 * Original version : Romain Goyet <r.goyet@gmail.com>
 * Current version : Laurent Gonzalez <palmte.linux@free.fr>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "omapfb.h"

static struct lcd_panel sghi600_panel = {
	.name		= "sghi600",
	.config		= OMAP_LCDC_PANEL_TFT,

	.data_lines	= 16,
	.bpp		= 16,
	.pixel_clock	= 6000,
	.x_res		= 320,
	.y_res		= 240,
	.hsw		= 10,
	.hfp		= 10,
	.hbp		= 20,
	.vsw		= 2,
	.vfp		= 4,
	.vbp		= 2,
	.pcd		= 0,
};

static int sghi600_panel_probe(struct platform_device *pdev)
{
	omapfb_register_panel(&sghi600_panel);
	return 0;
}

static struct platform_driver sghi600_panel_driver = {
	.probe		= sghi600_panel_probe,
	.driver		= {
		.name	= "lcd_sghi600",
	},
};

module_platform_driver(sghi600_panel_driver);

MODULE_AUTHOR("Romain Goyet <r.goyet@gmail.com>, Laurent Gonzalez <palmte.linux@free.fr>");
MODULE_DESCRIPTION("LCD panel support for the Palm Tungsten E");
MODULE_LICENSE("GPL");
