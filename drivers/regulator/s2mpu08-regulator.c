// SPDX-License-Identifier: GPL-2.0+
//
// Copyright (c) 2012-2014 Samsung Electronics Co., Ltd
//              http://www.samsung.com

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/samsung/s2mpu08.h>
#include <linux/mfd/samsung/core.h>

struct s2mpu08_info {
	struct regulator_dev *rdev[S2MPU08_REGULATOR_MAX];
	unsigned int opmode[S2MPU08_REGULATOR_MAX];
	struct s2mpu08_dev *iodev;
	struct mutex lock;
	struct i2c_client *i2c;
};

static int s2mpu08_get_ramp_delay(int ramp_delay)
{
	unsigned char cnt = 0;

	ramp_delay /= 6000;

	while (true) {
		ramp_delay = ramp_delay >> 1;
		if (ramp_delay == 0)
			break;
		cnt++;
	}
	return cnt;
}

static int s2mpu08_set_ramp_delay(struct regulator_dev *rdev, int ramp_delay)
{
	unsigned int ramp_value, ramp_shift, ramp_reg;
	int rdev_id = rdev_get_id(rdev);

	ramp_value = s2mpu08_get_ramp_delay(ramp_delay);
	if (ramp_value > 4) {
		pr_warn("%s: ramp_delay: %d not supported\n",
			rdev->desc->name, ramp_delay);
	}

	switch (rdev_id) {
	case S2MPU08_BUCK1:
	case S2MPU08_BUCK6:
	case S2MPU08_BUCK7:
		ramp_shift = S2MPU08_BUCK1_RAMP_SHIFT;
		break;
	case S2MPU08_BUCK2:
	case S2MPU08_BUCK3:
		ramp_shift = S2MPU08_BUCK2_RAMP_SHIFT;
		break;
	case S2MPU08_BUCK4:
	case S2MPU08_BUCK5:
		ramp_shift = S2MPU08_BUCK4_RAMP_SHIFT;
		break;
	case S2MPU08_BUCK8:
		ramp_shift = S2MPU08_BUCK8_RAMP_SHIFT;
		break;
	default:
		return 0;
	}
	ramp_reg = S2MPU08_REG_BUCKRAMP;

	return regmap_update_bits(rdev->regmap, ramp_reg,
				  S2MPU08_BUCK_RAMP_MASK << ramp_shift,
				  ramp_value << ramp_shift);
}

static int s2mpu08_regulator_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	unsigned int val;
	int ret;
	struct s2mpu08_info *s2mpu08 = rdev_get_drvdata(rdev);
	int rdev_id = rdev_get_id(rdev);
	val = mode << S2MPU08_ENABLE_SHIFT;
	ret = regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				val,
				rdev->desc->enable_mask);
	if (ret)
		return ret;
	s2mpu08->opmode[rdev_id] = val;
	return 0;
}

static int s2mpu08_regulator_is_enabled(struct regulator_dev *rdev)
{
	int ret;
	struct s2mpu08_info *s2mpu08 = rdev_get_drvdata(rdev);
	int rdev_id = rdev_get_id(rdev);
	unsigned int val, val2, val3;

	/* if pmic_rev is not 0 w/a is unnecessary */
	if (s2mpu08->iodev->pmic_rev == 0x0 && rdev_id == S2MPU08_LDO35)
		goto ldo35_36_workaround;

	if (rdev_id == S2MPU08_BUCK5)
		return 0;

	else {
		ret = regmap_read(rdev->regmap,
				rdev->desc->enable_reg, &val);
		if (ret)
			return ret;
	}

	if (rdev->desc->enable_is_inverted)
		return (val & rdev->desc->enable_mask) == 0;
	else
		return (val & rdev->desc->enable_mask) != 0;

ldo35_36_workaround:

	regmap_read(rdev->regmap, 0xFF, &val);

	if ((val & 0x08) == 0x08)
		return 1;

	else {
		regmap_read(rdev->regmap, S2MPU08_REG_L36CTRL, &val2);
		regmap_read(rdev->regmap, 0x75, &val3);

		if (((val2 & 0xC0) != 0x00) && ((val3 & 0xF0) == 0x00))
			return 1;
		else
			return 0;
	}
}

