#ifndef RFID_H 
#define RFID_H

#include "os_type.h"

#define TAG_ID_LENGTH 10
#define TAG_CHECKSUM_LENGTH 1

typedef struct {
  uint8_t id[TAG_ID_LENGTH];
  uint8_t checksum[TAG_CHECKSUM_LENGTH];
  uint8_t current_index;
  bool valid;
} tag_t;

void rfid_start();

#endif
