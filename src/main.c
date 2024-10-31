#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include "lcd_screen_i2c.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <inttypes.h>
#include <zephyr/sys/util.h>

#define LED_YELLOW_NODE DT_ALIAS(led_yellow)
#define LCD_I2C_NODE DT_ALIAS(led_screen)
#define BUTTON_0_NODE DT_ALIAS(button_0)
#define BUTTON_1_NODE DT_ALIAS(button_1)
#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
    ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
                         DT_SPEC_AND_COMMA)};

#define SW0_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;
const struct gpio_dt_spec led_yellow_gpio = GPIO_DT_SPEC_GET_OR(LED_YELLOW_NODE, gpios, {0});
const struct i2c_dt_spec dev_lcd_screen = I2C_DT_SPEC_GET(LCD_I2C_NODE);
const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0_NODE, gpios, {0});
const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1_NODE, gpios, {0});

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    printk("Le bouton est pressé : %" PRIu32 "\n", k_cycle_get_32());
}

void thread_humidity_ADC(){
    // ADC :
    int err;
    uint32_t count = 0;
    uint16_t buf;
    struct adc_sequence sequence = {
        .buffer = &buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(buf),
    };

    /* Configure channels individually prior to sampling. */
    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
    {
        if (!adc_is_ready_dt(&adc_channels[i]))
        {
            printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
        }
        err = adc_channel_setup_dt(&adc_channels[i]);
        if (err < 0)
        {
            printk("Could not setup channel #%d (%d)\n", i, err);
        }
    }

    while(1)
    {
        k_sleep(K_SECONDS(10));
        printf("-------------------------------------------- \n");
        printk("L'ADC affiche :", count++);
        for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
        {
            int32_t val_mv;
            (void)adc_sequence_init_dt(&adc_channels[i], &sequence);
            err = adc_read_dt(&adc_channels[i], &sequence);
            if (err < 0)
            {
                printk("Could not read (%d)\n", err);
                continue;
            }
            if (adc_channels[i].channel_cfg.differential)
            {
                val_mv = (int32_t)((int16_t)buf);
            }
            else
            {
                val_mv = (int32_t)buf;
            }
            err = adc_raw_to_millivolts_dt(&adc_channels[i],
                                           &val_mv);
            if (err < 0)
            {
                printk(" (value in mV not available)\n");
            }
            else
            {
                printk(" une humidité à %" PRId32 " mV\n", val_mv);
            }
        }
    }
}

void thread_humidity_and_temperature()
{
    struct sensor_value temperature, humidity;
    while(1){
        // Température et humidité :
        sensor_sample_fetch(dht11);
        sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
        sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY, &humidity);
        int temp = sensor_value_to_double(&temperature);
        int hum = sensor_value_to_double(&humidity);
        printf("La temperature est de %d°C \n", temperature);
        printf("Le taux d'humidité est de %d pourcents \n", humidity);
        printf("-------------------------------------------- \n");
        k_sleep(K_SECONDS(10));
    }
}

int main(void)
{
    // Init device
    init_lcd(&dev_lcd_screen);
    // Display a message
    write_lcd(&dev_lcd_screen, HELLO_MSG, LCD_LINE_1);
    // write_lcd_clear(&dev_lcd_screen, ZEPHYR_MSG, LCD_LINE_2);
    gpio_pin_configure_dt(&led_yellow_gpio, GPIO_OUTPUT_HIGH);

    int ret;
    if (!gpio_is_ready_dt(&button))
    {
        printk("Error: button device %s is not ready\n",
               button.port->name);
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0)
    {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
    }
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    printk("Set up button at %s pin %d\n", button.port->name, button.pin);

    while (1)
    {
        k_sleep(K_SECONDS(5));
    }
}
K_THREAD_DEFINE(thread_id_ADC, 521, thread_humidity_ADC, NULL, NULL, NULL, 9, 0, 0);
K_THREAD_DEFINE(thread_id_HT, 521, thread_humidity_and_temperature, NULL, NULL, NULL, 9, 0, 0);