static int s2mpu08_regulator_enable(struct regulator_dev *rdev)
{
	struct s2mpu08_info *s2mpu08 = rdev_get_drvdata(rdev);
	int rdev_id = rdev_get_id(rdev);
	unsigned int val;

	/* if pmic_rev is not 0 w/a is unnecessary */
	if (s2mpu08->iodev->pmic_rev == 0x0 &&
			(rdev_id == S2MPU08_LDO35 || rdev_id == S2MPU08_LDO36))
				goto ldo35_36_workaround;

	/* disregard BUCK5 enable */
	if (rdev_id == S2MPU08_BUCK5)
		return 0;

	return regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				  s2mpu08->opmode[rdev_id],
				  rdev->desc->enable_mask);

ldo35_36_workaround:

	if (rdev_id == S2MPU08_LDO35) {
		regmap_read(rdev->regmap, S2MPU08_REG_L36CTRL, &val);

		/* if LDO36 is on */
		if ((val & 0xC0) != 0x00) {
			regmap_update_bits(rdev->regmap, 0x75, 0x00, 0xF0);
			regmap_update_bits(rdev->regmap, 0xFF, 0x00, 0x08);
			return 0;
		}

		else {
			regmap_update_bits(rdev->regmap, 0x75, 0xF0, 0xF0);
			regmap_update_bits(rdev->regmap, 0xFF, 0x08, 0x08);
			return 0;
		}
	}

	/* rdev_id == S2MPU08_LDO36 */
	else {
		regmap_read(rdev->regmap, 0xFF, &val);

		/* if LDO35 is on */
		if ((val & 0x08) == 0x08) {
			regmap_update_bits(rdev->regmap, 0x75, 0x00, 0xF0);
			regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				0xC0, rdev->desc->enable_mask);
			/* LDO35 should be off when LDO36 is on */
			regmap_update_bits(rdev->regmap, 0xFF, 0x00, 0x08);
			return 0;
		}

		else {
			regmap_update_bits(rdev->regmap, 0x75, 0xF0, 0xF0);
			regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				0xC0, rdev->desc->enable_mask);
			return 0;
		}
	}
}

static int s2mpu08_regulator_disable(struct regulator_dev *rdev) {
	struct s2mpu08_info *s2mpu08 = rdev_get_drvdata(rdev);
	int val;
	int rdev_id = rdev_get_id(rdev);
	if (rdev_id == S2MPU08_BUCK5)
		return 0;

	if (s2mpu08->iodev->pmic_rev == 0x0 &&
			(rdev_id == S2MPU08_LDO35 || rdev_id == S2MPU08_LDO36))
				goto ldo35_36_workaround;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				  val, rdev->desc->enable_mask);

ldo35_36_workaround:

	if (rdev_id == S2MPU08_LDO35) {
		regmap_read(rdev->regmap, S2MPU08_REG_L36CTRL, &val);

		/* if LDO36 is on */
		if ((val & 0xC0) != 0x00) {
			regmap_update_bits(rdev->regmap, 0x75, 0xF0, 0xF0);
			regmap_update_bits(rdev->regmap, 0xFF, 0x00, 0x08);
			return 0;
		}

		else {
			regmap_update_bits(rdev->regmap, 0x75, 0x00, 0xF0);
			regmap_update_bits(rdev->regmap, 0xFF, 0x00, 0x08);
			return 0;
		}
	}

	/* rdev_id == S2MPU08_LDO36 */
	else {
		regmap_read(rdev->regmap, 0x75, &val);

		/* if LDO35 is on */
		if ((val & 0xF0) == 0x00) {
			regmap_update_bits(rdev->regmap, 0xFF, 0x08, 0x08);
			regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				0, rdev->desc->enable_mask);
			regmap_update_bits(rdev->regmap, 0x75, 0xF0, 0xF0);
			return 0;
		}
		else {
			regmap_update_bits(rdev->regmap, 0xFF, 0x00, 0x08);
			regmap_update_bits(rdev->regmap, rdev->desc->enable_reg,
				0x00, rdev->desc->enable_mask);
			return 0;
		}
	}
}

static int s2mpu08_dummy_operation(struct regulator_dev *rdev) {
	return 0;
}

static unsigned int s2mpu08_of_map_mode(unsigned int val) {
	switch(val) {
	case SEC_OPMODE_SUSPEND:
		return 0x3;
	case SEC_OPMODE_LOWPOWER:
		return 0x2;
	case SEC_OPMODE_ON:
		return 0x1;
	default:
		return 0x1;
	}
}

static const struct regulator_ops s2mpu08_ldo_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2mpu08_regulator_is_enabled,
	.enable			= s2mpu08_regulator_enable,
	.disable		= s2mpu08_regulator_disable,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.set_voltage_time_sel	= regulator_set_voltage_time_sel,
	.set_mode		= s2mpu08_regulator_set_mode,
};

