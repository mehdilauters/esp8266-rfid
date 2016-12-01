#include "config.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "flash.h"
#include <string.h>

#include <dhcpserver.h>
#include <lwip/api.h>

#include "captdns.h"
#include "webserver.h"


static bool load_network(struct sdk_station_config* _config) {
  char buffer[128];
  int res = flash_key_value_get("ssid",buffer);
  if(res == 0) {
    printf("n: no ssid\n");
    return false;
  }
  
  strcpy((char*)_config->ssid, buffer);
  
  res = flash_key_value_get("pwd",buffer);
  if(res == 0) {
    printf("n: no pwd\n");
    return false;
  }
  
  strcpy((char*)_config->password, buffer);
  return true;  
}

void connect(struct sdk_station_config* _config) {
  printf("connecting to %s\n",_config->ssid);
  sdk_wifi_set_opmode(STATION_MODE);
  sdk_wifi_station_set_config(_config);
}

void setup_ap() {
  printf("creating %s\n", DEFAULT_SSID);
  
  sdk_wifi_set_opmode(SOFTAP_MODE);
  struct ip_info ap_ip;
  IP4_ADDR(&ap_ip.ip, 172, 16, 0, 1);
  IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
  IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
  sdk_wifi_set_ip_info(1, &ap_ip);
  
  struct sdk_softap_config ap_config = {
    .ssid = DEFAULT_SSID,
    .ssid_hidden = 0,
    .channel = 3,
    .ssid_len = strlen(DEFAULT_SSID),
    .authmode = AUTH_OPEN,
    .password = "",
    .max_connection = 3,
    .beacon_interval = 100,
  };
  sdk_wifi_softap_set_config(&ap_config);
  
  ip_addr_t first_client_ip;
  IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
  dhcpserver_start(&first_client_ip, 4);
  
  webserverInit();
  captdnsInit();
}

//Init function 
void user_init() {
  uart_set_baud(0, 115200);
  struct sdk_station_config config;
  if(load_network(&config)) {
    connect(&config);
  } else {
    setup_ap();
  }
  
}
