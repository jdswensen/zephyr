/*
 * Copyright (c) 2021 Jake Swensen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(MAIN, 3);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif

void main(void)
{
	const struct device *led;
	const struct device *rtc;
	const char *const dev_id = DT_LABEL(DT_INST(0, m41t6x));

	bool led_is_on = true;
	int ret;

	led = device_get_binding(LED0);
	if (!led) {
		return;
	}

	rtc = device_get_binding(dev_id);
	if (!rtc) {
		printk("No device %s available\n", dev_id);
		return;
	}

	ret = gpio_pin_configure(led, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return;
	}

	printk("Starting RTC sample.\n");
	LOG_INF("Starting RTC sample.");
	LOG_INF("Test info");

	while (1) {
		gpio_pin_set(led, PIN, (int)led_is_on);
		led_is_on = !led_is_on;
		k_msleep(SLEEP_TIME_MS);
	}
}
