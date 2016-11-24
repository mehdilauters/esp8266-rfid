#include "rfid.h"
#include "user_interface.h"
#include "osapi.h"
#include "user_main.h"

#define TAG_START 0x02
#define TAG_STOP 0x03

#define feed_taskPrio        0
#define feed_taskQueueLen    10

os_event_t    feed_taskQueue[feed_taskQueueLen];
static void feed_task(os_event_t *events);
tag_t m_tag;

bool m_reading = false;
int button_pressed = 0;
bool button_processed = false;
void tag_init(tag_t * _tag)
{
  os_memset(_tag->id, 0, TAG_ID_LENGTH);
  _tag->current_index = 0;
  _tag->valid = false;
}

bool tag_feed(tag_t *_tag, uint8_t _val) {
  if(_tag->current_index < TAG_ID_LENGTH) {
    DEBUG("=%x=",_val);
//     DEBUG("ID\n");
    _tag->id[_tag->current_index] = _val;
  } else if(_tag->current_index > TAG_ID_LENGTH && _tag->current_index <= TAG_ID_LENGTH + TAG_CHECKSUM_LENGTH) {

  } else {
    // cr // lf
  }
  _tag->current_index++;
  return true;
}

void ICACHE_FLASH_ATTR process_uart() {
  
  uint8 uart_buf[128]={0};
  uint16 len = 0;
  len = rx_buff_deq(uart_buf, 128 );
  if(len !=0)
  {
    int i;
    for(i=0; i < len; i++) {
      if(uart_buf[i] == TAG_START) {
        m_reading = true;
        tag_init(&m_tag);
      } else {
        if(uart_buf[i] == TAG_STOP) {
          m_reading = false;
          m_tag.valid = true;
        }
        if(m_reading) {
          tag_feed(&m_tag, uart_buf[i]);
        }
      }
//       DEBUG("-%x-",uart_buf[i]);
    }
  }
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
  DISCONNECT(arg);
}

void tcp_connected( void *arg )
{
  DEBUG( "%s\n", __FUNCTION__);
  struct espconn *conn = arg;
  espconn_regist_recvcb( conn, data_received );
  espconn_regist_sentcb( conn, data_sent);
  
  SEND( &m_conn, m_tag.id, TAG_ID_LENGTH );
}

void tag_process() {
  DEBUG("Processing tag\n");
  if(is_connected()) {
    struct espconn *conn = &m_conn;
    
    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp=&m_tcp;
    conn->proto.tcp->local_port = espconn_port();
    conn->proto.tcp->remote_port = PORT;
    os_memcpy( conn->proto.tcp->remote_ip, &(m_server_ip.addr), 4 );
    
    DEBUG("=====\n" IPSTR ,
    IP2STR(&m_server_ip),
    "=====\n");
  //   os_printf("====>%d", ((uint8_t*)m_server_ip.addr)[0]);
    espconn_regist_connectcb( conn, tcp_connected );
    espconn_regist_disconcb( conn, tcp_disconnected );
    espconn_regist_reconcb(conn, tcp_reconnect);
    
    
    tcp_connect_start(conn);
  } else {
   os_printf("not connected\n"); 
  }
}

static void ICACHE_FLASH_ATTR button_triggered() {
  tag_init(&m_tag);
  int i;
  for(i=0; i < strlen(BUTTON_TAG_VALUE); i++)
  {
    tag_feed(&m_tag, BUTTON_TAG_VALUE[i]);
  }
  m_tag.valid = true;
}

static void ICACHE_FLASH_ATTR process_button() {
  if(get_button_pressed() ) {
    if(button_pressed > BUTTON_PRESSED_DELAY && ! button_processed)
    {
      button_processed = true;
      button_triggered();
    }
    button_pressed ++;
  } else {
    button_processed = false;
    button_pressed = 0;
  }
}

static void ICACHE_FLASH_ATTR feed_task(os_event_t *events)
{
//   DEBUG("FEEED\n");
  process_uart();
  process_button();
  
  if(m_tag.valid) {
    m_tag.valid = false;
    tag_process();
  }
  
  os_delay_us(1000);
  system_os_post(feed_taskPrio, 0, 0 );
}

void rfid_start() {
  DEBUG("RFID start\n");
  //Start os task
  system_os_task(feed_task, feed_taskPrio,feed_taskQueue, feed_taskQueueLen);
  system_os_post(feed_taskPrio, 0, 0 );
}