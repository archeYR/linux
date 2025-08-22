// SPDX-License-Identifier: GPL-2.0-only
/*
 * Device framebuffer initialization file
 */

#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/platform_device.h>
#include <linux/platform_data/simplefb.h>
#include "../../../../../drivers/gpu/drm/gma500/psb_drv.h"
#include "../../../../../drivers/gpu/drm/gma500/psb_intel_reg.h"

static int gma_device_list[] = {
	/* Poulsbo */
	0x8108,
	0x8109,
	/* Oak Trail */
	0x4101,
	0x4102,
	0x4103,
	0x4104,
	0x4105,
	0x4106,
	0x4107,
	0x4108,
    /* Medfield */
	0x0130,
	0x0131,
	0x0132,
	0x0133,
	0x0134,
	0x0135,
	0x0136,
	0x0137,
    /* Clover Trail+ */
	0x08c0,
	0x08c7,
	0x08c8,
    /* Merrifield/Moorefield */
	0x1180,
	0x1181,
	0x1182,
	0x1183,
	0x1184,
	0x1185,
	0x1186,
	0x1187,
	0x1480,
};

static int simplefb_sfi_init(void)
{
	unsigned int fb_address = 0;
    unsigned long device_id = 0;
    void *bar0_address;
    struct pci_dev * gma_device = NULL;
	static const char simplefb_resname[] = "FB";
	static struct simplefb_platform_data mode;
	struct resource res[1];

	/* Look for GMA devices */
	while((device_id < ARRAY_SIZE(gma_device_list))) {
		gma_device = pci_get_device(PCI_VENDOR_ID_INTEL,
									gma_device_list[device_id],
									gma_device);
		if (gma_device && (gma_device->class >> 8) == PCI_CLASS_DISPLAY_VGA)
			break;
		device_id++;
	}

	/* Get framebuffer address from PSB_BSM PCI register */
	if (gma_device == NULL ||
		pci_read_config_dword(
			gma_device, PSB_BSM, &fb_address) == PCIBIOS_DEVICE_NOT_FOUND)
	{
		pr_err("No GMA devices found\n");
		return -ENODEV;
	}

	bar0_address = pci_iomap(gma_device, 0, 0);

	/* Get device resolution from DPI_RESOLUTION GMA register */
	mode.width = ioread16(bar0_address + DPI_RESOLUTION_REG);
	mode.height = ioread16(bar0_address + DPI_RESOLUTION_REG + 2);

	/* Get framebuffer format from DSPASTRIDE and DSPACNTR GMA registers */
	mode.stride = ioread16(bar0_address + DSPASTRIDE);
	switch(ioread32(bar0_address + DSPACNTR) & DISPPLANE_PIXFORMAT_MASK)
	{
		case DISPPLANE_8BPP:
		case DISPPLANE_15_16BPP:
			pr_err("simplefb does not support current GMA framebuffer format\n");
			fallthrough;
		case DISPPLANE_16BPP:
			mode.format = "r5g6b5";
			break;
		case DISPPLANE_32BPP_NO_ALPHA:
			mode.format = "x8r8g8b8";
			break;
		case DISPPLANE_32BPP:
			mode.format = "a8r8g8b8";
			break;
		default:
			pr_err("Unknown GMA framebuffer format\n");
			mode.format = "r5g6b5";
			break;
	}

	/* setup IORESOURCE_MEM as framebuffer memory */
	memset(&res[0], 0, sizeof(res[0]));
	res[0].flags = IORESOURCE_MEM;
	res[0].name = simplefb_resname;
	res[0].start = fb_address;
	res[0].end = fb_address + mode.height * mode.stride - 1;

	platform_device_register_resndata(NULL, "simple-framebuffer", 0,
					  &res[0], 1, &mode, sizeof(mode));
	return 0;
}

device_initcall(simplefb_sfi_init);
