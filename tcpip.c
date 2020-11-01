// ESP32 component for TCP/IP utilities over WiFi and Ethernet
// Rina Shkrabova, 2018
//
// Unless required by applicable law or agreed to in writing, this
// software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied.

#include "tcpip.h"

#define TAG "TCP/IP:"
static int s_retry_num = 0;
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

EventGroupHandle_t net_event_group;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{

  ///////////////////////
  // ETHERNET EVENTS
  ///////////////////////
  if (event_base == ETH_EVENT) {

    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
      case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
      case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
      case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
      case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
      default:
        break;
    }
  }
  ///////////////////////
  // WIFI EVENTS
  ///////////////////////
  else if (event_base == WIFI_EVENT) {
    switch (event_id) {
      case WIFI_EVENT_STA_START:
        
        esp_wifi_connect();
        break;
      case WIFI_EVENT_STA_DISCONNECTED:
        {
          if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            xEventGroupClearBits(net_event_group, NET_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGW(TAG,"retry to connect to the AP");
          }
          ESP_LOGE(TAG,"connect to the AP failed\n");
        }
        break;
      case WIFI_EVENT_AP_STACONNECTED:
        {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        xEventGroupSetBits(net_event_group, STA_CONNECTED_BIT);
        }
        break;
      case WIFI_EVENT_AP_STADISCONNECTED:
        {
          wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
          ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
              MAC2STR(event->mac), event->aid);
          xEventGroupClearBits(net_event_group, STA_CONNECTED_BIT);
        }
        break;
    }
  }
  ///////////////////////
  // IP EVENTS
  ///////////////////////
  else if (event_base == IP_EVENT) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;

    switch (event_id) {
      case IP_EVENT_ETH_GOT_IP:
        ESP_LOGI(TAG, "Ethernet Got IP Address");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "ETH IP:" IPSTR, IP2STR(&ip_info->ip));
        ESP_LOGI(TAG, "ETH MASK:" IPSTR, IP2STR(&ip_info->netmask));
        ESP_LOGI(TAG, "ETH GW:" IPSTR, IP2STR(&ip_info->gw));
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        xEventGroupSetBits(net_event_group, NET_CONNECTED_BIT);
        break;
      case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "WiFi Got IP Address");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "WIFI IP:" IPSTR, IP2STR(&ip_info->ip));
        ESP_LOGI(TAG, "WIFI MASK:" IPSTR, IP2STR(&ip_info->netmask));
        ESP_LOGI(TAG, "WIFI GW:" IPSTR, IP2STR(&ip_info->gw));
        ESP_LOGI(TAG, "~~~~~~~~~~~");

        s_retry_num = 0;
        xEventGroupSetBits(net_event_group, NET_CONNECTED_BIT);
        break;

    }
    
  }

}

void tcpip_driver_init() {
  ////////////////////////////////////
  //NON-VOLETILE STORAGE
  ////////////////////////////////////
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  //TCP/IP event handling & group (akin to flags and semaphores)
  net_event_group = xEventGroupCreate();

  xEventGroupWaitBits(leds_event_group, SETTINGS_DONE, false, true, portMAX_DELAY);
  ESP_LOGE("driver init","SETTINGS DONE");

  ////////////////////////////////////
  //TCP/IP DRIVER INIT WITH A STATIC IP
  ////////////////////////////////////

  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_ip_info_t ipInfo;


  ////////////////////////////////////
  //EVENT HANDLER (CALLBACK)
  ////////////////////////////////////
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  if (net_mode == MODE_ETH) {
    ////////////////////////////////////
    //ETHERNET CONFIGURATION & INIT
    ////////////////////////////////////
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *netif = esp_netif_new(&cfg);


     // Set default handlers to process TCP/IP stuffs
    ESP_ERROR_CHECK(esp_eth_set_default_handlers(netif));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    if (!dhcp_mode){
      //have stop DHCP 
      esp_netif_dhcpc_stop(netif);

      //set the static IP
      ip4addr_aton(ip, &ipInfo.ip);
      ip4addr_aton(gw, &ipInfo.gw);
      ip4addr_aton(subnet, &ipInfo.netmask);
      ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ipInfo));
    }

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    phy_config.phy_addr = CONFIG_PHY_ADDRESS;
    phy_config.reset_gpio_num = CONFIG_PHY_POWER_PIN;

    mac_config.smi_mdc_gpio_num = CONFIG_PHY_SMI_MDC_PIN;
    mac_config.smi_mdio_gpio_num = CONFIG_PHY_SMI_MDIO_PIN;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);


    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

