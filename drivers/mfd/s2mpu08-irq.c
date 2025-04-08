
/*
 * s2mpu08-irq.c - Interrupt controller support for S2MPU08
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/samsung/s2mpu08.h>

#define MFD_DEV_NAME "s2mpu08"

static const u8 s2mpu08_mask_reg[] = {
	/* TODO: Need to check other INTMASK */
	[PMIC_INT1] = 	S2MPU08_REG_INT1M,
	[PMIC_INT2] =	S2MPU08_REG_INT2M,
	[PMIC_INT3] = 	S2MPU08_REG_INT3M,
};

static struct i2c_client *get_i2c(struct s2mpu08_dev *s2mpu08,
				enum s2mpu08_irq_source src)
{
	switch (src) {
	case PMIC_INT1 ... PMIC_INT3:
		return s2mpu08->pmic;
	default:
		return ERR_PTR(-EINVAL);
	}
	return ERR_PTR(-EINVAL);
}

struct s2mpu08_irq_data {
	int mask;
	enum s2mpu08_irq_source group;
};

static const struct regmap_irq s2mpu08_irqs[] = {
	[S2MPU08_IRQ_PWRONF] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_PWRONF_MASK,
	},
	[S2MPU08_IRQ_PWRONR] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_PWRONR_MASK,
	},
	[S2MPU08_IRQ_JIGONBF] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_JIGONBF_MASK,
	},
	[S2MPU08_IRQ_JIGONBR] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_JIGONBR_MASK,
	},
	[S2MPU08_IRQ_ACOKBF] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_ACOKBF_MASK,
	},
	[S2MPU08_IRQ_ACOKBR] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_ACOKBR_MASK,
	},
	[S2MPU08_IRQ_PWRON1S] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_PWRON1S_MASK,
	},
	[S2MPU08_IRQ_MREVENT] = {
		.reg_offset = 0,
		.mask = S2MPU08_IRQ_MREVENT_MASK,
	},
	[S2MPU08_IRQ_RTC60S] = {
		.reg_offset = 1,
		.mask = S2MPU08_IRQ_RTC60S_MASK,
	},
	[S2MPU08_IRQ_RTCA1] = {
		.reg_offset = 1,
		.mask = S2MPU08_IRQ_RTCA1_MASK,
	},
	[S2MPU08_IRQ_RTCA0] = {
		.reg_offset = 1,
		.mask = S2MPU08_IRQ_RTCA0_MASK,
	},
	[S2MPU08_IRQ_SMPL] = {
		.reg_offset = 1,
		.mask = S2MPU08_IRQ_SMPL_MASK,
	},
	[S2MPU08_IRQ_RTC1S] = {
		.reg_offset = 1,
		.mask = S2MPU08_IRQ_RTC1S_MASK,
	},
	[S2MPU08_IRQ_WTSR] = {
		.reg_offset = 1,
		.mask = S2MPU08_IRQ_WTSR_MASK,
	},
	[S2MPU08_IRQ_SCLDO2] = {
		.reg_offset = 1,
		.mask = S2MPU08_IRQ_SCLDO2_MASK,
	},
	[S2MPU08_IRQ_INT120C] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_INT120C_MASK,
	},
	[S2MPU08_IRQ_INT140C] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_INT140C_MASK,
	},
	[S2MPU08_IRQ_TSD] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_TSD_MASK,
	},
	[S2MPU08_IRQ_SCLDO18] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_SCLDO18_MASK,
	},
	[S2MPU08_IRQ_SCLDO19] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_SCLDO19_MASK,
	},
	[S2MPU08_IRQ_SCLDO18] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_SCLDO35_MASK,
	},
	[S2MPU08_IRQ_MRBF] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_MRBF_MASK,
	},
	[S2MPU08_IRQ_MRBR] = {
		.reg_offset = 2,
		.mask = S2MPU08_IRQ_MRBR_MASK,
	},
};

static void s2mpu08_irq_lock(struct irq_data *data)
{
	struct s2mpu08_dev *s2mpu08 = irq_get_chip_data(data->irq);

	mutex_lock(&s2mpu08->irqlock);
}

static void s2mpu08_irq_sync_unlock(struct irq_data *data)
{
	struct s2mpu08_dev *s2mpu08 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < S2MPU08_IRQ_GROUP_NR; i++) {
		u8 mask_reg = s2mpu08_mask_reg[i];
		struct i2c_client *i2c = get_i2c(s2mpu08, i);

		if (mask_reg == S2MPU08_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		s2mpu08->irq_masks_cache[i] = s2mpu08->irq_masks_cur[i];

		s2mpu08_write_reg(i2c, s2mpu08_mask_reg[i],
				s2mpu08->irq_masks_cur[i]);
	}

	mutex_unlock(&s2mpu08->irqlock);
}

