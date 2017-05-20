# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ARCHITECTURE = avr
BOARD_TAG    = pro
BOARD_SUB	 = 8MHzatmega328
MONITOR_BAUDRATE  = 57600
ARDUINO_LIBS = RF24 RF24Ethernet RF24Mesh RF24Network DHTlib PubSubClient SPI EEPROM

CXXFLAGS_STD = -std=gnu++14
CXXFLAGS += -pedantic -Wall -Wextra -fno-exceptions -Os -DDEBUG_PRINT_ENABLED -DINFO_PRINT_ENABLED -felide-constructors

LDFLAGS += -flto

include /opt/Arduino-Makefile/Arduino.mk