static const struct regulator_ops s2mpu08_buck_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2mpu08_regulator_is_enabled,
	.enable			= s2mpu08_regulator_enable,
	.disable		= s2mpu08_regulator_disable,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.set_voltage_time_sel	= regulator_set_voltage_time_sel,
	.set_mode		= s2mpu08_regulator_set_mode,
	.set_ramp_delay		= s2mpu08_set_ramp_delay,
};
static const struct regulator_ops s2mpu08_buck5_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2mpu08_dummy_operation,
	.enable			= s2mpu08_dummy_operation,
	.disable		= s2mpu08_dummy_operation,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.set_voltage_time_sel	= regulator_set_voltage_time_sel,
	.set_mode		= s2mpu08_regulator_set_mode,
	.set_ramp_delay		= s2mpu08_set_ramp_delay,
};

#define regulator_desc_s2mpu08_ldo1 {			\
	.name		= "LDO1",			\
	.id		= S2MPU08_LDO1,			\
	.ops		= &s2mpu08_ldo_ops,		\
	.type		= REGULATOR_VOLTAGE,		\
	.owner		= THIS_MODULE,			\
	.min_uV		= MIN_700_MV,			\
	.uV_step	= S2MPU08_LDO_STEP1,		\
	.n_voltages	= S2MPU08_LDO_N_VOLTAGES,	\
	.ramp_delay	= RAMP_DELAY_12_MVUS,		\
	.vsel_reg	= S2MPU08_REG_L1CTRL,		\
	.vsel_mask	= S2MPU08_LDO_VSEL_MASK,	\
	.enable_reg	= S2MPU08_REG_L1CTRL,		\
	.enable_mask	= S2MPU08_ENABLE_MASK,		\
	.enable_time	= S2MPU08_ENABLE_TIME_LDO	\
}

#define regulator_desc_s2mpu08_ldo2 {			\
	.name		= "LDO2",			\
	.id		= S2MPU08_LDO2,			\
	.ops		= &s2mpu08_ldo_ops,		\
	.type		= REGULATOR_VOLTAGE,		\
	.owner		= THIS_MODULE,			\
	.min_uV		= MIN_1800_MV,			\
	.uV_step	= S2MPU08_LDO_STEP2,		\
	.n_voltages	= S2MPU08_LDO_N_VOLTAGES,	\
	.ramp_delay	= RAMP_DELAY_12_MVUS,		\
	.vsel_reg	= S2MPU08_REG_L2CTRL1,		\
	.vsel_mask	= S2MPU08_LDO_VSEL_MASK,	\
	.enable_reg	= S2MPU08_REG_L2CTRL1,		\
	.enable_mask	= S2MPU08_ENABLE_MASK,		\
	.enable_time	= S2MPU08_ENABLE_TIME_LDO	\
}

#define regulator_desc_s2mpu08_ldo(num, min, step) {	\
	.name		= "LDO"#num,			\
	.id		= S2MPU08_LDO##num,		\
	.ops		= &s2mpu08_ldo_ops,		\
	.type		= REGULATOR_VOLTAGE,		\
	.owner		= THIS_MODULE,			\
	.min_uV		= min,				\
	.uV_step	= step,				\
	.n_voltages	= S2MPU08_LDO_N_VOLTAGES,	\
	.ramp_delay	= RAMP_DELAY_12_MVUS,		\
	.vsel_reg	= S2MPU08_REG_L3CTRL + num - 3,	\
	.vsel_mask	= S2MPU08_LDO_VSEL_MASK,	\
	.enable_reg	= S2MPU08_REG_L3CTRL + num - 3,	\
	.enable_mask	= S2MPU08_ENABLE_MASK,		\
	.enable_time	= S2MPU08_ENABLE_TIME_LDO	\
}

#define regulator_desc_s2mpu08_buck12346(num) {			\
	.name		= "BUCK"#num,				\
	.id		= S2MPU08_BUCK##num,			\
	.ops		= &s2mpu08_buck_ops,			\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= MIN_500_MV,				\
	.uV_step	= S2MPU08_BUCK_STEP1,			\
	.n_voltages	= S2MPU08_BUCK_N_VOLTAGES,		\
	.ramp_delay	= RAMP_DELAY_12_MVUS,			\
	.vsel_reg	= S2MPU08_REG_B1CTRL2 + (num - 1) * 2,	\
	.vsel_mask	= S2MPU08_BUCK_VSEL_MASK,		\
	.enable_reg	= S2MPU08_REG_B1CTRL1 + (num - 1) * 2,	\
	.enable_mask	= S2MPU08_ENABLE_MASK,			\
	.enable_time	= S2MPU08_ENABLE_TIME_BUCK##num		\
}