static const inline struct regmap_irq *
irq_to_s2mpu08_irq(struct s2mpu08_dev *s2mpu08, int irq)
{
	return &s2mpu08_irqs[irq - s2mpu08->irq_base];
}

static void s2mpu08_irq_mask(struct irq_data *data)
{
	struct s2mpu08_dev *s2mpu08 = irq_get_chip_data(data->irq);
	const struct regmap_irq *irq_data =
	    irq_to_s2mpu08_irq(s2mpu08, data->irq);

	if (irq_data->reg_offset >= S2MPU08_IRQ_GROUP_NR)
		return;

	s2mpu08->irq_masks_cur[irq_data->reg_offset] |= irq_data->mask;
}

static void s2mpu08_irq_unmask(struct irq_data *data)
{
	struct s2mpu08_dev *s2mpu08 = irq_get_chip_data(data->irq);
	const struct regmap_irq *irq_data =
	    irq_to_s2mpu08_irq(s2mpu08, data->irq);

	if (irq_data->reg_offset >= S2MPU08_IRQ_GROUP_NR)
		return;

	s2mpu08->irq_masks_cur[irq_data->reg_offset] &= ~irq_data->mask;
}

static struct irq_chip s2mpu08_irq_chip = {
	.name			= MFD_DEV_NAME,
	.irq_bus_lock		= s2mpu08_irq_lock,
	.irq_bus_sync_unlock	= s2mpu08_irq_sync_unlock,
	.irq_mask		= s2mpu08_irq_mask,
	.irq_unmask		= s2mpu08_irq_unmask,
};

int codec_notifier_flag = 0;

void set_codec_notifier_flag()
{
	codec_notifier_flag = 1;
}

static irqreturn_t s2mpu08_irq_thread(int irq, void *data)
{
	struct s2mpu08_dev *s2mpu08 = data;
	u8 irq_reg[S2MPU08_IRQ_GROUP_NR] = {0};
	u8 irq_src;
	// FOR CODEC
	//u8 irq1_codec, irq2_codec, irq3_codec, irq4_codec, status1;
	//u8 param1, param2, param3, param4, param5;
	int i, ret;

	//pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
	pr_err("%s: irq gpio pre-state(0x%02x)\n", __func__,
				gpio_get_value(s2mpu08->irq_gpio));

	ret = s2mpu08_read_reg(s2mpu08->i2c,
					S2MPU08_PMIC_REG_INTSRC, &irq_src);
	pr_err("%s: interrupt source(0x%02x)\n", __func__, irq_src);
	if (ret) {
		pr_err("%s:%s Failed to read interrupt source: %d\n",
			MFD_DEV_NAME, __func__, ret);
		return IRQ_NONE;
	}

	pr_info("%s: interrupt source(0x%02x)\n", __func__, irq_src);

	if (irq_src & S2MPU08_IRQSRC_PMIC) {
		/* PMIC_INT */
		ret = s2mpu08_bulk_read(s2mpu08->pmic, S2MPU08_REG_INT1,
				S2MPU08_NUM_IRQ_PMIC_REGS, &irq_reg[PMIC_INT1]);
		if (ret) {
			pr_err("%s:%s Failed to read pmic interrupt: %d\n",
				MFD_DEV_NAME, __func__, ret);
			return IRQ_NONE;
		}

		pr_info("%s: pmic interrupt(0x%02x, 0x%02x, 0x%02x)\n",
			 __func__, irq_reg[PMIC_INT1], irq_reg[PMIC_INT2], irq_reg[PMIC_INT3]);
	}

#if 0
	if(irq_src & S2MPU08_IRQSRC_CODEC) {
		if(s2mpu08->codec){
			pr_err("%s codec interrupt occur\n", __func__);
			ret = s2mpu08_read_reg(s2mpu08->codec, 0x1, &irq1_codec);
			ret = s2mpu08_read_reg(s2mpu08->codec, 0x2, &irq2_codec);
			ret = s2mpu08_read_reg(s2mpu08->codec, 0x3, &irq3_codec);
			ret = s2mpu08_read_reg(s2mpu08->codec, 0x4, &irq4_codec);
			ret = s2mpu08_read_reg(s2mpu08->codec, 0x9, &status1);
			ret = s2mpu08_read_reg(s2mpu08->close, 0x7B, &param1);
			ret = s2mpu08_read_reg(s2mpu08->close, 0x7C, &param2);
			ret = s2mpu08_read_reg(s2mpu08->close, 0x7E, &param3);
			ret = s2mpu08_read_reg(s2mpu08->close, 0x7F, &param4);
			ret = s2mpu08_read_reg(s2mpu08->close, 0x80, &param5);

			if(codec_notifier_flag)
				cod3035x_call_notifier(irq1_codec, irq2_codec, irq3_codec,
						irq4_codec, status1, param1, param2, param3, param4, param5);
		}
	}
#endif


	/* Apply masking */
	for (i = 0; i < S2MPU08_IRQ_GROUP_NR; i++)
		irq_reg[i] &= ~s2mpu08->irq_masks_cur[i];

	/* Report */
	for (i = 0; i < S2MPU08_IRQ_NR; i++) {
		if (irq_reg[s2mpu08_irqs[i].reg_offset] & s2mpu08_irqs[i].mask)
			handle_nested_irq(s2mpu08->irq_base + i);
	}

	return IRQ_HANDLED;
}

