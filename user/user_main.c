#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_config.h"
#include "uart.h"

#include "user_interface.h"

ip_addr_t m_server_ip;

void user_rf_pre_init(void) {
}


void data_received( void *arg, char *pdata, unsigned short len )
{
  DEBUG( "%s \n", __FUNCTION__);
}


static void ICACHE_FLASH_ATTR tcp_reconnect(void *arg, sint8 errType)
{
  DEBUG( "%s\n", __FUNCTION__);
  struct espconn *pespconn = (struct espconn *) arg;
  espconn_delete(pespconn);
  //   sync_done(false);
}

void tcp_connect_start( void *arg )
{
  DEBUG( "%s\n", __FUNCTION__);
  uint8_t count;
  struct espconn *conn = arg;
  
  for(count = 0; count < MAX_TRIES; count++) {
    if(CONNECT(conn)) {
      break;
    }
  }
}

void tcp_disconnected( void *arg )
{
  DEBUG( "%s\n", __FUNCTION__);
}

void data_sent(void *arg)
{
  DEBUG( "%s\n", __FUNCTION__);
  struct espconn *conn = arg;
  os_printf("+\n");
}

void tcp_connected( void *arg )
{
  DEBUG( "%s\n", __FUNCTION__);
  struct espconn *conn = arg;
  espconn_regist_recvcb( conn, data_received );
  espconn_regist_sentcb( conn, data_sent);
  
  rfid_start();
  
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
    
    struct espconn *conn = arg;
    
    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp=&m_tcp;
    conn->proto.tcp->local_port = espconn_port();
    conn->proto.tcp->remote_port = PORT;
    os_memcpy( conn->proto.tcp->remote_ip, &ipaddr->addr, 4 );
    
    espconn_regist_connectcb( conn, tcp_connected );
    espconn_regist_disconcb( conn, tcp_disconnected );
    espconn_regist_reconcb(conn, tcp_reconnect);
    
    
    tcp_connect_start(arg);
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
