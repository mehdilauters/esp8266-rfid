#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_config.h"
#include "uart.h"
#include "flash.h"
#include "user_interface.h"
#include "page.h"

static struct espconn httpconfig_conn;
static esp_tcp httpconfig_tcp_conn;

void remove_trailing_spaces(char *_in, char *_out) {
  do while(isspace(*_in)) _in++; while(*_out++ = *_in++);
}

static void ICACHE_FLASH_ATTR save_network(char * _essid, char *_password) {
  os_printf("saving essid=%s, password=%s to flash",_essid, _password);
//   int res_ssid = flash_key_value_set("ssid",_essid);
//   int res_wpa = flash_key_value_set("wpa",_password);
//   
//   if(res_ssid == 0 || res_wpa == 0) {
//     os_printf("Error saving to flash\n");
//   }
}

static void ICACHE_FLASH_ATTR httpconfig_recv_cb(void *arg, char *data, unsigned short len) {
  struct espconn *conn=(struct espconn *)arg;
  os_printf("==========\n");
  os_printf("%s\n",data);
  os_printf("==========\n");
  //TODO NOT WORKING
  const char * essid_key = "name=\"essid\"";
  const char * password_key = "&password=";
  
  char * essid_start = strstr(data, essid_key);
  const char * password_start = strstr(data, password_key);
  
  char essid[256];
  
  remove_trailing_spaces(essid_start, essid);
  os_printf(">>>>>%s\n",essid);
  if(essid_start != NULL) {
    essid_start += strlen(essid_key);
    int count = password_start - essid_start;
    strncpy(essid, essid_start, count);
  }
  
  if(password_start != NULL) {
    password_start += strlen(password_key);
  }
  
  if(password_start != NULL && essid_start != NULL) {
    save_network(essid, (char*)password_start);
  }
  
  espconn_disconnect(conn);
}

static void ICACHE_FLASH_ATTR httpconfig_recon_cb(void *arg, sint8 err) {
}

static void ICACHE_FLASH_ATTR httpconfig_discon_cb(void *arg) {
}

static void ICACHE_FLASH_ATTR httpconfig_sent_cb(void *arg) {
}

static void ICACHE_FLASH_ATTR httpconfig_connected_cb(void *arg) {
  struct espconn *conn=arg;
  
  espconn_regist_recvcb  (conn, httpconfig_recv_cb);
  espconn_regist_reconcb (conn, httpconfig_recon_cb);
  espconn_regist_disconcb(conn, httpconfig_discon_cb);
  espconn_regist_sentcb  (conn, httpconfig_sent_cb);
  sint8 d = espconn_sent(conn,page_content,strlen(page_content));
}


void ICACHE_FLASH_ATTR httpconfig_conn_init() {
  
  httpconfig_conn.type=ESPCONN_TCP;
  httpconfig_conn.state=ESPCONN_NONE;
  httpconfig_tcp_conn.local_port=80;
  httpconfig_conn.proto.tcp=&httpconfig_tcp_conn;
  
  espconn_regist_connectcb(&httpconfig_conn, httpconfig_connected_cb);
  espconn_accept(&httpconfig_conn);
}


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

void setup_configuration_wifi()
{
    os_printf("Start in config mode\n");
    struct softap_config config;
    wifi_softap_get_config(&config); // Get config first.

    char ssid[32] = "esp-rfid";
    char password[33] = "";
    char macaddr[6];
    
    wifi_softap_get_config(&config);
    wifi_get_macaddr(SOFTAP_IF, macaddr);
    
    os_memset(config.ssid, 0, 32);
    os_memcpy(config.ssid, ssid, 32);
    os_memset(config.password, 0, sizeof(config.password));
    os_memcpy(config.password, password, os_strlen(password));
    config.authmode = AUTH_OPEN;
    config.beacon_interval = 100;
    config.ssid_len = strlen(ssid);
    
    os_printf("Creating %s\n", config.ssid);
    wifi_set_opmode(SOFTAP_MODE);
    wifi_softap_set_config(&config);
    wifi_softap_dhcps_start();
    
    httpconfig_conn_init();
}

void ICACHE_FLASH_ATTR init_done(void) {
    os_printf("esp8266_%d, Sdk version %s\n", system_get_chip_id(), system_get_sdk_version());
    DEBUG("Hello\n");
//     flash_erase_all();
    char ssid[32];
    char wpa[32];
    int res_ssid = flash_key_value_get("ssid",ssid);
    int res_wpa = flash_key_value_get("wpa",wpa);
    if(res_ssid == 1 && res_wpa == 1) {
        connect(ssid, wpa);
    } else  {
        os_printf("Config not found, reset\n");
        // key not found
        // first format flash
        flash_erase_all();
        setup_configuration_wifi();
    }
    
}


//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
    wifi_set_opmode(STATION_MODE);
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
  
  system_init_done_cb(init_done);
}
