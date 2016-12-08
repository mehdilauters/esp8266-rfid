PROGRAM=my_program
FW_FILE_1=rom1
FW_FILE_2=rom2
FLASH_SIZE=32
PROGRAM_SRC_DIR=. ./user
EXTRA_COMPONENTS=extras/rboot-ota extras/dhcpserver extras/http-parser extras/stdin_uart_interrupt
include /home/mehdi/Mehdi/perso/esp-open-rtos/common.mk