#include "tcpip.h"

#define TAG "TCP/IP:"

/* FreeRTOS event group to signal when we are connected to WiFi and ready to start UDP comm*/
EventGroupHandle_t udp_event_group;

/* This replaces the default PHY power on/off function with one that
   also uses a GPIO for power on/off.
   If this GPIO is not connected on your device (and PHY is always powered), you can use the default PHY-specific power
   on/off function rather than overriding with this one.
*/
static void phy_device_power_enable_via_gpio(bool enable)
{
    assert(ETHERNET_PHY_CONFIG.phy_power_enable);

    if (!enable) {
        /* Do the PHY-specific power_enable(false) function before powering down */
        ETHERNET_PHY_CONFIG.phy_power_enable(false);
    }

    gpio_pad_select_gpio(PIN_PHY_POWER);
    gpio_set_direction(PIN_PHY_POWER,GPIO_MODE_OUTPUT);
    if(enable == true) {
        gpio_set_level(PIN_PHY_POWER, 1);
        ESP_LOGD(TAG, "phy_device_power_enable(TRUE)");
    } else {
        gpio_set_level(PIN_PHY_POWER, 0);
        ESP_LOGD(TAG, "power_enable(FALSE)");
    }

    // Allow the power up/down to take effect, min 300us
    vTaskDelay(1);

    if (enable) {
        /* Run the PHY-specific power on operations now the PHY has power */
        ETHERNET_PHY_CONFIG.phy_power_enable(true);
    }
}


static void eth_gpio_config_rmii(void) {
    // RMII data pins are fixed:
    // TXD0 = GPIO19
    // TXD1 = GPIO22
    // TX_EN = GPIO21
    // RXD0 = GPIO25
    // RXD1 = GPIO26
    // CLK == GPIO0
    phy_rmii_configure_data_interface_pins();
    phy_rmii_smi_configure_pins(PIN_SMI_MDC, PIN_SMI_MDIO);
}


static esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch(event->event_id) {
    case SYSTEM_EVENT_ETH_START:
      ESP_LOGI(TAG, "event_handler:ETH_START!");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      ESP_LOGI(TAG, "event_handler:ETHERNET_GOT_IP!");
      //ESP_LOGI(TAG, "got ip:%s\n",
      //    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
      break;
    case SYSTEM_EVENT_STA_START:
      esp_wifi_connect();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      xEventGroupClearBits(udp_event_group, WIFI_CONNECTED_BIT);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGI(TAG, "event_handler:SYSTEM_EVENT_STA_GOT_IP!");
      ESP_LOGI(TAG, "got ip:%s\n",
          ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      ESP_LOGI(TAG, "station:"MACSTR" join,AID=%d\n",
          MAC2STR(event->event_info.sta_connected.mac),
          event->event_info.sta_connected.aid);
      xEventGroupSetBits(udp_event_group, WIFI_CONNECTED_BIT);
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      ESP_LOGI(TAG, "station:"MACSTR"leave,AID=%d\n",
          MAC2STR(event->event_info.sta_disconnected.mac),
          event->event_info.sta_disconnected.aid);
      xEventGroupClearBits(udp_event_group, WIFI_CONNECTED_BIT);
      break;
    default:
      break;
  }
  return ESP_OK;
}



void tcpip_driver_init() {
  ////////////////////////////////////
  //TCP/IP DRIVER INIT WITH A STATIC IP
  ////////////////////////////////////
  tcpip_adapter_init();
  tcpip_adapter_ip_info_t ipInfo;


  ////////////////////////////////////
  //EVENT HANDLER (CALLBACK)
  ////////////////////////////////////
  //TCP/IP event handling & group (akin to flags and semaphores)
  udp_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );


  if (ETH) {
    ////////////////////////////////////
    //ETHERNET CONFIGURATION & INIT
    ////////////////////////////////////
    if (!DHCP){
      //have stop DHCP 
      tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);

      //set the static IP
      ip4addr_aton(DEVICE_IP, &ipInfo.ip);
      ip4addr_aton(DEVICE_GW, &ipInfo.gw);
      ip4addr_aton(DEVICE_NETMASK, &ipInfo.netmask);
      ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo));
    }

    eth_config_t config = ETHERNET_PHY_CONFIG;
    config.phy_addr = PHY1;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.clock_mode = CONFIG_PHY_CLOCK_MODE;
    if (INIT_ETH_POWER) config.phy_power_enable = phy_device_power_enable_via_gpio;

    ESP_ERROR_CHECK(esp_eth_init(&config));
    ESP_ERROR_CHECK(esp_eth_enable());
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo));
    

  } else {
    ////////////////////////////////////
    //WIFI CONFIGURATION & INIT
    ////////////////////////////////////
    if (!DHCP) {
      //have stop DHCP 
      tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);

      //set the static IP
      ip4addr_aton(DEVICE_IP, &ipInfo.ip);
      ip4addr_aton(DEVICE_GW, &ipInfo.gw);
      ip4addr_aton(DEVICE_NETMASK, &ipInfo.netmask);
      ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo));
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
      .sta = {
        .ssid = DEFAULT_SSID,
        .password = DEFAULT_PWD
      },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo));
  }



  ////////////////////////////////////
  //TCP\IP INFORMATION PRINT
  ////////////////////////////////////
  ESP_LOGI(TAG, "TCP/IP initialization finished.");

  if (!ETH)
    ESP_LOGI(TAG, "WiFi \t SSID:%s \t PSWD:%s \n", DEFAULT_SSID, DEFAULT_PWD);

  if (!DHCP) {
    ESP_LOGI(TAG, "TCP|IP \t IP:"IPSTR, IP2STR(&ipInfo.ip));
    ESP_LOGI(TAG, "TCP|IP \t MASK:"IPSTR, IP2STR(&ipInfo.netmask));
    ESP_LOGI(TAG, "TCP|IP \t GW:"IPSTR, IP2STR(&ipInfo.gw));
  }

}

