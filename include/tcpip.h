// UPDATE november 2019
// code is updated to be complient with esp-idf 4.0, not backwards compatible
//
// ESP32 component for TCP/IP utilities over WiFi and Ethernet
// Rina Shkrabova, 2018
//
// Unless required by applicable law or agreed to in writing, this
// software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied.

#ifndef __WW_TCPIP_H__
#define __WW_TCPIP_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_eth.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"

#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include <lwip/sockets.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"


#include "global.h"

#define ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#define INIT_ETH_POWER CONFIG_PHY_USE_POWER_PIN
#define PIN_PHY_POWER CONFIG_PHY_POWER_PIN
#define PIN_SMI_MDC   CONFIG_PHY_SMI_MDC_PIN
#define PIN_SMI_MDIO  CONFIG_PHY_SMI_MDIO_PIN

#define DEVICE_IP CONFIG_TCPIP_IP
#define DEVICE_GW CONFIG_TCPIP_GW
#define DEVICE_NETMASK CONFIG_TCPIP_NETMASK

#ifdef CONFIG_ETH
#define ETH 1
#else 
#define ETH 0
#endif


#ifdef CONFIG_AP
#define AP 1
#else 
#define AP 0
#endif

#ifdef CONFIG_DHCP
#define DHCP 1
#else
#define DHCP 0
#endif
    
#ifdef CONFIG_MDNS
#define MDNS 1
#else
#define MDNS 0
#endif

#define MDNS_HOSTNAME CONFIG_MDNS_HOSTNAME
#define MDNS_INSTANCE CONFIG_MDNS_INSTANCE

// set AP CONFIG values
#ifdef CONFIG_AP_HIDE_SSID
#define CONFIG_AP_SSID_HIDDEN 1
#else
#define CONFIG_AP_SSID_HIDDEN 0
#endif	
#ifdef CONFIG_WIFI_AUTH_OPEN
#define CONFIG_AP_AUTHMODE WIFI_AUTH_OPEN
#endif
#ifdef CONFIG_WIFI_AUTH_WEP
#define CONFIG_AP_AUTHMODE WIFI_AUTH_WEP
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_PSK
#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_PSK
#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_WPA2_PSK
#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_ENTERPRISE
#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_ENTERPRISE
#endif



/*AP info and tcp_server info*/
#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected to Network and ready for comm*/
extern EventGroupHandle_t net_event_group;
#define NET_CONNECTED_BIT BIT0
#define STA_CONNECTED_BIT BIT3

extern void tcpip_driver_init();


#ifdef __cplusplus
}
#endif

#endif
