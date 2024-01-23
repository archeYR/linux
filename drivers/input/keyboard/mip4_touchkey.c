// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2016 MELFAS Inc.
 *
 * MELFAS MIP4 compatible driver for touchkeys.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>

#define MIP4_R0_INFO				0x01
#define MIP4_R1_INFO_KEY_NUM			0x16
#define MIP4_R1_INFO_IC_NAME			0x71

#define MIP4_R0_EVENT				0x02
#define MIP4_R1_EVENT_SIZE			0x06
#define MIP4_R1_EVENT_PACKET_INFO		0x10
#define MIP4_R1_EVENT_PACKET_DATA		0x11

struct mip4_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct regulator *supply;

	char ic[4];

	u32 *keycodes;
	u8 keycodes_count;

	u8 *events;
	u8 event_size;
};

static int mip4_i2c_read(struct i2c_client *i2c, char *cmd, char *reply, u8 len)
{
	int error;
	int retries = 3;
	struct i2c_msg msg[] = {
		{
			.addr = i2c->addr,
			.flags = 0,
			.buf = cmd,
			.len = 2,
		}, {
			.addr = i2c->addr,
			.flags = I2C_M_RD,
			.buf = reply,
			.len = len,
		},
	};

	while (retries) {
		error = i2c_transfer(i2c->adapter, msg, ARRAY_SIZE(msg));
		if (error == ARRAY_SIZE(msg))
			return 0;

		retries--;
	}

	dev_err(&i2c->dev, "Failed to read %d byte(s)\n", len);
	return -EIO;
}

static irqreturn_t mip4_irq_func(int irq, void *ptr)
{
	struct mip4_data *data = ptr;
	u8 cmd[2];
	u8 events_count;
	u8 keyidx;
	u32 keycode;
	bool status;
	int error;
	int i;

	cmd[0] = MIP4_R0_EVENT;
	cmd[1] = MIP4_R1_EVENT_PACKET_INFO;
	error = mip4_i2c_read(data->client, cmd, &events_count, 1);
	if (error)
		return IRQ_HANDLED;

	/* Check the alert bit. */
	if (events_count & BIT(7))
		return IRQ_HANDLED;

	events_count &= GENMASK(6, 0);

	cmd[0] = MIP4_R0_EVENT;
	cmd[1] = MIP4_R1_EVENT_PACKET_DATA;
	error = mip4_i2c_read(data->client, cmd, data->events, events_count);
	if (error)
		return IRQ_HANDLED;

	for (i = 0; i < events_count; i += data->event_size) {
		keyidx = data->events[i] & GENMASK(3, 0);
		status = data->events[i] & BIT(7);

		if (keyidx) {
			keycode = data->keycodes[keyidx - 1];

			input_event(data->input, EV_MSC, MSC_SCAN, keycode);
			input_report_key(data->input, keycode, status);
		}
	}

	input_sync(data->input);

	return IRQ_HANDLED;
}

static int mip4_read_chip(struct mip4_data *data)
{
	u8 cmd[2];
	int error;

	cmd[0] = MIP4_R0_INFO;
	cmd[1] = MIP4_R1_INFO_IC_NAME;
	error = mip4_i2c_read(data->client, cmd, data->ic, sizeof(data->ic));
	if (error)
		return error;

	cmd[0] = MIP4_R0_INFO;
	cmd[1] = MIP4_R1_INFO_KEY_NUM;
	error = mip4_i2c_read(data->client, cmd, &data->keycodes_count, 1);
	if (error)
		return error;

	cmd[0] = MIP4_R0_EVENT;
	cmd[1] = MIP4_R1_EVENT_SIZE;
	error = mip4_i2c_read(data->client, cmd, &data->event_size, 1);
	if (error)
		return error;

	return 0;
}

static int mip4_probe(struct i2c_client *client)
{
	struct mip4_data *data;
	struct input_dev *input;
	int flags;
	int error;
	int i;

	input = devm_input_allocate_device(&client->dev);
	if (!input)
		return -ENOMEM;

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->input = input;
	data->client = client;
	data->supply = devm_regulator_get(&client->dev, "vdd");

	input_set_drvdata(input, data);
	i2c_set_clientdata(client, data);

	error = regulator_enable(data->supply);
	if (error)
		return error;

	msleep(300);

	error = mip4_read_chip(data);
	if (error)
		return error;

	input->id.bustype = BUS_I2C;
	input->name = devm_kasprintf(&client->dev, GFP_KERNEL, "MELFAS %s",
				     data->ic);
	input->phys = devm_kasprintf(&client->dev, GFP_KERNEL, "%s/input0",
				     dev_name(&client->dev));

	/* Allocate arrays for keycodes and touchkey events. */
	data->keycodes = devm_kcalloc(&client->dev, sizeof(*data->keycodes),
				      data->keycodes_count, GFP_KERNEL);
	data->events = devm_kcalloc(&client->dev, sizeof(*data->events),
				    data->keycodes_count * data->event_size,
				    GFP_KERNEL);

#ifdef CONFIG_OF
	of_property_read_u32_array(client->dev.of_node, "linux,keycodes",
				   data->keycodes, data->keycodes_count);
#endif

	input->keycode = data->keycodes;
	input->keycodesize = sizeof(*data->keycodes);
	input->keycodemax = data->keycodes_count;

	for (i = 0; i < data->keycodes_count; i++)
		input_set_capability(input, EV_KEY, data->keycodes[i]);

	error = input_register_device(input);
	if (error) {
		dev_err(&client->dev, "Couldn't register input device\n");
		return error;
	}

	flags = irq_get_trigger_type(client->irq) | IRQF_ONESHOT;
	error = devm_request_threaded_irq(&client->dev, client->irq, NULL,
					  mip4_irq_func, flags, "mip4_touchkey",
					  data);
	if (error) {
		dev_err(&client->dev, "Couldn't request IRQ %d\n", client->irq);
		return error;
	}

	return 0;
}

static int mip4_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_data *data = i2c_get_clientdata(client);
	int i;

	for (i = 0; i < data->keycodes_count; i++)
		input_report_key(data->input, data->keycodes[i], 0);

	input_sync(data->input);
	disable_irq(client->irq);
	regulator_disable(data->supply);

	return 0;
}

static int mip4_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mip4_data *data = i2c_get_clientdata(client);
	int error;

	error = regulator_enable(data->supply);
	if (error)
		return error;

	msleep(300);
	enable_irq(client->irq);

	return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(mip4_pm_ops, mip4_suspend, mip4_resume);

#ifdef CONFIG_OF
static const struct of_device_id mip4_of_match[] = {
	{ .compatible = "melfas,mip4_touchkey", },
	{ },
};
MODULE_DEVICE_TABLE(of, mip4_of_match);
#endif

static struct i2c_driver mip4_driver = {
	.probe = mip4_probe,
	.driver = {
		.name = "mip4_touchkey",
		.of_match_table = of_match_ptr(mip4_of_match),
		.pm = pm_sleep_ptr(&mip4_pm_ops),
	},
};
module_i2c_driver(mip4_driver);

MODULE_DESCRIPTION("MELFAS MIP4 compatible touchkey driver");
MODULE_AUTHOR("methanal <baclofen@tuta.io>");
MODULE_LICENSE("GPL");
