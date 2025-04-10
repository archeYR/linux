/* SPDX-License-Identifier: GPL-2.0+
 *
 *  Copyright (C) 2016 Samsung Electronics
 *
 */

#ifndef __LINUX_MFD_S2MPU08_H
#define __LINUX_MFD_S2MPU08_H
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/firmware/samsung/exynos-acpm-protocol.h>

#define SEC_REV_ID(iodev) iodev->pmic_rev


#define S2MPU08_REG_INVALID             (0xff)
#define S2MPU08_IRQSRC_PMIC		(1 << 0)

#define S2MPU08_PMIC_REG_PMICID     0x00
#define S2MPU08_PMIC_REG_INTSRC     0x01
#define S2MPU08_PMIC_REG_INTSRC_MASK     0x02

/* Slave addr = 0xCC */
/* PMIC Registers */
enum S2MPU08_reg {
	S2MPU08_REG_ID,
	S2MPU08_REG_INT1,
	S2MPU08_REG_INT2,
	S2MPU08_REG_INT3,
	S2MPU08_REG_INT1M,
	S2MPU08_REG_INT2M,
	S2MPU08_REG_INT3M,
	S2MPU08_REG_STATUS1,
	S2MPU08_REG_STATUS2,
	S2MPU08_REG_PWRONSRC,
	S2MPU08_REG_OFFSRC,
	S2MPU08_REG_BUCHG,
	S2MPU08_REG_RTCBUF,
	S2MPU08_REG_CTRL1,
	S2MPU08_REG_CTRL2,
	S2MPU08_REG_CTRL3,
	S2MPU08_REG_ETCOTP,
	S2MPU08_REG_UVLONO,
	S2MPU08_REG_UVLOTRIM,
	S2MPU08_REG_CFGPM,
	S2MPU08_REG_TIMECTRL,
	S2MPU08_REG_B1CTRL1,
	S2MPU08_REG_B1CTRL2,
	S2MPU08_REG_B2CTRL1,
	S2MPU08_REG_B2CTRL2,
	S2MPU08_REG_B3CTRL1,
	S2MPU08_REG_B3CTRL2,
	S2MPU08_REG_B4CTRL1,
	S2MPU08_REG_B4CTRL2,
	S2MPU08_REG_B5CTRL1,
	S2MPU08_REG_B5CTRL2,
	S2MPU08_REG_B6CTRL1,
	S2MPU08_REG_B6CTRL2,
	S2MPU08_REG_B7CTRL1,
	S2MPU08_REG_B7CTRL2,
	S2MPU08_REG_B7CTRL3,
	S2MPU08_REG_B8CTRL1,
	S2MPU08_REG_B8CTRL2,
	S2MPU08_REG_B8CTRL3,
	S2MPU08_REG_B9CTRL1,
	S2MPU08_REG_B9CTRL2,
	S2MPU08_REG_BUCKRAMP,
	S2MPU08_REG_LDO8_DVS,
	S2MPU08_REG_LDO9_DVS,
	S2MPU08_REG_LDO10_DVS,
	S2MPU08_REG_L1CTRL,
	S2MPU08_REG_L2CTRL1,
	S2MPU08_REG_L2CTRL2,
	S2MPU08_REG_L3CTRL,
	S2MPU08_REG_L4CTRL,
	S2MPU08_REG_L5CTRL,
	S2MPU08_REG_L6CTRL,
	S2MPU08_REG_L7CTRL,
	S2MPU08_REG_L8CTRL,
	S2MPU08_REG_L9CTRL,
	S2MPU08_REG_L10CTRL,
	S2MPU08_REG_L11CTRL,
	S2MPU08_REG_L12CTRL,
	S2MPU08_REG_L13CTRL,
	S2MPU08_REG_L14CTRL,
	S2MPU08_REG_L33CTRL = 0x4D,
	S2MPU08_REG_L34CTRL,
	S2MPU08_REG_L35CTRL,
	S2MPU08_REG_L36CTRL,
	S2MPU08_REG_L37CTRL,
	S2MPU08_REG_LDO_DSCH1,
	S2MPU08_REG_LDO_DSCH2,
	S2MPU08_REG_LDO_DSCH3,
	S2MPU08_REG_LDO_DSCH4,
	S2MPU08_REG_LDO_DSCH5,
	S2MPU08_REG_LDO_DSCH6,
	S2MPU08_REG_LDO_DSCH7,
	S2MPU08_REG_LDO_DSCH8,
	S2MPU08_REG_LDO_CTRL1,
	S2MPU08_REG_LDO_CTRL2,
	S2MPU08_REG_LDO_CTRL3,
	S2MPU08_REG_TCXO_CTRL,
	S2MPU08_REG_SEQ_CTRL,
	S2MPU08_REG_SEL_CTRL1 = 0x8A,
	S2MPU08_REG_SEL_CTRL2,
};

