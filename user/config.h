#ifndef CONFIG_HPP
#define CONFIG_HPP

#define DEFAULT_SSID "esp-rfid"


/* TFTP client will request this image filenames from this server */
#define TFTP_IMAGE_SERVER "192.168.1.104"
#define TFTP_PORT 69

// should be firmwar_1.bin on the server
#define TFTP_IMAGE_FILENAME_BASE "firmware_"

#define GREEN_LED_PIN 04
#define RED_LED_PIN 05
#define NEXT_PUSH_PIN 15
#define PLAYPAUSE_PUSH_PIN 2
#endif