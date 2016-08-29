#include "rfid.h"
#include "user_interface.h"
#include "osapi.h"

#define TAG_START 0x02
#define TAG_STOP 0x03

#define feed_taskPrio        0
#define feed_taskQueueLen    10

os_event_t    feed_taskQueue[feed_taskQueueLen];
static void feed_task(os_event_t *events);
tag_t m_tag;

bool m_reading = false;


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

void tag_process(tag_t * _tag) {
  SEND( &m_conn, _tag->id, TAG_ID_LENGTH );
}

static void ICACHE_FLASH_ATTR feed_task(os_event_t *events)
{
//   DEBUG("FEEED\n");
  process_uart();
  
  if(m_tag.valid) {
    m_tag.valid = false;
    tag_process(&m_tag);
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