/* S2MPU08 regulator ids */

enum S2MPU08_regulators {
	S2MPU08_LDO1,
	S2MPU08_LDO2,
	S2MPU08_LDO3,
	S2MPU08_LDO4,
	S2MPU08_LDO5,
	S2MPU08_LDO6,
	S2MPU08_LDO7,
	S2MPU08_LDO8,
	S2MPU08_LDO9,
	S2MPU08_LDO10,
	S2MPU08_LDO11,
	S2MPU08_LDO12,
	S2MPU08_LDO13,
	S2MPU08_LDO14,
	S2MPU08_LDO33,
	S2MPU08_LDO34,
	S2MPU08_LDO35,
	S2MPU08_LDO36,
	S2MPU08_LDO37,
	S2MPU08_BUCK1,
	S2MPU08_BUCK2,
	S2MPU08_BUCK3,
	S2MPU08_BUCK4,
	S2MPU08_BUCK5,
	S2MPU08_BUCK6,
	S2MPU08_BUCK7,
	S2MPU08_BUCK8,
	S2MPU08_REGULATOR_MAX,
};

#define S2MPU08_ENABLE_TIME_LDO		128
#define S2MPU08_ENABLE_TIME_BUCK1	110
#define S2MPU08_ENABLE_TIME_BUCK2	110
#define S2MPU08_ENABLE_TIME_BUCK3	110
#define S2MPU08_ENABLE_TIME_BUCK4	150
#define S2MPU08_ENABLE_TIME_BUCK5	150
#define S2MPU08_ENABLE_TIME_BUCK6	150
#define S2MPU08_ENABLE_TIME_BUCK7	150
#define S2MPU08_ENABLE_TIME_BUCK8	150

#define S2MPU08_ENABLE_SHIFT	0x06
#define S2MPU08_ENABLE_MASK	(0x03 << S2MPU08_ENABLE_SHIFT)

#define S2MPU08_LDO_STEP1	12500
#define S2MPU08_LDO_STEP2	25000
#define S2MPU08_BUCK_STEP1	6250
#define S2MPU08_BUCK_STEP2	12500

#define S2MPU08_LDO_VSEL_MASK	0x3F
#define S2MPU08_BUCK_VSEL_MASK	0xFF

#define S2MPU08_BUCK_RAMP_MASK	3

#define S2MPU08_LDO_N_VOLTAGES	(S2MPU08_LDO_VSEL_MASK + 1)
#define S2MPU08_BUCK_N_VOLTAGES (S2MPU08_BUCK_VSEL_MASK + 1)

#define S2MPU08_BUCK1_RAMP_SHIFT	6
#define S2MPU08_BUCK2_RAMP_SHIFT	4
#define S2MPU08_BUCK4_RAMP_SHIFT	2
#define S2MPU08_BUCK8_RAMP_SHIFT	0

enum s2mpu08_irq_source {
	PMIC_INT1 = 0,
	PMIC_INT2,
	PMIC_INT3,
	S2MPU08_IRQ_GROUP_NR,
};

#define S2MPU08_NUM_IRQ_PMIC_REGS	3

#define S2MPU08_IRQ_PWRONF_MASK		(1 << 0)
#define S2MPU08_IRQ_PWRONR_MASK		(1 << 1)
#define S2MPU08_IRQ_JIGONBF_MASK	(1 << 2)
#define S2MPU08_IRQ_JIGONBR_MASK	(1 << 3)
#define S2MPU08_IRQ_ACOKBF_MASK		(1 << 4)
#define S2MPU08_IRQ_ACOKBR_MASK		(1 << 5)
#define S2MPU08_IRQ_PWRON1S_MASK	(1 << 6)
#define S2MPU08_IRQ_MREVENT_MASK	(1 << 7)

