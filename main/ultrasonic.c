/**
 * @file ultrasonic.c
 *
 * ESP-IDF driver for ultrasonic range meters, e.g. HC-SR04, HY-SRF05 and so on
 *
 * Ported from esp-open-rtos
 * Copyright (C) 2016, 2018 Ruslan V. Uss <unclerus@gmail.com>
 * BSD Licensed as described in the file LICENSE
 */
#include "ultrasonic.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>

#define TRIGGER_LOW_DELAY 10
#define TRIGGER_HIGH_DELAY 15
#define PING_TIMEOUT 6000
#define ROUNDTRIP 58

//static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

static inline uint32_t get_time_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec;
}

#define timeout_expired(start, len) ((uint32_t)(get_time_us() - (start)) >= (len))

#define RETURN_CRTCAL(MUX, RES) do { vPortExitCritical(); return RES; } while(0)
static int time2 = 0;
static void gpio_isr_handler(void *arg)
{
    time2 = get_time_us();
    //xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void ultrasonic_init(const ultrasonic_sensor_t *dev)
{
	gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = (1 << dev->trigger_pin);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

	io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = (1 << dev->echo_pin);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    //io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    
	//gpio_set_direction(dev->trigger_pin, GPIO_MODE_OUTPUT);
    //gpio_set_direction(dev->echo_pin, GPIO_MODE_INPUT);

	gpio_set_intr_type(dev->echo_pin, GPIO_INTR_ANYEDGE);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(dev->echo_pin, gpio_isr_handler, (void *) dev->echo_pin);

    gpio_set_level(dev->trigger_pin, 0);
}

esp_err_t ultrasonic_measure_cm(const ultrasonic_sensor_t *dev, uint32_t max_distance, uint32_t *distance)
{
    if (!distance)
        return ESP_ERR_INVALID_ARG;

    vPortEnterCritical();

    // Ping: Low for 2..4 us, then high 10 us
    gpio_set_level(dev->trigger_pin, 0);
    vTaskDelay(TRIGGER_LOW_DELAY / portTICK_PERIOD_MS);
    gpio_set_level(dev->trigger_pin, 1);
    vTaskDelay(TRIGGER_HIGH_DELAY / portTICK_PERIOD_MS);
    gpio_set_level(dev->trigger_pin, 0);



    // Previous ping isn't ended
    //if (gpio_get_level(dev->echo_pin))
      //  RETURN_CRTCAL(mux, ESP_ERR_ULTRASONIC_PING);

    // Wait for echo
    //uint32_t start = get_time_us();
    /*while (!gpio_get_level(dev->echo_pin))
    {
        if (timeout_expired(start, PING_TIMEOUT))
            RETURN_CRTCAL(mux, ESP_ERR_ULTRASONIC_PING_TIMEOUT);
    }*/

    // got echo, measuring
    uint32_t echo_start = get_time_us();
    uint32_t time = echo_start;
    uint32_t meas_timeout = echo_start + max_distance * ROUNDTRIP;
    /*while (gpio_get_level(dev->echo_pin))
    {
        time = get_time_us();
        //if (timeout_expired(echo_start, meas_timeout))
          //  RETURN_CRTCAL(mux, ESP_ERR_ULTRASONIC_ECHO_TIMEOUT);
    }*/
    vPortExitCritical();
	uint32_t duration = time2 - echo_start;
	//(duration * 0.034) / 2
    *distance = (duration * 0.034) / 2;

    return ESP_OK;
}
