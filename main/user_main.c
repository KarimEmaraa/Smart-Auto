/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "MQTTClient.h"
#include "driver/gpio.h"
#include "ultrasonic.h"
#include "driver/uart.h"


//UART
#define BUF_SIZE (1024)
#define R_M_F	0UL
#define L_M_F	15UL
#define R_M_B	4UL
#define L_M_B	13UL

#define R_E	2UL
#define L_E	5UL

#define PIN_MASK_MOTOR ((1 << R_M_F) | (1 << L_M_F) | (1 << R_M_B) | (1 << L_M_B) | (1 << R_E) | (1 << L_E)) 


//ULTRSONICE SPECIFIC DEFINES
#define MAX_DISTANCE_CM 500 // 5m max
#define TRIGGER_GPIO 14
#define ECHO_GPIO 12

#define ULTRASONIC_PERIOD 500
#define TASK_STACK_SIZE 512

//MQTT AND WIFI SPECSIFIC DEFINES.
#define SSID "osama1965"
#define PASSWORD "ghada1967"
#define PAYLOAD_SIZE 16	//in bytes
#define MQTT_BROKER "broker.mqttdashboard.com"
#define MQTT_PORT  (1883)
#define MQTT_SUB_TOPIC "/karim/sub/"
#define MQTT_SUB_QOS	(0)
#define MQTT_PUB_TOPIC "/karim/pub"
#define MQTT_PUB_QOS	(0)
#define MQTT_CLIENT_ID	"N1"
#define MQTT_KEEP_ALIVE  (30)
#define MQTT_USER_NAME	 ""
#define MQTT_PASSWORD	 ""
#define MQTT_VERSION	(3)
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  2048
#define MQTT_CLIENT_THREAD_PRIO         8

static const char *TAG = "mashro3";

//QueueHandle_t Q1;

ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
};

//MQTTClient client;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
	
    case SYSTEM_EVENT_STA_START:
	/*WIFI MODULE STARTED SUCCESFULLY*/
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
	/*CONNECTED AND GOT AN IP*/
	ESP_LOGI(TAG, "got ip:%s",ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
	
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;	

    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* Disconnected from wifi, there is several reasons check firefox bookmark. */
	ESP_LOGI(TAG, "%s", "DISCONNCTED FROM WIFI");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;

    default:
        break;
    }

    return ESP_OK;
}

