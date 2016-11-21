#ifndef FLASH_H
#define FLASH_H

#include "ets_sys.h"
#include "osapi.h"

ICACHE_FLASH_ATTR static char *find_key(const char *key,char *settings);
ICACHE_FLASH_ATTR static char *find_free(char *settings);
ICACHE_FLASH_ATTR void flash_erase_all();
ICACHE_FLASH_ATTR int flash_key_value_set(const char *key,const char *value);
ICACHE_FLASH_ATTR int flash_key_value_get(char *key,char *value);

#endif