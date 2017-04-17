/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <oc_api.h>

#include <sections.h>
#include <string.h>
#include <zephyr.h>
#include <device.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <misc/byteorder.h>
#include <misc/printk.h>
#include <sensor.h>
#include <gpio.h>

#define GPIO_DRV_NAME	"GPIO_0"

/* IO7(GPIO_20) & IO8(GPIO_16) for Chanable RGB LED */
#define GPIO_OUT_RGB_LED_DATA_PIN	16
#define GPIO_OUT_RGB_LED_CLOCK_PIN	20
#define CLK_PULSE_DELAY		20
#define MAX_RGB_LED_NUM		1
#define CL_RED			0
#define CL_GREEN		1
#define CL_BLUE			2
static int color_state[3] = {0};

/* IO3(GPIO_17) for Grove LED */
#define GPIO_OUT_LED_PIN	17

/* IO2(GPIO_18) for Motion sensor */
#define GPIO_IN_MOTION_PIN	18

QUARK_SE_IPM_DEFINE(sensor_ipm, 0, QUARK_SE_IPM_INBOUND);

static struct k_sem block;
static bool motion = false;
static bool light_state = false;
static double temp = 0;
static double lux = 0;
enum {C, F, K};
static const char *tempUnit[] = {"C", "F", "K"};
static const char *tempRange[] = {"-40,125", "-40, 257", "233.15,398.15"};

static struct device *gpio_dev = NULL;

static void set_device_custom_property(void *data)
{
	(void)data;
	oc_set_custom_device_property(purpose, "Multisensor");
}

static int app_init(void)
{
	int ret = oc_init_platform("Intel", NULL, NULL);
	ret |= oc_add_device("/oic/d", "oic.d.multisensors",
				"SmartHome multisensors", "1.0", "1.0",
				set_device_custom_property, NULL);
	return ret;
}

