/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include "lcd_screen_i2c.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED_YELLOW_NODE DT_ALIAS(led_yellow)
#define LCD_I2C_NODE DT_ALIAS(led_screen)

const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
const struct i2c_dt_spec dev_lcd_screen = I2C_DT_SPEC_GET(LCD_I2C_NODE);
const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);

int main(void) {
  // Init device
    init_lcd(&dev_lcd_screen);

    // Display a message
    write_lcd(&dev_lcd_screen, HELLO_MSG, LCD_LINE_1);
    //write_lcd_clear(&dev_lcd_screen, ZEPHYR_MSG, LCD_LINE_2);
    gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);

    struct sensor_value temperature, humidity;
    // Display temperature and humidity level
    sensor_sample_fetch(dht11);
    sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);

    double temp = sensor_value_to_double(&temperature);
    double hum = sensor_value_to_double(&humidity);

    printf("Temperature = %d \n",temperature);
    printf("Humidity = %d \n",humidity);
}
