#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_config.h"
#include "uart.h"

#include "user_interface.h"

void user_rf_pre_init(void) {
}


bool get_button_pressed() {
  return GPIO_INPUT_GET(0) == 0;
}

void dns_done( const char *name, ip_addr_t *ipaddr, void *arg )
{
  if ( ipaddr == NULL) 
  {
    DEBUG("DNS lookup failed\n");
  }
  else
  {
    DEBUG("found server %d.%d.%d.%d\n",
          *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1), *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
    
    os_memcpy(&(m_server_ip.addr), ipaddr, 4 );
    
    rfid_start();
  }
}

void dns_resolv(const char *_hostname)
{
  err_t res = espconn_gethostbyname( &m_conn, _hostname, &m_server_ip, dns_done );
  if(res != ESPCONN_OK && res != ESPCONN_INPROGRESS) {
    DEBUG("DNS error %d\n",res);
  }
}

void wifi_callback( System_Event_t *evt )
{  
  switch ( evt->event )
  {
    case EVENT_STAMODE_CONNECTED:
    {
      break;
    }
    
    case EVENT_STAMODE_DISCONNECTED:
    {
      break;
    }
    
    case EVENT_STAMODE_GOT_IP:
    {
      DEBUG("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
        IP2STR(&evt->event_info.got_ip.ip),
        IP2STR(&evt->event_info.got_ip.mask),
        IP2STR(&evt->event_info.got_ip.gw));
      DEBUG("\n");
      dns_resolv(SERVER);
      
      break;
    }
    
    default:
    {
      break;
    }
  }
}

bool connect(const char* _essid, const char * _key)
{
  wifi_set_event_handler_cb( wifi_callback );
  
  wifi_set_opmode(STATION_MODE);
  static struct station_config config;
  config.bssid_set = 0;
  os_memcpy( &config.ssid, _essid, strlen(_essid) );
  os_memcpy( &config.password, _key, strlen(_key) );
  wifi_station_set_config_current( &config );
  wifi_station_connect();
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
  uart_init(BIT_RATE_9600, BIT_RATE_9600);
  DEBUG("Hello\n");
  
  connect(ESSID, PWD);
}
