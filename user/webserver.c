#include "webserver.h" 


#include "espressif/esp_common.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "http-parser/http_parser.h"
#include "page.h"

#define BACKLOG 10


int create_and_bind() {
  int yes=1;
  struct sockaddr_in my_addr;
  int sockfd = 0;
  int portf = 80;
  
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return -1;
  }
  
  if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
    perror("setsockopt");
    return -1;
  }
  
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(portf);
  my_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(my_addr.sin_zero), '\0', 8);
  
  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    perror("bind");
    return -1;
  }
    
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    return -1;
  }
  return sockfd;
}

char buffer[1024];
void handle(int _sockfd) {
  printf ("handle\n");
  memset(buffer, 0, 1024);
  int res = read(_sockfd, buffer, 3000);
  if (res == -1) {
    perror("recive");
    return;
  }
  printf("received %d\n%s\n",res,buffer);
  
  
//   https://github.com/nodejs/http-parser
/*  
  http_parser_settings settings;
  settings.on_url = my_url_callback;
  settings.on_header_field = my_header_field_callback;
  
  
  http_parser *parser = malloc(sizeof(http_parser));
  http_parser_init(parser, HTTP_REQUEST);
  parser->data = my_socket;*/
  
  
  
  write(_sockfd, page_content, strlen(page_content));
  close(_sockfd);
}

void webserver_task(void *pvParameters) {
  int sockfd = create_and_bind();
  
  while(1) {
    socklen_t sin_size;
    int new_fd = 0;
    sin_size = sizeof(struct sockaddr_in);
    struct sockaddr_in their_addr;
    if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
      perror("accept");
      continue;
    }
    handle(new_fd);
  }
}

void webserverInit() {
  printf("starting webserver\n");
  xTaskCreate(webserver_task, (const char *)"webserver_task", 192, NULL, 3, NULL);//1024,866
}
