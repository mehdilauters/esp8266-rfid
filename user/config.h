#ifndef CONFIG_HPP
#define CONFIG_HPP

#define DEFAULT_SSID "esp-rfid"

// should be firmwar_1.bin on the server
#define TFTP_IMAGE_FILENAME "firmware.bin"

#define GREEN_LED_PIN 04
#define RED_LED_PIN 05
#define NEXT_PUSH_PIN 15
#define PLAYPAUSE_PUSH_PIN 2
#define PROGRAM_PUSH_PIN 0

#define ROTARY_A 14
#define ROTARY_B 12
#define ROTARY_C 13

#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__
#endif