static void get_temperature(oc_request_t *request,
			    oc_interface_mask_t interface,
			    void *user_data)
{
	(void)user_data;
	PRINT("GET_temperature:\n");
	char *requestUnit = 0;
	double resTemp;
	int index;

	int ret = oc_get_query_value(request, "units", &requestUnit);
	if (ret != -1) {
		if (strcmp(requestUnit, tempUnit[C]) == 0) {
			resTemp = temp;
			index = C;
		}
		else if (strcmp(requestUnit, tempUnit[F]) == 0) {
			resTemp = temp * 1.8 + 32.0;
			index = F;
		}
		else if (strcmp(requestUnit, tempUnit[K]) == 0) {
			resTemp = temp + 273.15;
			index = K;
		}
		else {
			oc_ignore_request(request);
			return;
		}
	}
	else {
		// fallback to default Fahrenheit unit
		resTemp = temp * 1.8 + 32.0;
		index = F;
	}

	oc_rep_start_root_object();
	switch (interface) {
	case OC_IF_BASELINE:
		oc_process_baseline_interface(request->resource);
	case OC_IF_R:
		oc_rep_set_double(root, temperature, resTemp);
		oc_rep_set_text_string(root, units, tempUnit[index]);
		oc_rep_set_text_string(root, range, tempRange[index]);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	oc_send_response(request, OC_STATUS_OK);
	PRINT("Temperature %d\n", temp);
}

static void get_illuminance(oc_request_t *request,
			    oc_interface_mask_t interface,
			    void *user_data)
{
	(void)user_data;
	PRINT("GET_illuminance:\n");

	oc_rep_start_root_object();
	switch (interface) {
	case OC_IF_BASELINE:
		oc_process_baseline_interface(request->resource);
	case OC_IF_R:
		oc_rep_set_double(root, illuminance, lux);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	oc_send_response(request, OC_STATUS_OK);
	PRINT("Illuminance %f\n", lux);
}

static void get_motion(oc_request_t *request,
			    oc_interface_mask_t interface,
			    void *user_data)
{
	(void)user_data;
	uint32_t val;
	PRINT("GET_motion:\n");

	oc_rep_start_root_object();
	switch (interface) {
	case OC_IF_BASELINE:
		oc_process_baseline_interface(request->resource);
	case OC_IF_R:
		if (gpio_dev) {
			gpio_pin_read(gpio_dev, GPIO_IN_MOTION_PIN, &val);
			motion = (val > 0) ? true : false;
		}
		oc_rep_set_boolean(root, value, motion);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	oc_send_response(request, OC_STATUS_OK);
	PRINT("Motion %f\n", motion);
}

static void
get_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
	(void)user_data;
	PRINT("GET_light:\n");
	oc_rep_start_root_object();
	switch (interface) {
	case OC_IF_BASELINE:
		oc_process_baseline_interface(request->resource);
	case OC_IF_RW:
		oc_rep_set_boolean(root, value, light_state);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	oc_send_response(request, OC_STATUS_OK);
	PRINT("Light state %d\n", light_state);
}

static void
post_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
	(void)interface;
	(void)user_data;
	PRINT("POST_light:\n");
	oc_rep_t *rep = request->request_payload;
	while (rep != NULL) {
		PRINT("key: %s ", oc_string(rep->name));
		switch (rep->type) {
		case BOOL:
			light_state = rep->value.boolean;
			if (gpio_dev) {
				gpio_pin_write(gpio_dev, GPIO_OUT_LED_PIN,
						light_state ? 1 : 0);
			}
			PRINT("Light value: %d\n", light_state);
			break;
		default:
			oc_send_response(request, OC_STATUS_BAD_REQUEST);
			return;
			break;
		}
		rep = rep->next;
	}
	oc_rep_start_root_object();
	oc_rep_set_boolean(root, value, light_state);
	oc_rep_end_root_object();

	oc_send_response(request, OC_STATUS_OK);
}

static void
put_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
	(void)interface;
	(void)user_data;
	post_light(request, interface, user_data);
}

void clk()
{
	gpio_pin_write(gpio_dev, GPIO_OUT_RGB_LED_CLOCK_PIN, 0);
	k_busy_wait(CLK_PULSE_DELAY);
	gpio_pin_write(gpio_dev, GPIO_OUT_RGB_LED_CLOCK_PIN, 1);
	k_busy_wait(CLK_PULSE_DELAY);
}

void sendByte(uint8_t b)
{
	// send one bit at a time
	for (uint8_t i = 0; i < 8; i++) {
		if ((b & 0x80) != 0)
			gpio_pin_write(gpio_dev, GPIO_OUT_RGB_LED_DATA_PIN, 1);
		else
			gpio_pin_write(gpio_dev, GPIO_OUT_RGB_LED_DATA_PIN, 0);
		clk();
		b <<= 1;
	}
}

void sendColor(uint8_t red, uint8_t green, uint8_t blue)
{
	// start by sending a byte with the format "1 1 /B7 /B6 /G7 /G6 /R7 /R6"
	uint8_t prefix = 0b11000000;
	if ((blue & 0x80) == 0) prefix |= 0b00100000;
	if ((blue & 0x40) == 0) prefix |= 0b00010000;
	if ((green & 0x80) == 0) prefix |= 0b00001000;
	if ((green & 0x40) == 0) prefix |= 0b00000100;
	if ((red & 0x80) == 0) prefix |= 0b00000010;
	if ((red & 0x40) == 0) prefix |= 0b00000001;

	sendByte(prefix);
	sendByte(blue);
	sendByte(green);
	sendByte(red);
}

void sendColorToAll(int red, int green, int blue)
{
	uint8_t i;
	// send prefix 32 x "0"
	for (i = 0; i < 4; i++) {
		sendByte(0x00);
	}

	for (i = 0; i < MAX_RGB_LED_NUM; i++) {
		sendColor((uint8_t) red, (uint8_t) green, (uint8_t) blue);
	}

	// terminate data frame
	for (i = 0; i < 4; i++) {
		sendByte(0x00);
	}
}

static void
get_color(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
	(void)user_data;
	oc_rep_start_root_object();
	switch (interface) {
	case OC_IF_BASELINE:
		oc_process_baseline_interface(request->resource);
	case OC_IF_RW:
		oc_rep_set_int_array(root, rgbValue, color_state, 3);
		break;
	default:
		break;
	}
	oc_rep_end_root_object();
	oc_send_response(request, OC_STATUS_OK);
}

static void
post_color(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
	(void)interface;
	(void)user_data;

	oc_rep_t *rep = request->request_payload;
	while (rep != NULL) {
		PRINT("key: %s ", oc_string(rep->name));
		switch (rep->type) {
		case INT_ARRAY:
			if (oc_int_array_size(rep->value.array) == 3) {
				int *arr = oc_int_array(rep->value.array);
				color_state[0] = arr[0];
				color_state[1] = arr[1];
				color_state[2] = arr[2];
			}
			if (gpio_dev) {
				sendColorToAll(color_state[0],
						color_state[1],
						color_state[2]);
			}
			break;
		default:
			oc_send_response(request, OC_STATUS_BAD_REQUEST);
			return;
			break;
		}
		rep = rep->next;
	}
	oc_rep_start_root_object();
	oc_rep_set_int_array(root, rgbValue, color_state, 3);
	oc_rep_end_root_object();

	oc_send_response(request, OC_STATUS_OK);
}

static void
put_color(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
	(void)interface;
	(void)user_data;
	post_color(request, interface, user_data);
}
static void register_resources(void)
{
	oc_resource_t *res = oc_new_resource("/a/temperature", 1, 0);
	oc_resource_bind_resource_type(res, "oic.r.temperature");
	oc_resource_bind_resource_interface(res, OC_IF_R);
	oc_resource_set_default_interface(res, OC_IF_R);
	oc_resource_set_discoverable(res, true);
	oc_resource_set_periodic_observable(res, 1);
	oc_resource_set_request_handler(res, OC_GET, get_temperature, NULL);
	oc_add_resource(res);

	oc_resource_t *res2 = oc_new_resource("/a/illuminance", 1, 0);
	oc_resource_bind_resource_type(res2, "oic.r.sensor.illuminance");
	oc_resource_bind_resource_interface(res2, OC_IF_R);
	oc_resource_set_default_interface(res2, OC_IF_R);
	oc_resource_set_discoverable(res2, true);
	oc_resource_set_periodic_observable(res2, 1);
	oc_resource_set_request_handler(res2, OC_GET, get_illuminance, NULL);
	oc_add_resource(res2);

	oc_resource_t *res3 = oc_new_resource("/a/motion", 1, 0);
	oc_resource_bind_resource_type(res3, "oic.r.sensor.motion");
	oc_resource_bind_resource_interface(res3, OC_IF_R);
	oc_resource_set_default_interface(res3, OC_IF_R);
	oc_resource_set_discoverable(res3, true);
	oc_resource_set_periodic_observable(res3, 1);
	oc_resource_set_request_handler(res3, OC_GET, get_motion, NULL);
	oc_add_resource(res3);

	oc_resource_t *res4 = oc_new_resource("/a/led", 1, 0);
	oc_resource_bind_resource_type(res4, "oic.r.switch.binary");
	oc_resource_bind_resource_interface(res4, OC_IF_RW);
	oc_resource_set_default_interface(res4, OC_IF_RW);
	oc_resource_set_discoverable(res4, true);
	oc_resource_set_periodic_observable(res4, 1);
	oc_resource_set_request_handler(res4, OC_GET, get_light, NULL);
	oc_resource_set_request_handler(res4, OC_POST, post_light, NULL);
	oc_resource_set_request_handler(res4, OC_PUT, put_light, NULL);
	oc_add_resource(res4);

	oc_resource_t *res5 = oc_new_resource("/a/rgbled", 1, 0);
	oc_resource_bind_resource_type(res5, "oic.r.colour.rgb");
	oc_resource_bind_resource_interface(res5, OC_IF_RW);
	oc_resource_set_default_interface(res5, OC_IF_RW);
	oc_resource_set_discoverable(res5, true);
	oc_resource_set_periodic_observable(res5, 1);
	oc_resource_set_request_handler(res5, OC_GET, get_color, NULL);
	oc_resource_set_request_handler(res5, OC_POST, post_color, NULL);
	oc_resource_set_request_handler(res5, OC_PUT, put_color, NULL);
	oc_add_resource(res5);

}

static void signal_event_loop(void)
{
	k_sem_give(&block);
}

static void sensor_ipm_callback(void *context, uint32_t id, volatile void *data)
{
	volatile struct sensor_value *val;
	switch (id) {
        case SENSOR_CHAN_TEMP:
		val = data;
		temp = sensor_value_to_double((struct sensor_value *)val);
		break;
        case SENSOR_CHAN_LIGHT:
		val = data;
		lux = sensor_value_to_double((struct sensor_value *)val);
		break;
	default:
		break;
        }
}

void main(void)
{
	struct device *ipm;
	int ret;

	ipm = device_get_binding("sensor_ipm");
	if (ipm == NULL) {
		PRINT("Failed to get IPM device\n");
	}
	else {
		ipm_register_callback(ipm, sensor_ipm_callback, NULL);
		ipm_set_enabled(ipm, 1);
	}

	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		PRINT("Cannot find %s!\n", GPIO_DRV_NAME);
	}
	else {
		/* Setup GPIO output for LED */
		ret = gpio_pin_configure(gpio_dev, GPIO_OUT_LED_PIN, (GPIO_DIR_OUT));
		if (ret) {
			PRINT("Error configuring GPIO_%d!\n", GPIO_OUT_LED_PIN);
		}

		/* Setup GPIO output for RGB LEDs */
		ret = gpio_pin_configure(gpio_dev, GPIO_OUT_RGB_LED_DATA_PIN, (GPIO_DIR_OUT));
		if (ret) {
			PRINT("Error configuring GPIO_%d!\n", GPIO_OUT_RGB_LED_DATA_PIN);
		}
		ret = gpio_pin_configure(gpio_dev, GPIO_OUT_RGB_LED_CLOCK_PIN, (GPIO_DIR_OUT));
		if (ret) {
			PRINT("Error configuring GPIO_%d!\n", GPIO_OUT_RGB_LED_CLOCK_PIN);
		}
		for (uint8_t i = 0; i < MAX_RGB_LED_NUM; i++) {
			sendColorToAll(0, 0, 0);
		}

		/* Setup GPIO input for motion detection */
		ret = gpio_pin_configure(gpio_dev, GPIO_IN_MOTION_PIN, GPIO_DIR_IN);
		if (ret) {
			PRINT("Error configuring GPIO_%d!\n", GPIO_IN_MOTION_PIN);
		}
	}

	static const oc_handler_t handler = {.init = app_init,
			.signal_event_loop = signal_event_loop,
			.register_resources = register_resources };

	k_sem_init(&block, 0, 1);

	if (oc_main_init(&handler) < 0) {
		PRINT("Failed to initialize OC server\n");
		return;
	}

	oc_clock_time_t next_event;

	while (true) {
		next_event = oc_main_poll();
		if (next_event == 0) {
			next_event = K_FOREVER;
		}
		else {
			next_event -= oc_clock_time();
		}
		k_sem_take(&block, next_event);
	}

	oc_main_shutdown();
}
