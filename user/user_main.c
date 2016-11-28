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
static bool _is_connected;

char* replace(char* str, char* a, char* b)
{
  int len  = strlen(str);
  int lena = strlen(a), lenb = strlen(b);
  char *p;
  for (p = str; p = strstr(p, a); ++p) {
    if (lena != lenb) // shift end as needed
      memmove(p+lenb, p+lena,
              len - (p - str) + lenb);
      memcpy(p, b, lenb);
    p+=strlen(b);
  }
  return str;
}

static bool ICACHE_FLASH_ATTR save_network(char * _essid, char *_password) {
  os_printf("saving essid=%s, password=%s to flash\n",_essid, _password);
  int res_ssid = flash_key_value_set("ssid",_essid);
  int res_wpa = flash_key_value_set("wpa",_password);
  if(res_ssid == 0 || res_wpa == 0) {
    os_printf("Error saving to flash\n");
    flash_erase_all();
    return false;
  }
  
  return true;
}

static bool ICACHE_FLASH_ATTR load_network(char * _essid, char *_password) {
  int res_ssid = flash_key_value_get("ssid",_essid);
  int res_wpa = flash_key_value_get("wpa",_password);
  if(res_ssid == 1 && res_wpa == 1) {
    return true;
  } else {
    return false;
  }
}

bool ICACHE_FLASH_ATTR load_server(char * _server, int *_port) {
  
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

static bool ICACHE_FLASH_ATTR save_server(char * _server, int _port) {
  os_printf("saving server http://%s:%d to flash\n",_server, _port);
  int res_server = flash_key_value_set("server",_server);
  
  char port[5];
  if(os_sprintf(port, "%d", _port)) {
    int res_port = flash_key_value_set("port",port);
    
    if(res_server == 0 || res_port == 0) {
      os_printf("Error saving to flash\n");
      flash_erase_all();
      return false;
    }
  } else {
    os_printf("bad port %s\n", _port);
  }
  
  return false;
}

static bool ICACHE_FLASH_ATTR http_get_post_value(char *_data, char * _key, char *_output) {
  char start_key[64];
  const char * key_end = "\r\n--";
  
  if(os_sprintf(start_key, "name=\"%s\"\r\n\r\n", _key) > 0) {
    char * value_start = strstr(_data, start_key);
    if(value_start != NULL) {
      value_start += strlen(start_key);
      char * value_end = strstr(value_start, key_end);
      
      int value_len = value_end - value_start;
      os_memcpy(_output, value_start, value_len);
      _output[value_len] = '\0';
      return true;
    }
  }
  _output[0] = '\0';
  return false;
}

static void ICACHE_FLASH_ATTR httpconfig_recv_cb(void *arg, char *data, unsigned short len) {
  struct espconn *conn=(struct espconn *)arg;
  
  
  // really really basic security check
  bool check = false;
  uint32_t id = system_get_chip_id();
  char security[5];
  if( http_get_post_value(data, "security", security) ) {
    uint32_t _id = strtol(security, NULL, 10);
    if(id == _id) {
      check = true;
    }
  }
  
  if(!check) {
    espconn_disconnect(conn);
    return;
  }
  
  char essid[33];
  char password[128];
  
  bool res = http_get_post_value(data, "essid", essid);
  res &= http_get_post_value(data, "password", password);
  
  bool reset = false;
  
  if(res && essid[0] != '\0' && password[0] != '\0') {
    reset = save_network(essid, password);
  }
  
  char server[128];
  char port[5];
  
  res = http_get_post_value(data, "server", server);
  res &= http_get_post_value(data, "port", port);
    
  if(res && server[0] != '\0' && port[0] != '\0') {
    int port_number = strtol(port, NULL, 10);
    save_server(server, port_number);
    reset = true;
  }
  
  espconn_disconnect(conn);
  if(reset) {
    system_restart();
  }
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
  int size = strlen(page_content)+128;
  char buffer[size];
  os_memset(buffer,0,size);
  os_sprintf(buffer, "%s", page_content);
  
  char ssid[32];
  char wpa[32];
  if(load_network(ssid, wpa)) {
    replace(buffer, "ESSID", ssid);
  } else {
    replace(buffer, "ESSID", "NONE");
  }
  
  char server[256];
  int port = 0;
  if(load_server(server, &port)) {
    
    replace(buffer, "SERVER", server);
    char tmp[5];
    os_sprintf(tmp, "%d", port);
    replace(buffer, "PORT", tmp);
  } else {
    replace(buffer, "SERVER", "0.0.0.0");
    replace(buffer, "PORT", "-1");
  }
  
  sint8 d = espconn_sent(conn,buffer,strlen(buffer));
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
    _is_connected = true;
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
      char server[256];
      if(load_server(server, NULL)) {
        dns_resolv(server);
      } else {
        os_printf("cannot read server\n");
      }
      
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
}

void ICACHE_FLASH_ATTR init_done(void) {
    DEBUG("Hello\n");
//     flash_erase_all();
    char ssid[32];
    char wpa[32];
    if(load_network(ssid, wpa)) {
        connect(ssid, wpa);
    } else  {
        os_printf("Config not found, reset\n");
        // key not found
        // first format flash
        flash_erase_all();
        setup_configuration_wifi();
    }
    httpconfig_conn_init();
    rfid_start();
    os_printf("esp8266_%d, Sdk version %s\n", system_get_chip_id(), system_get_sdk_version());
}

bool is_connected() {
  return _is_connected;
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
    _is_connected = false;
    wifi_set_opmode(STATION_MODE);
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
  
  system_init_done_cb(init_done);
}
