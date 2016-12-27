#include "rotary.h"
#include "espressif/esp_common.h"
#include "FreeRTOS.h"

#include "esp/gpio.h"
#include "task.h"
#include "queue.h"

int a = -1;
int b = -1;
int c = -1;

static QueueHandle_t tsqueue;
const gpio_inttype_t int_type = GPIO_INTTYPE_EDGE_ANY;


volatile int lastEncoded = 0;
volatile long encoderValue = 0;

long lastencoderValue = 0;

// int lastMSB = 0;
// int lastLSB = 0;

void gpio_intr_handler(uint8_t gpio_num)
{
//   uint32_t now = xTaskGetTickCountFromISR();
//   xQueueSendToBackFromISR(tsqueue, &now, NULL);
  
  
  
  
  
  int MSB = gpio_read(a); //MSB = most significant bit
  int LSB = gpio_read(b); //LSB = least significant bit
  
  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
  
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
  
  lastEncoded = encoded; //store this value for next time
}

void buttonIntTask(void *pvParameters)
{
  printf("Waiting for button press interrupt on gpio %d and %d...\r\n", a, b);
  QueueHandle_t *tsqueue = (QueueHandle_t *)pvParameters;
  gpio_set_interrupt(a, int_type, gpio_intr_handler);
  gpio_set_interrupt(b, int_type, gpio_intr_handler);
  
  while(1) {
    uint32_t button_ts;
    xQueueReceive(*tsqueue, &button_ts, portMAX_DELAY);
    
//     int MSB = digitalRead(encoderPin1); //MSB = most significant bit
//     int LSB = digitalRead(encoderPin2); //LSB = least significant bit
//     
//     int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
//     int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
//     
//     if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
//     if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
//     
//     lastEncoded = encoded; //store this value for next time
    
  }
}

void rotary_init(int _a, int _b, int _c) {
  a = _a;
  b = _b;
  c = _c;
  
  gpio_enable(a, GPIO_INPUT);
  gpio_set_pullup(a, true,true); 
  
  gpio_enable(b, GPIO_INPUT);
  gpio_set_pullup(b, true,true);
  
  if(c != -1) {
    gpio_enable(c, GPIO_OUTPUT);
    gpio_write(c, 0); 
  }
  
  tsqueue = xQueueCreate(2, sizeof(uint32_t));
  xTaskCreate(buttonIntTask, "buttonIntTask", 256, &tsqueue, 2, NULL);
}
