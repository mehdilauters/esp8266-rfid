#ifndef CONFIG_HPP
#define CONFIG_HPP

#define DEFAULT_SSID "esp-rfid"


/* TFTP client will request this image filenames from this server */
#define TFTP_IMAGE_SERVER "192.168.1.131"
#define TFTP_PORT 69

// should be firmwar_1.bin on the server
#define TFTP_IMAGE_FILENAME_BASE "firmware_"

#endif