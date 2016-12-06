#include "rfid.h"
#include "main.h"
#include <string.h>
#include "espressif/esp_common.h"
#include <esp/uart.h>

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <netdb.h>

#include "tags.h"

#define TAG_START 0x02
#define TAG_STOP 0x03

tag_t m_tag;

bool m_reading = false;
int button_pressed = 0;
bool button_processed = false;

static int m_port;
static char m_server[256];


void tag_init(tag_t * _tag)
{
  memset(_tag->id, 0, TAG_ID_LENGTH);
  _tag->current_index = 0;
  _tag->valid = false;
}

bool tag_feed(tag_t *_tag, uint8_t _val) {
  if(_tag->current_index < TAG_ID_LENGTH) {
    printf("=%x=",_val);
//     printf("ID\n");
    _tag->id[_tag->current_index] = _val;
  } else if(_tag->current_index > TAG_ID_LENGTH && _tag->current_index <= TAG_ID_LENGTH + TAG_CHECKSUM_LENGTH) {

  } else {
    // cr // lf
  }
  _tag->current_index++;
  return true;
}

bool send_tag() {
  int sockfd;
  struct sockaddr_in serverSockAddr;
  struct hostent *serverHostEnt;
  long hostAddr;
  bzero(&serverSockAddr,sizeof(serverSockAddr));
  hostAddr = inet_addr(m_server);
  if ( (long)hostAddr != (long)-1)
    bcopy(&hostAddr,&serverSockAddr.sin_addr,sizeof(hostAddr));
  else
  {
    serverHostEnt = gethostbyname(m_server);
    if (serverHostEnt == NULL)
    {
      printf("gethost fail\n");
      return false;
    }
    bcopy(serverHostEnt->h_addr,&serverSockAddr.sin_addr,serverHostEnt->h_length);
  }
  serverSockAddr.sin_port = htons(m_port);
  serverSockAddr.sin_family = AF_INET;
  
  if ( (sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    perror("failed to create socket");
    return false;
  }
  
  if(connect( sockfd,
    (struct sockaddr *)&serverSockAddr,
              sizeof(serverSockAddr)) < 0 ) {
    perror("conection failed");
  return false;
  }
  
  write(sockfd, m_tag.id, TAG_ID_LENGTH);
  
  shutdown(sockfd,2);
  close(sockfd);
  return true;
}

void tag_process() {
  printf("Processing tag\n");
  if(is_connected()) {
    send_tag();
  } else {
   printf("not connected\n"); 
  }
}

static void button_triggered() {
  tag_init(&m_tag);
  int i;
  for(i=0; i < strlen(BUTTON_TAG_VALUE); i++)
  {
    tag_feed(&m_tag, BUTTON_TAG_VALUE[i]);
  }
  m_tag.valid = true;
}

static void process_button() {
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

static void feed_task(void *pvParameters)
{
  while(true) {
    
    
    int c = getchar();
    if(c != EOF) {
      if(c == TAG_START) {
        m_reading = true;
        tag_init(&m_tag);
      } else {
        if(c == TAG_STOP) {
          m_reading = false;
          m_tag.valid = true;
        }
        if(m_reading) {
          tag_feed(&m_tag, c);
        }
      }
    }
    
    process_button();
    
    if(m_tag.valid) {
      m_tag.valid = false;
      tag_process();
    }
    
  }
}

void rfid_start() {
  printf("RFID start\n");
  
  memset(m_server, 0, 256);
  m_port = 0;
  load_server(m_server, &m_port);
  
  xTaskCreate(feed_task, (const char *)"rfid_task", 128, NULL, 3, NULL);//1024,866
}