#define S2MPU08_IRQ_RTC60S_MASK		(1 << 0)
#define S2MPU08_IRQ_RTCA1_MASK		(1 << 1)
#define S2MPU08_IRQ_RTCA0_MASK		(1 << 2)
#define S2MPU08_IRQ_SMPL_MASK		(1 << 3)
#define S2MPU08_IRQ_RTC1S_MASK		(1 << 4)
#define S2MPU08_IRQ_WTSR_MASK		(1 << 5)
#define S2MPU08_IRQ_SCLDO2_MASK		(1 << 6)

#define S2MPU08_IRQ_INT120C_MASK	(1 << 0)
#define S2MPU08_IRQ_INT140C_MASK	(1 << 1)
#define S2MPU08_IRQ_TSD_MASK		(1 << 2)
#define S2MPU08_IRQ_SCLDO18_MASK	(1 << 3)
#define S2MPU08_IRQ_SCLDO19_MASK	(1 << 4)
#define S2MPU08_IRQ_SCLDO35_MASK	(1 << 5)
#define S2MPU08_IRQ_MRBF_MASK		(1 << 6)
#define S2MPU08_IRQ_MRBR_MASK		(1 << 7)

enum s2mpu08_irq {
	/* PMIC */
	S2MPU08_IRQ_PWRONF,
	S2MPU08_IRQ_PWRONR,
	S2MPU08_IRQ_JIGONBF,
	S2MPU08_IRQ_JIGONBR,
	S2MPU08_IRQ_ACOKBF,
	S2MPU08_IRQ_ACOKBR,
	S2MPU08_IRQ_PWRON1S,
	S2MPU08_IRQ_MREVENT,

	S2MPU08_IRQ_RTC60S,
	S2MPU08_IRQ_RTCA1,
	S2MPU08_IRQ_RTCA0,
	S2MPU08_IRQ_SMPL,
	S2MPU08_IRQ_RTC1S,
	S2MPU08_IRQ_WTSR,
	S2MPU08_IRQ_SCLDO2,

	S2MPU08_IRQ_INT120C,
	S2MPU08_IRQ_INT140C,
	S2MPU08_IRQ_TSD,
	S2MPU08_IRQ_SCLDO18,
	S2MPU08_IRQ_SCLDO19,
	S2MPU08_IRQ_SCLDO35,
	S2MPU08_IRQ_MRBF,
	S2MPU08_IRQ_MRBR,

	S2MPU08_IRQ_NR,
};

struct s2mpu08_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;

	int num_regulators;
	struct	of_regulator_data *regulators;
	struct	sec_opmode_data		*opmode;
//	struct	mfd_cell *sub_devices;
//	int 	num_subdevs;

//	int	(*cfg_pmic_irq)(void);
	int	device_type;
//	int	ono;
//	int	buck_ramp_delay;

	/* ---- RTC ---- */
//	struct sec_wtsr_smpl *wtsr_smpl;
/*	struct sec_rtc_time *init_time; */
//	struct rtc_time *init_time;
//	int	osc_bias_up;
//	int	cap_sel;
//	int	osc_xin;
//	int	osc_xout;

//	bool	use_i2c_speedy;
};

enum s2mpu08_device_type {
	S2MPU08X,
};

struct s2mpu08_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct i2c_client *pmic;
	struct i2c_client *rtc;
	struct i2c_client *codec;
	struct i2c_client *codeca;
	struct i2c_client *close;
	struct mutex i2c_lock;
	struct acpm_handle *acpm;
	struct acpm_ops *ops;
	unsigned int acpm_channel_id;
	u8 speedy_channel;

	int type;
	int device_type;
	int irq;
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct mutex irqlock;
	int irq_masks_cur[S2MPU08_IRQ_GROUP_NR];
	int irq_masks_cache[S2MPU08_IRQ_GROUP_NR];

	/* pmic REV register */
	unsigned int pmic_rev;	/* pmic Rev */

	struct s2mpu08_platform_data *pdata;
};


extern int s2mpu08_irq_init(struct s2mpu08_dev *s2mpu08);
extern void s2mpu08_irq_exit(struct s2mpu08_dev *s2mpu08);

extern int s2mpu08_read_codec_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
/* S2MPU08 shared i2c API function */
extern int s2mpu08_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2mpu08_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mpu08_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2mpu08_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mpu08_write_word(struct i2c_client *i2c, u8 reg, u16 value);
extern int s2mpu08_read_word(struct i2c_client *i2c, u8 reg);

extern int s2mpu08_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

extern void set_codec_notifier_flag(void);

#endif /* __LINUX_MFD_S2MPU08_H */