#define regulator_desc_s2mpu08_buck5(num) {			\
	.name		= "BUCK"#num,				\
	.id		= S2MPU08_BUCK##num,			\
	.ops		= &s2mpu08_buck5_ops,			\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= MIN_500_MV,				\
	.uV_step	= S2MPU08_BUCK_STEP1,			\
	.n_voltages	= S2MPU08_BUCK_N_VOLTAGES,		\
	.ramp_delay	= RAMP_DELAY_12_MVUS,			\
	.vsel_reg	= S2MPU08_REG_B1CTRL2 + (num - 1) * 2,	\
	.vsel_mask	= S2MPU08_BUCK_VSEL_MASK,		\
	.enable_reg	= S2MPU08_REG_B1CTRL1 + (num - 1) * 2,	\
	.enable_mask	= S2MPU08_ENABLE_MASK,			\
	.enable_time	= S2MPU08_ENABLE_TIME_BUCK##num		\
}

#define regulator_desc_s2mpu08_buck78(num, min) {		\
	.name		= "BUCK"#num,				\
	.id		= S2MPU08_BUCK##num,			\
	.ops		= &s2mpu08_buck_ops,			\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= min,					\
	.uV_step	= S2MPU08_BUCK_STEP2,			\
	.n_voltages	= S2MPU08_BUCK_N_VOLTAGES,		\
	.ramp_delay	= RAMP_DELAY_12_MVUS,			\
	.vsel_reg	= S2MPU08_REG_B7CTRL2 + (num - 1) * 3,	\
	.vsel_mask	= S2MPU08_BUCK_VSEL_MASK,		\
	.enable_reg	= S2MPU08_REG_B7CTRL1 + (num - 1) * 3,	\
	.enable_mask	= S2MPU08_ENABLE_MASK,			\
	.enable_time	= S2MPU08_ENABLE_TIME_BUCK##num		\
}