int s2mpu08_irq_init(struct s2mpu08_dev *s2mpu08)
{
	int i;
	int ret;
	u8 i2c_data;
	int cur_irq;

	if (!s2mpu08->irq_gpio) {
		dev_warn(s2mpu08->dev, "No interrupt specified.\n");
		s2mpu08->irq_base = 0;
		return 0;
	}

	if (!s2mpu08->irq_base) {
		dev_err(s2mpu08->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&s2mpu08->irqlock);

	s2mpu08->irq = gpio_to_irq(s2mpu08->irq_gpio);
	pr_info("%s:%s irq=%d, irq->gpio=%d\n", MFD_DEV_NAME, __func__,
			s2mpu08->irq, s2mpu08->irq_gpio);

	ret = gpio_request(s2mpu08->irq_gpio, "pmic_irq");
	if (ret) {
		dev_err(s2mpu08->dev, "%s: failed requesting gpio %d\n",
			__func__, s2mpu08->irq_gpio);
		return ret;
	}
	gpio_direction_input(s2mpu08->irq_gpio);
	gpio_free(s2mpu08->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < S2MPU08_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;

		s2mpu08->irq_masks_cur[i] = 0xff;
		s2mpu08->irq_masks_cache[i] = 0xff;

		i2c = get_i2c(s2mpu08, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (s2mpu08_mask_reg[i] == S2MPU08_REG_INVALID)
			continue;

		s2mpu08_write_reg(i2c, s2mpu08_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < S2MPU08_IRQ_NR; i++) {
		cur_irq = i + s2mpu08->irq_base;
		irq_set_chip_data(cur_irq, s2mpu08);
		irq_set_chip_and_handler(cur_irq, &s2mpu08_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	s2mpu08_write_reg(s2mpu08->i2c, S2MPU08_PMIC_REG_INTSRC_MASK, 0xff);
	/* Unmask s2mpu08 interrupt */
	ret = s2mpu08_read_reg(s2mpu08->i2c, S2MPU08_PMIC_REG_INTSRC_MASK,
			  &i2c_data);
	if (ret) {
		pr_err("%s:%s fail to read intsrc mask reg\n",
					 MFD_DEV_NAME, __func__);
		return ret;
	}

	//if(s2mpu08->codec)
	//	i2c_data &= ~(S2MPU08_IRQSRC_CODEC);

	i2c_data &= ~(S2MPU08_IRQSRC_PMIC);	/* Unmask pmic interrupt */
	s2mpu08_write_reg(s2mpu08->i2c, S2MPU08_PMIC_REG_INTSRC_MASK,
			   i2c_data);

	pr_info("%s:%s s2mpu08_PMIC_REG_INTSRC_MASK=0x%02x\n",
			MFD_DEV_NAME, __func__, i2c_data);

	ret = request_threaded_irq(s2mpu08->irq, NULL, s2mpu08_irq_thread,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "s2mpu08-irq", s2mpu08);

	if (ret) {
		dev_err(s2mpu08->dev, "Failed to request IRQ %d: %d\n",
			s2mpu08->irq, ret);
		return ret;
	}

	return 0;
}

void s2mpu08_irq_exit(struct s2mpu08_dev *s2mpu08)
{
	if (s2mpu08->irq)
		free_irq(s2mpu08->irq, s2mpu08);
}

//MODULE_AUTHOR("Change Me <CHANGEME@kernel.org>");
//MODULE_DESCRIPTION("Interrupt support for S2MPU08");
//MODULE_LICENSE("GPL");
