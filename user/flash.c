#include "flash.h"
#include "esp/spi.h"
#include <string.h>
#include "espressif/esp_common.h"

 
 char *find_key(const char *key,char *settings) {
 
  int n;
  for(n=0;n<1024;n+=128) {
    if(strcmp(key,settings+n) == 0) return settings+n;
  }
  return 0;
}
 
static char *find_free(char *settings) {
 
  int n;
  for(n=0;n<1024;n+=128) {
    if(settings[n] == 0) return settings+n;
  }
  return 0;
}
 
void flash_erase_all() {
  char settings[1024];
 
  int n;
  for(n=0;n<1024;n++) settings[n]=0;
 
//   ETS_UART_INTR_DISABLE();
  sdk_spi_flash_erase_sector(0x3C);
  sdk_spi_flash_write(0x3C000,(uint32_t *)settings,1024);
//   ETS_UART_INTR_ENABLE();
}
 
 
int flash_key_value_set(const char *key,const char *value) {
   
  if(strlen(key)   > 64) return 1;
  if(strlen(value) > 64) return 1;
 
  char settings[1024];
  sdk_spi_flash_read(0x3C000, (uint32_t *) settings, 1024);
 
  char *location = find_key(key,settings);
  if(location == NULL) {
    location = find_free(settings);
  }
 
  if(location == NULL) return 0;
 
  strcpy(location,key);
  strcpy(location+64,value);
  
//   ETS_UART_INTR_DISABLE();
  sdk_spi_flash_erase_sector(0x3C);
  sdk_spi_flash_write(0x3C000,(uint32_t*)settings,1024);
//   ETS_UART_INTR_ENABLE();
 
  return 1;
}
 
int flash_key_value_get(char *key,char *value) {
  if(strlen(key)   > 64) return 0;
  if(strlen(value) > 64) return 0;
 
  char settings[1024];
 
  sdk_spi_flash_read(0x3C000, (uint32_t *) settings, 1024);
 
  char *location = find_key(key,settings);
 
  if(location == NULL) {
      value[0]=0;
      return 0;
  }
  strcpy(value,location+64);
  return 1;
}