static const struct regulator_desc s2mpu08_regulators[] = {
	regulator_desc_s2mpu08_ldo1,
	regulator_desc_s2mpu08_ldo2,
	regulator_desc_s2mpu08_ldo(3,  MIN_800_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(4,  MIN_500_MV,  S2MPU08_LDO_STEP1),
	regulator_desc_s2mpu08_ldo(5,  MIN_800_MV,  S2MPU08_LDO_STEP1),
	regulator_desc_s2mpu08_ldo(6,  MIN_800_MV,  S2MPU08_LDO_STEP1),
	regulator_desc_s2mpu08_ldo(7,  MIN_800_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(8,  MIN_500_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(9,  MIN_500_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(10, MIN_500_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(11, MIN_500_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(12, MIN_800_MV,  S2MPU08_LDO_STEP1),
	regulator_desc_s2mpu08_ldo(13, MIN_800_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(14, MIN_1800_MV, S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(33, MIN_800_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(34, MIN_1800_MV, S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(35, MIN_1800_MV, S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(36, MIN_800_MV,  S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_ldo(37, MIN_1800_MV, S2MPU08_LDO_STEP2),
	regulator_desc_s2mpu08_buck12346(1),
	regulator_desc_s2mpu08_buck12346(2),
	regulator_desc_s2mpu08_buck12346(3),
	regulator_desc_s2mpu08_buck12346(4),
	regulator_desc_s2mpu08_buck5(5),
	regulator_desc_s2mpu08_buck12346(6),
	regulator_desc_s2mpu08_buck78(7, MIN_1200_MV),
	regulator_desc_s2mpu08_buck78(8, MIN_1800_MV),
};

static int s2mpu08_pmic_dt_parse(struct platform_device *pdev,
		struct of_regulator_match *rdata, struct s2mpu08_info *s2mpu08,
		unsigned int rdev_num)
{
	struct device_node *reg_np;

	reg_np = of_get_child_by_name(pdev->dev.parent->of_node, "regulators");
	if (!reg_np) {
		dev_err(&pdev->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	of_regulator_match(&pdev->dev, reg_np, rdata, rdev_num);

	of_node_put(reg_np);

	return 0;
}

static int s2mpu08_pmic_probe(struct platform_device *pdev)
{
	struct s2mpu08_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct s2mpu08_platform_data *pdata = iodev->pdata;
	struct of_regulator_match *rdata = NULL;
	struct regulator_config config = { };
	struct s2mpu08_info *s2mpu08;
	unsigned int rdev_num = 0;
	int i, ret = 0;
	const struct regulator_desc *regulators;

	s2mpu08 = devm_kzalloc(&pdev->dev, sizeof(struct s2mpu08_info),
				GFP_KERNEL);
	if (!s2mpu08)
		return -ENOMEM;

	s2mpu08->iodev = iodev;
	s2mpu08->i2c = iodev->pmic;

	mutex_init(&s2mpu08->lock);

	platform_set_drvdata(pdev, s2mpu08);

	rdev_num = ARRAY_SIZE(s2mpu08_regulators);
	//regulators = s2mpu08_regulators;

	rdata = kcalloc(rdev_num, sizeof(*rdata), GFP_KERNEL);
	if (!rdata)
		return -ENOMEM;

	ret = s2mpu08_pmic_dt_parse(pdev, rdata, s2mpu08, rdev_num);
	if (ret)
		goto out;

	for (i = 0; i < rdev_num; i++) {
		config.dev = &pdev->dev;
		config.driver_data = s2mpu08;
		config.init_data = rdata[i].init_data;
		config.of_node = rdata[i].of_node;
		s2mpu08->opmode[i] = S2MPU08_ENABLE_MASK;

		s2mpu08->rdev[i] = devm_regulator_register(&pdev->dev,
						&s2mpu08_regulators[i], &config);
		if (IS_ERR(s2mpu08->rdev[i])) {
			ret = PTR_ERR(s2mpu08->rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n", i);
			goto out;
		}
	}

	regmap_update_bits(config.regmap, S2MPU08_REG_RTCBUF, 0x2, 0x2);

	/* SELMIF settings */
	/* LDO2,4,7,5,6,7,8,33,34,35 - controlled by PWREN_MIF */
	/* LDO1,10,11,12,13,14 - controlled by PWREN */
	regmap_write(config.regmap, S2MPU08_REG_SEL_CTRL1, 0x7E);
	regmap_update_bits(config.regmap, S2MPU08_REG_SEL_CTRL2, 0x00, 0x7F);

	/* initialize LDO35,36 for revision 0*/
	regmap_read(config.regmap, S2MPU08_PMIC_REG_PMICID, &iodev->pmic_rev);
	if(iodev->pmic_rev == 0x00) {
		regmap_update_bits(config.regmap, 0x75, 0xF0, 0xF0);
		regmap_update_bits(config.regmap, 0x7C, 0x08, 0x08);
		regmap_update_bits(config.regmap, 0x8B, 0x00, 0x60);
	}

	/* changed water out THD in codec side */
	//regmap_write(iodev->close, 0x83, 0x74);
	//regmap_write(iodev->close, 0x84, 0x0E);
	/* changed water jack in THD in codec side */
	//regmap_read(iodev->close, 0x7C, &flag_wtp);
	//if ((flag_wtp & BIT(7)) == false) {
	//	regmap_update_bits(iodev->close, 0x7C, BIT(7), BIT(7));
	//	regmap_read(iodev->close, 0x82, &lowr_wtp);
	//	lowr_wtp -= 15;
	//	regmap_write(iodev->close, 0x82, lowr_wtp);
	//}
out:
	kfree(rdata);

	return ret;
}

static void s2mpu08_pmic_remove(struct platform_device *pdev)
{
	struct s2mpu08_info *s2mpu08 = platform_get_drvdata(pdev);
	int i;
	for (i = 0; i < S2MPU08_REGULATOR_MAX; i++)
		regulator_unregister(s2mpu08->rdev[i]);
}

static const struct platform_device_id s2mpu08_pmic_id[] = {
	{ "s2mpu08-regulator", S2MPU08X},
	{ },
};
MODULE_DEVICE_TABLE(platform, s2mpu08_pmic_id);

static struct platform_driver s2mpu08_pmic_driver = {
	.driver = {
		.name = "s2mpu08-regulator",
		.owner = THIS_MODULE,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
	.probe = s2mpu08_pmic_probe,
	.remove = s2mpu08_pmic_remove,
	.id_table = s2mpu08_pmic_id,
};

module_platform_driver(s2mpu08_pmic_driver);

/* Module information */
MODULE_AUTHOR("CHANGE ME <changeme@gmail.com>");
MODULE_DESCRIPTION("Samsung S2MPU08 Regulator Driver");
MODULE_LICENSE("GPL");
