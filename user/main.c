#include "rboot-api.h"



#include "config.h"
#include "espressif/esp_common.h"
#include "esp/gpio.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "flash.h"
#include <string.h>

#include <dhcpserver.h>
#include <lwip/api.h>

#include "captdns.h"
#include "webserver.h"
#include "rfid.h"
#include "ota.h"

#include <unistd.h>

#define T uint8_t
#include "fifo.h"

#define SERIAL_BUFFER_SIZE 64

static bool _is_connected;
fifo_t serial;
uint8_t serial_buffer[SERIAL_BUFFER_SIZE];

bool save_network(char * _essid, char *_password) {
  printf("saving essid=%s, password=%s to flash\n",_essid, _password);
  int res_ssid = flash_key_value_set("ssid",_essid);
  int res_wpa = flash_key_value_set("pwd",_password);
  if(res_ssid == 0 || res_wpa == 0) {
    printf("Error saving to flash\n");
    flash_erase_all();
    return save_network(_essid, _password);
  }
  
  return true;
}

bool load_network(struct sdk_station_config* _config) {
  printf("LOADING\n");
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
  printf("found %s\n", _config->ssid);
  return true;  
}


bool save_server(char * _server, int _port) {
  printf("saving server http://%s:%d to flash\n",_server, _port);
  int res_server = flash_key_value_set("server",_server);
  
  char port[5];
  if(sprintf(port, "%d", _port)) {
    int res_port = flash_key_value_set("port",port);
    
    if(res_server == 0 || res_port == 0) {
      printf("Error saving to flash\n");
      flash_erase_all();
      return false;
    }
  } else {
    printf("bad port %d\n", _port);
  }
  
  return false;
}

bool load_server(char * _server, int *_port) {
  
  int res_server = flash_key_value_get("server",_server);
  
  char buffer[5];
  int res_port = flash_key_value_get("port",buffer);
  
  if(res_server == 1 && res_port == 1) {
    if( _port != NULL ) {
      *_port = strtol(buffer, NULL, 10);
    }
    return true;
  } else {
    return false;
  }
}


bool get_button_pressed() {
  return gpio_read(0) != false;
}

void connect(struct sdk_station_config* _config) {
  printf("connecting to %s\n",_config->ssid);
  sdk_wifi_set_opmode(STATION_MODE);
  sdk_wifi_station_set_config(_config);
  _is_connected = true;
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
  captdnsInit();
}

bool is_connected() {
  return _is_connected;
}


bool get_serial(uint8_t *_c) {
  if(! fifo_isempty(&serial)) {
    *_c = fifo_pop(&serial);
    return true;
  }
  return false;
}

void serial_task(void *pvParameters) {
  char c;
  while(1) {
    if (read(0, (void*)&c, 1)) { // 0 is stdin
      if(! fifo_isfull(&serial)) {
        fifo_push(&serial, c);
        switch(c) {
          case '0':
          {
            printf("reboot to 0\n");
            rboot_config conf = rboot_get_config();
            conf.current_rom = 0;
            rboot_set_config(&conf);
            sdk_system_restart();
            break;
          }
          case '1':
          {
            printf("reboot to 1\n");
            rboot_config conf = rboot_get_config();
            conf.current_rom = 1;
            rboot_set_config(&conf);
            sdk_system_restart();
            break;
          }
          default:
            printf("unknown\n");
        }
      }
    }
  }
}



//Init function 
void user_init() {
  _is_connected = false;
  fifo_init(&serial, serial_buffer, SERIAL_BUFFER_SIZE);
  
  uart_set_baud(0, 115200);
  xTaskCreate(serial_task, (const char *)"serial_task", 512, NULL, 3, NULL);//1024,866
  struct sdk_station_config config;
  if(load_network(&config)) {
    connect(&config);
  } else {
    flash_erase_all();
    setup_ap();
  }
  
  uint32_t id = sdk_system_get_chip_id();
  printf("#%d\n", id);
  
  ota_start();
  webserverInit();
  rfid_start();
}
