# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile
#AVR_TOOLS_DIR = /opt/ardu61/bin
#AVR_TOOLS_DIR = /usr
AVR_TOOLS_DIR = /home/adaszek/Workspace/build_avr/gcc/

ARCHITECTURE = avr
BOARD_TAG    = pro
BOARD_SUB	 = 8MHzatmega328
MONITOR_BAUDRATE  = 57600
ARDUINO_LIBS = RF24 RF24Ethernet RF24Mesh RF24Network DHTlib PubSubClient SPI EEPROM

CXXFLAGS_STD = -std=gnu++14
CXXFLAGS += -pedantic -Wall -Wextra -fno-exceptions -Os -flto -DDEBUG_PRINT_ENABLED -DINFO_PRINT_ENABLED -felide-constructors -fno-sized-deallocation

LDFLAGS += -flto

include /opt/Arduino-Makefile/Arduino.mk
