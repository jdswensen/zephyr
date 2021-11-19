/*
 * Copyright (c) 2022 Jake Swensen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT m41t6x

#include <device.h>
#include <drivers/counter.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <logging/log.h>
#include <sys/printk.h>
#include <sys/timeutil.h>

// LOG_MODULE_REGISTER(M41T6X, CONFIG_COUNTER_LOG_LEVEL);

struct regmap {
	struct {
		uint8_t tenths: 4;
		uint8_t thousandths: 4;
	} __packed sub_sec;

	struct {
		uint8_t stop_bit: 1;
		uint8_t tens: 3;
		uint8_t ones: 4;
	} __packed sec;

	struct {
		uint8_t osc_fail_irq_enable: 1;
		uint8_t tens: 3;
		uint8_t ones: 4;
	} __packed min;

	struct {
		uint8_t reserved: 2;
		uint8_t tens: 2;
		uint8_t ones: 4; // 24 hour format
	} __packed hour;

	struct {
		union {
			struct {
				uint8_t rs3: 1;
				uint8_t rs2: 1;
				uint8_t rs1: 1;
				uint8_t rs0: 1;
			} __packed bits;

			uint8_t value: 4;
		} sqw_freq;

		uint8_t reserved: 1;
		uint8_t day: 3;
	} __packed dow;

	struct {
		uint8_t reserved: 2;
		uint8_t tens: 2;
		uint8_t ones: 4;
	} __packed dom;

	struct {
		uint8_t century: 2;
		uint8_t reserved: 1;
		uint8_t tens: 1;
		uint8_t ones: 4;
	} __packed month;

	struct {
		uint8_t tens: 4;
		uint8_t ones: 4;
	} year;

	uint8_t calibration;
	uint8_t wdt;

	struct {
		uint8_t mon;
		uint8_t date;
		uint8_t hour;
		uint8_t min;
		uint8_t sec;
	} __packed alarm;

	uint8_t flags;
};

#define M41T6X_STOP_BIT(dev) (dev->sec.stop_bit)
#define M41T6X_OFIE(dev) (dev->min.osc_fail_irq_enable)
#define M41T6X_SQW_FREQ(dev) (dev->dow.sqw_freq.value)

struct m41t6x_config {
	/* Common structure first because generic API expects this here. */
	struct counter_config_info generic;
	const char *bus_name;
	//struct gpios isw_gpios;
	uint16_t addr;
};

struct m41t6x_data {
	const struct device *m41t6x;
	const struct device *i2c;
	struct regmap reisters;

	struct k_sem lock;

	struct k_timer sync_timer;

	struct k_work alarm_work;
	struct k_work sync_work;

	// syncpoint
	// new_sp

	time_t rtc_registers;
	time_t rtc_base;
	uint32_t syncclock_base;

	union {
		void *ptr;
		struct sys_notify *notify;
		struct k_poll_signal *signal;
	} sync;

	/* Handlers and state when using the counter alarm API. */
	counter_alarm_callback_t counter_handler[2];
	uint32_t counter_ticks[2];

	/* Handlers and state for M41T6x alarm API */
	//m41t6x_alarm_callback_handler_t alarm_handler[2];
	void *alarm_user_data[2];
	uint8_t alarm_flags[2];

	/* Status of syncronization operations */
	uint8_t sync_state;
	bool sync_signal;
};

int m41t6x_ds3231_get_alarm(const struct device *dev,
			   uint8_t id,
			   const struct counter_alarm_cfg *alarm_cfg)
{
	return 0;
}

static int m41t6x_counter_cancel_alarm(const struct device *dev,
				       uint8_t id)
{
	return 0;
}

int m41t6x_set_alarm(const struct device *dev,
			   uint8_t id,
			   const struct counter_alarm_cfg *alarm_cfg)
{
	return 0;
}

int m41t6x_check_alarms(const struct device *dev)
{
	return 0;
}

static int m41t6x_counter_get_value(const struct device *dev,
				    uint32_t *ticks)
{
	return 0;
}

static int m41t6x_init(const struct device *dev)
{
	printk("m41t6x_init\n");
	return 0;
}

static int m41t6x_counter_start(const struct device *dev)
{
	return -EALREADY;
}

static int m41t6x_counter_stop(const struct device *dev)
{
	return -ENOTSUP;
}

int m41t6x_counter_set_alarm(const struct device *dev,
			     uint8_t id,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	return 0;
}

static uint32_t m41t6x_counter_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static uint32_t m41t6x_counter_get_pending_int(const struct device *dev)
{
	return 0;
}

static int m41t6x_counter_set_top_value(const struct device *dev,
					const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static const struct counter_driver_api counter_m41t6x_api = {
	.start = m41t6x_counter_start,
	.stop = m41t6x_counter_stop,
	.get_value = m41t6x_counter_get_value,
	.set_alarm = m41t6x_counter_set_alarm,
	.cancel_alarm = m41t6x_counter_cancel_alarm,
	.set_top_value = m41t6x_counter_set_top_value,
	.get_pending_int = m41t6x_counter_get_pending_int,
	.get_top_value = m41t6x_counter_get_top_value,
};

static const struct m41t6x_config m41t6x_counter_config = {
	.generic = {
		.max_top_value = UINT32_MAX,
		.freq = 1,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 1,
	},
	.bus_name = DT_INST_BUS_LABEL(0),
// #if DT_INST_NODE_HAS_PROP(0, isw_gpios)
// 	.isw_gpios = {
// 		DT_INST_GPIO_LABEL(0, isw_gpios),
// 		DT_INST_GPIO_PIN(0, isw_gpios),
// 		DT_INST_GPIO_FLAGS(0, isw_gpios),
// 	},
// #endif
	.addr = DT_INST_REG_ADDR(0),
};

static struct m41t6x_data m41t6x_counter_data;

#if CONFIG_COUNTER_INIT_PRIORITY <= CONFIG_I2C_INIT_PRIORITY
#error CONFIG_COUNTER_INIT_PRIORITY must be greater than I2C_INIT_PRIORITY
#endif

DEVICE_DT_INST_DEFINE(0, m41t6x_init, NULL, &m41t6x_counter_data,
		    &m41t6x_counter_config,
		    POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,
		    &counter_m41t6x_api);
