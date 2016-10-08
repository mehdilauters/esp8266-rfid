#ifndef USER_CONFIG_HPP
#define USER_CONFIG_HPP

#define ESSID "MY_SSID"
#define PWD   "MY_PWD"

#define SERVER "my.server.fr"
#define PORT 5555

#define MAX_TRIES 5

#define BUTTON_TAG_VALUE "AF00AF00AF"
#define BUTTON_PRESSED_DELAY 100

#define DEBUG os_printf
// #define DEBUG 

#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"

struct espconn m_conn;
esp_tcp m_tcp;
ip_addr_t m_server_ip;

# define CONNECT(conn) espconn_connect( conn )
# define DISCONNECT(conn) espconn_disconnect( conn )
# define SEND(conn, buffer, len) espconn_sent(conn, buffer, len)

#endif