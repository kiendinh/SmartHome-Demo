/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <sensor.h>
#include <misc/byteorder.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <misc/printk.h>
#include <misc/util.h>

#define SLEEP_TIME	1000

QUARK_SE_IPM_DEFINE(sensor_ipm, 0, QUARK_SE_IPM_OUTBOUND);

void main(void)
{
	struct device *ipm, *grove_temp, *grove_light;
	struct sensor_value temp, lux;

	ipm = device_get_binding("sensor_ipm");
	if (ipm == NULL) {
		printk("Failed to get IPM device\n");
		return;
	}

	grove_temp = device_get_binding("GROVE_TEMPERATURE_SENSOR");
	if (grove_temp == NULL) {
		printk("Temperature sensor device not found.\n");
		return;
	}

	grove_light = device_get_binding("GROVE_LIGHT_SENSOR");
	if (grove_light == NULL) {
		printk("Light sensor device not found.\n");
		return;
	}

	while (1) {
		if (sensor_sample_fetch(grove_temp)) {
			printk("Failed to fetch data from the temperature sensor\n");
		}

		if (sensor_channel_get(grove_temp, SENSOR_CHAN_TEMP, &temp)) {
			printk("Failed to read data from the temperature sensor channel\n");
		}
		else {
			if (ipm_send(ipm, 1, SENSOR_CHAN_TEMP, &temp, sizeof(temp))) {
				printk("Failed to send temperature data over ipm\n");
			}
		}

		if (sensor_sample_fetch(grove_light)) {
			printk("Failed to fetch data from the light sensor\n");
		}

		if (sensor_channel_get(grove_light, SENSOR_CHAN_LIGHT, &lux)) {
			printk("Failed to read data from the light sensor channel\n");
		}
		else {
			if (ipm_send(ipm, 1, SENSOR_CHAN_LIGHT, &lux, sizeof(lux))) {
				printk("Failed to send illuminance data over ipm\n");
			}
		}
		k_sleep(SLEEP_TIME);
	}
}
