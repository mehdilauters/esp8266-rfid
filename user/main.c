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
#include "rotary.h"

#include <unistd.h>

#define T uint8_t
#include "fifo.h"

#define SERIAL_BUFFER_SIZE 64

static bool _is_connected;
fifo_t serial;
uint8_t serial_buffer[SERIAL_BUFFER_SIZE];
struct sdk_station_config wifi_config;


void set_red_led_blink(bool _status, unsigned int _frequency) {
  
  if( _frequency == 0 ) {
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false); 
    gpio_write(RED_LED_PIN, _status);
  } else {
    timer_set_frequency(FRC1, _frequency);
    timer_set_interrupts(FRC1, true);
    timer_set_run(FRC1, true);
  }
}


void set_red_led(bool _status) {
  set_red_led_blink(_status, 0);
}

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
        printf("%c\n",c);
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

static void wifi_task(void *pvParameters) {
  uint8_t status = 0;
  uint8_t retries = 30;
    while (1) {
      _is_connected = false;
      
      while ((status != STATION_GOT_IP) && (retries)) {
        status = sdk_wifi_station_get_connect_status();
        printf("%s: status = %d\n\r", __func__, status);
        --retries;
        if (status == STATION_WRONG_PASSWORD) {
          printf("WiFi: wrong password\n\r");
          break;
        } else if (status == STATION_NO_AP_FOUND) {
          printf("WiFi: AP not found\n\r");
          break;
        } else if (status == STATION_CONNECT_FAIL) {
          printf("WiFi: connection failed\r\n");
          break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
      
      if(retries == 0) {
        setup_ap();
        while(true) {
          vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
      }
      
      while ((status = sdk_wifi_station_get_connect_status())
        == STATION_GOT_IP) {
        if ( ! _is_connected ) {
          set_red_led(false);
          printf("WiFi: Connected\n\r");
          _is_connected = true;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        
        _is_connected = false;
      printf("WiFi: disconnected\n\r");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void test_task(void *pvParameters) {
  uint32_t next_ts = 0;
  uint32_t program_ts = 0;
  bool program_enabled = false;
  while(true) {
    bool next = gpio_read(NEXT_PUSH_PIN);
    bool program = gpio_read(PROGRAM_PUSH_PIN);
    
    // program bytton has a pull up
    if(!program) {
      if(program_enabled) {
        if(program_ts == 0) {
          program_ts = xTaskGetTickCount()*portTICK_PERIOD_MS;
        }
        
        if(xTaskGetTickCount()*portTICK_PERIOD_MS - program_ts > 3000) {
          printf("RESET\n"); 
          program_ts = 0;
          flash_erase_all();
          sdk_system_restart();
        }
      }
    } else {
      program_enabled = true;
      program_ts = 0;
    }
    
    if(next) {
      if(next_ts == 0) {
        next_ts = xTaskGetTickCount()*portTICK_PERIOD_MS;
      }
      if(xTaskGetTickCount()*portTICK_PERIOD_MS - next_ts > 3000) {
        printf("Pressed for 3 secs");
        next_ts = 0;
      }
      
    }
    
    long value = get_encoder_value();
    if( value != 0 ) {
      if(value > 0) {
        printf("Up\n");
        push_tag(UP_TAG);
      } else {
        printf("Down\n");
        push_tag(DOWN_TAG);
      }
    }      
    
    bool playpause = gpio_read(PLAYPAUSE_PUSH_PIN);
//     printf("program %d next %d  play_pause %d\n",program, next, playpause);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}


void frc1_interrupt_handler(void)
{
  gpio_toggle(RED_LED_PIN);
}

//Init function 
void user_init() {
  _is_connected = false;
  fifo_init(&serial, serial_buffer, SERIAL_BUFFER_SIZE);
  
  uart_set_baud(0, 9600);
  xTaskCreate(serial_task, (const char *)"serial_task", 512, NULL, 3, NULL);//1024,866
  
  xTaskCreate(test_task, (const char *)"test_task", 512, NULL, 3, NULL);//1024,866
  
  timer_set_interrupts(FRC1, false);
  timer_set_run(FRC1, false);
  _xt_isr_attach(INUM_TIMER_FRC1, frc1_interrupt_handler);
  
  uint32_t id = sdk_system_get_chip_id();
  printf("#%d\n", id);
  rfid_start();
  webserverInit();
  
  gpio_enable(NEXT_PUSH_PIN, GPIO_INPUT);
  gpio_enable(PROGRAM_PUSH_PIN, GPIO_INPUT);
  
  gpio_enable(GREEN_LED_PIN, GPIO_OUTPUT);
  //   gpio_write(GREEN_LED_PIN, 1);
  
  gpio_enable(RED_LED_PIN, GPIO_OUTPUT);
  set_red_led(false);
  
  rotary_init(ROTARY_A, ROTARY_B, ROTARY_C);
  
  
  if( load_network(&wifi_config)) {
    set_red_led_blink(true, 5);
    connect(&wifi_config);
    xTaskCreate(wifi_task, (const char *)"wifi_task", 512, NULL, 3, NULL);//1024,866
  } else {
    set_red_led_blink(true, 1);
    flash_erase_all();
    setup_ap();
  }
}