static void initialize_wifi(void)
{
 	/* typical must follow steps*/
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void messageArrived(MessageData *data)
{
    ESP_LOGI(TAG, "Message arrived[len:%u]: %.*s", \
		data->message->payloadlen, data->message->payloadlen, (char *)data->message->payload);
}

static void mqtt_client_thread(void *pvParameters)
{
    char payload[PAYLOAD_SIZE] = {0};
    MQTTClient client;
    Network network;
    int rc = 0;
    char clientID[16] = {0};
    uint32_t distance = 0;
	uint8_t data = 0;

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    NetworkInit(&network);

    if (MQTTClientInit(&client, &network, 0, NULL, 0, NULL, 0) == false) {
        ESP_LOGE(TAG, "mqtt init err");
        vTaskDelete(NULL);
    }

    for (;;) {
        ESP_LOGI(TAG, "wait wifi connect...");
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

        if ((rc = NetworkConnect(&network, MQTT_BROKER, MQTT_PORT)) != 0) {
            ESP_LOGE(TAG, "Return code from network connect is %d", rc);
            continue;
        }

        connectData.MQTTVersion = MQTT_VERSION;

        sprintf(clientID, "%s", MQTT_CLIENT_ID);

        connectData.clientID.cstring = clientID;
        connectData.keepAliveInterval = MQTT_KEEP_ALIVE;

        connectData.username.cstring = MQTT_USER_NAME;
        connectData.password.cstring = MQTT_PASSWORD;

        connectData.cleansession = true;

        ESP_LOGI(TAG, "MQTT Connecting");

        if ((rc = MQTTConnect(&client, &connectData)) != 0) {
            ESP_LOGE(TAG, "Return code from MQTT connect is %d", rc);
            network.disconnect(&network);
            continue;
        }

        ESP_LOGI(TAG, "MQTT Connected");

#if defined(MQTT_TASK)

        if ((rc = MQTTStartTask(&client)) != pdPASS) {
            ESP_LOGE(TAG, "Return code from start tasks is %d", rc);
        } else {
            ESP_LOGI(TAG, "Use MQTTStartTask");
        }

#endif

        if ((rc = MQTTSubscribe(&client, MQTT_SUB_TOPIC, MQTT_SUB_QOS, messageArrived)) != 0) {
            ESP_LOGE(TAG, "Return code from MQTT subscribe is %d", rc);
            network.disconnect(&network);
            continue;
        }

        ESP_LOGI(TAG, "MQTT subscribe to topic %s OK", MQTT_SUB_TOPIC);

		MQTTMessage message;

        message.qos = MQTT_PUB_QOS;
        message.retained = 0;
        message.payload = payload;
        sprintf(payload, "%d ", distance);
        message.payloadlen = strlen(payload);
        for (;;) {
			ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
			int len = uart_read_bytes(UART_NUM_0, &data, BUF_SIZE, 20 / portTICK_RATE_MS);
	    	sprintf(payload, "%d", distance);
            if ((rc = MQTTPublish(&client, MQTT_PUB_TOPIC, &message)) != 0) {
                ESP_LOGE(TAG, "Return code from MQTT publish is %d", rc);
            } else {
                ESP_LOGI(TAG, "MQTT published topic %s, len:%u heap:%u", MQTT_PUB_TOPIC, message.payloadlen, esp_get_free_heap_size());
            }

            if (rc != 0) {
                break;
            }
			if(data == 'f')
			{
				ESP_LOGI(TAG, "F is pressed");
				gpio_set_level(R_M_F, 1);
				gpio_set_level(R_M_B, 1);
				gpio_set_level(L_M_F, 1);
				gpio_set_level(L_M_B, 1);
			}
			else if(data == 'b')
			{
				ESP_LOGI(TAG, "b is pressed");
				gpio_set_level(R_M_F, 1);
				gpio_set_level(R_M_B, 0);
				gpio_set_level(L_M_F, 1);
				gpio_set_level(L_M_B, 0);
			}
			else if(data == 'r')
			{
				ESP_LOGI(TAG, "r is pressed");
				gpio_set_level(R_M_F, 0);
				gpio_set_level(R_M_B, 0);
				gpio_set_level(L_M_F, 1);
				gpio_set_level(L_M_B, 1);
			}
			else if(data == 'l')
			{
				ESP_LOGI(TAG, "l is pressed");
				gpio_set_level(R_M_F, 1);
				gpio_set_level(R_M_B, 1);
				gpio_set_level(L_M_F, 0);
				gpio_set_level(L_M_B, 0);
			}
			
		        vTaskDelay(500 / portTICK_RATE_MS);
		    }

		    network.disconnect(&network);
		}

    ESP_LOGW(TAG, "mqtt_client_thread going to be deleted");
    vTaskDelete(NULL);
    return;
}


void motor_init(void)
{
	gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = PIN_MASK_MOTOR;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

void uart_init(void)
{
	   // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 9800,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL);
}

void app_main(void)
{
    // Initialize NVS
	//esp_err_t ret = 0;	
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    initialize_wifi();

	vTaskDelay(10000/portTICK_PERIOD_MS);
	ultrasonic_init(&sensor);
	motor_init();
	uart_init();

    ret = xTaskCreate(&mqtt_client_thread,
                      MQTT_CLIENT_THREAD_NAME,
                      MQTT_CLIENT_THREAD_STACK_WORDS,
                      NULL,
                      3,
                      NULL);

    if (ret != pdPASS)  {
        ESP_LOGE(TAG, "mqtt create client thread %s failed", MQTT_CLIENT_THREAD_NAME);
    }
}
