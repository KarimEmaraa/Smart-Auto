deps_config := \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/app_update/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/aws_iot/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/esp8266/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/freertos/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/libsodium/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/log/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/lwip/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/mdns/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/mqtt/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/newlib/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/pthread/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/ssl/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/tcpip_adapter/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/wpa_supplicant/Kconfig \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/bootloader/Kconfig.projbuild \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/esptool_py/Kconfig.projbuild \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/components/partition_table/Kconfig.projbuild \
	/home/hp/NodeMCU/v3/ESP8266_RTOS_SDK-release-v3.1/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
