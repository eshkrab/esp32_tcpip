// ESP32 component for TCP/IP utilities over WiFi and Ethernet
// Rina Shkrabova, 2018
//
// Unless required by applicable law or agreed to in writing, this
// software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied.

#ifndef __TCPIP_H__
#define __TCPIP_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_eth.h"
#include "esp_err.h"
#include "esp_attr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_event_loop.h"

#include "soc/gpio_reg.h"
#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "soc/emac_ex_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_sig_map.h"

#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include "rom/gpio.h"
#include "rom/ets_sys.h"

#include "driver/gpio.h"
#include "driver/periph_ctrl.h"

#include <lwip/sockets.h>

#include "eth_phy/phy_lan8720.h"

#define ETHERNET_PHY_CONFIG phy_lan8720_default_ethernet_config
#define INIT_ETH_POWER CONFIG_PHY_USE_POWER_PIN
#define PIN_PHY_POWER CONFIG_PHY_POWER_PIN
#define PIN_SMI_MDC   CONFIG_PHY_SMI_MDC_PIN
#define PIN_SMI_MDIO  CONFIG_PHY_SMI_MDIO_PIN

// #define DEVICE_IP "192.168.1.50"
// #define DEVICE_GW "192.168.1.1"
// #define DEVICE_NETMASK "255.255.255.0"
#define DEVICE_IP CONFIG_TCPIP_IP
#define DEVICE_GW CONFIG_TCPIP_GW
#define DEVICE_NETMASK CONFIG_TCPIP_NETMASK

#ifdef CONFIG_ETH
#define ETH 1
#else
#define ETH 0
#endif

#ifdef CONFIG_DHCP
#define DHCP 1
#else
#define DHCP 0
#endif
    
/*AP info and tcp_server info*/
#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD

/* FreeRTOS event group to signal when we are connected to WiFi and ready to start UDP comm*/
extern EventGroupHandle_t udp_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define UDP_CONNCETED_SUCCESS BIT1

extern void tcpip_driver_init();


#ifdef __cplusplus
}
#endif

#endif
