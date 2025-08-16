/**
 * Copyright (c) 2011 Richard Ian Taylor.
 * Copyright (c) 2012 Alexey Makhalov (makhaloff@gmail.com)
 * Copyright (c) 2018 Joey Hewitt
 *
 * This file is part of the iDroid Project. (http://www.idroidproject.org).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include "timer-of.h"

#define TIMER0_VAL		0x8
#define TIMER0_CTRL		0x10

#define TIMER0_CTRL_STOP	0
#define TIMER0_CTRL_RUN		1
#define TIMER0_CTRL_UPDATE	2

// TODO use timer_of API instead? can we?
struct s5l_clocksource {
	void __iomem *ticks_hi, *ticks_lo;
	struct clocksource clksrc;
};

u64 s5l_read_clock(struct clocksource *clk) {
	struct s5l_clocksource *s5lclk = container_of(clk, struct s5l_clocksource, clksrc);

	register u32 hi = readl_relaxed(s5lclk->ticks_hi);
	register u32 lo = readl_relaxed(s5lclk->ticks_lo);
	register u32 tst = readl_relaxed(s5lclk->ticks_hi);

	if (hi != tst) {
		hi = tst;
		lo = readl_relaxed(s5lclk->ticks_lo);
	}

	return (((uint64_t)hi) << 32) | lo;
}

static void s5l_timer0_disable(struct timer_of *tof) {
	writel(TIMER0_CTRL_UPDATE, timer_of_base(tof) + TIMER0_CTRL);
	writel(TIMER0_CTRL_STOP,   timer_of_base(tof) + TIMER0_CTRL);
}

static int s5l_set_next_event(unsigned long cycles, struct clock_event_device *evt) {
	struct timer_of *tof = to_timer_of(evt);

	writel(~0, timer_of_base(tof) + TIMER0_VAL);
	writel(TIMER0_CTRL_RUN | TIMER0_CTRL_UPDATE, timer_of_base(tof) + TIMER0_CTRL);
	writel(cycles, timer_of_base(tof) + TIMER0_VAL);
	writel(TIMER0_CTRL_RUN, timer_of_base(tof) + TIMER0_CTRL);

	return 0;
}

static irqreturn_t s5l_timer_interrupt(int irq, void* dev_id) {
	struct clock_event_device *evt = dev_id;
	struct timer_of *tof = to_timer_of(evt);

	s5l_timer0_disable(tof);
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static int __init s5l_add_clocksource(struct device_node *node, struct timer_of *tof, u32 offset) {
	struct s5l_clocksource *cs;

	// XXX I'd use clocksource_mmio_init, but read-read-test-read? for case of rollovers doesn't seem to fit well with it,
	// so I'm copying it in here...

	cs = kzalloc(sizeof(struct s5l_clocksource), GFP_KERNEL);
	if (!cs)
		return -ENOMEM;

	cs->ticks_lo = timer_of_base(tof) + offset;
	cs->ticks_hi = cs->ticks_lo + sizeof(u32);
	cs->clksrc.name = node->name;
	cs->clksrc.rating = 250;
	cs->clksrc.read = s5l_read_clock;
	cs->clksrc.mask = CLOCKSOURCE_MASK(64);
	cs->clksrc.flags = CLOCK_SOURCE_IS_CONTINUOUS;

	return clocksource_register_hz(&cs->clksrc, timer_of_rate(tof));
}

static struct timer_of tof = {
	.flags = TIMER_OF_IRQ | TIMER_OF_CLOCK | TIMER_OF_BASE,
	.clkevt = {
		.rating = 200,
		// TODO try implementing periodic?
		.features = CLOCK_EVT_FEAT_ONESHOT,
		.set_next_event = s5l_set_next_event,
	},
	.of_irq = {
		.handler = s5l_timer_interrupt,
		.flags = IRQF_TIMER | IRQF_IRQPOLL,
	}
};

void s5l8930_timer_reboot(void);

// TODO can we/should we reset the ticks?
static int __init s5l8930_init(struct device_node *node) {
	int ret;

	ret = timer_of_init(node, &tof);
	if (ret)
		return ret;

	s5l_timer0_disable(&tof);

	/* TODO should probably add a sched clock, but I want to understand more about why it's needed first */

	clockevents_config_and_register(&tof.clkevt, timer_of_rate(&tof),
			1, 0xF0000000);

	return s5l_add_clocksource(node, &tof, 0);
}
TIMER_OF_DECLARE(s5l, "apple,s5l8930-timer", s5l8930_init);

// Used by the machine definition to reboot
void s5l8930_timer_reboot(void) {
	void __iomem *ppmgr = timer_of_base(&tof);

	if (ppmgr) {
		writel(0, ppmgr + 0x2c);
		writel(1, ppmgr + 0x24);
		writel(0x80000000, ppmgr + 0x20);
		writel(4, ppmgr + 0x2c);
		writel(0, ppmgr + 0x20);
	}
}