//#ifdef CONFIG_PHY_CLOCK_GPIO0_IN
//    config.clock_mode  = 0;// ETH_CLOCK_GPIO0_IN;
//#endif

    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));

    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(netif, esp_eth_new_netif_glue(eth_handle)));
    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ipInfo));


  } else if (net_mode == MODE_WIFI) {
    ////////////////////////////////////
    //WIFI CONFIGURATION & INIT
    ////////////////////////////////////
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //esp_netif_t *netif = esp_netif_new(&cfg);


    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    if (!dhcp_mode) {
      //have stop DHCP 
      esp_netif_dhcpc_stop(netif);

      //set the static IP
      ip4addr_aton(ip, &ipInfo.ip);
      ip4addr_aton(gw, &ipInfo.gw);
      ip4addr_aton(subnet, &ipInfo.netmask);
      ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ipInfo));
    }


    //wifi_config_t wifi_config={};

    wifi_config_t wifi_config = {
        .sta = {
            //.ssid = {SSID},
            //.password = {pswd},
            //.ssid = "ww_mini",
            //.password = "glitterpixels",
            ///* Setting a password implies station will connect to all security modes including WEP/WPA.
            // * However these modes are deprecated and not advisable to be used. Incase your Access point
            // * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = {WIFI_AUTH_WPA2_PSK},

            .pmf_cfg = {
                .capable = {true},
                .required = {false},
            },
        },
    };

    strcpy((char *)wifi_config.sta.ssid, (const char *)SSID);
    strcpy((char *)wifi_config.sta.password, (const char *)pswd);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ipInfo));
  }

#ifdef CONFIG_AP
  if (CONFIG_AP) {
    // stop DHCP server
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(netif));
    // assign a static IP to the network interface
    memset(&ipInfo, 0, sizeof(ipInfo));
    inet_pton(AF_INET, CONFIG_AP_IP, &ipInfo.ip);
    inet_pton(AF_INET, CONFIG_AP_GW, &ipInfo.gw);
    inet_pton(AF_INET, CONFIG_AP_NETMASK, &ipInfo.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ipInfo));
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ipInfo.ip), str, INET_ADDRSTRLEN);
    printf("- TCP adapter configured with %s\n", str);

    // start the DHCP server   
    ESP_ERROR_CHECK(esp_netif_dhcps_start(netif));
    printf("- DHCP server started\n");

    // initialize the wifi stack in AccessPoint mode with config in RAM
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    printf("- Wifi adapter configured in SoftAP mode\n");

    // configure the wifi connection and start the interface
    wifi_config_t ap_config = {
      .ap = {
        .ssid = CONFIG_AP_SSID,
        .password = CONFIG_AP_PASSWORD,
        .ssid_len = 0,
        .channel = CONFIG_AP_CHANNEL,
        .authmode = CONFIG_AP_AUTHMODE,
        .ssid_hidden = CONFIG_AP_SSID_HIDDEN,
        .max_connection = CONFIG_AP_MAX_CONNECTIONS,
        .beacon_interval = CONFIG_AP_BEACON_INTERVAL,			
      },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    printf("- Wifi network settings applied\n");


    // start the wifi interface
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("- Wifi adapter starting...\n");
      
  }
#endif



  ////////////////////////////////////
  //TCP\IP INFORMATION PRINT
  ////////////////////////////////////
  ESP_LOGI(TAG, "TCP/IP init finished.");

}

