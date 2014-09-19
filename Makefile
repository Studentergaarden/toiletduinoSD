# see
# http://mjo.tc/atelier/2009/02/arduino-cli.html
# http://emacswiki.org/emacs/ArduinoSupport

# Start serial terminal
# sudo apt-get install screen
# screen [serial-port] [baud-rate] , 9600 is standard  eg
# screen /dev/ttyUSB0

# Get available board tags by
# ard-parse-boards --boards

# BOARD_TAG    = atmea328
# ARDUINO_PORT = /dev/ttyUSB*

BOARD_TAG    = uno
ARDUINO_PORT = /dev/ttyACM*

# BOARD_TAG    = mega
# ARDUINO_PORT = /dev/ttyUSB*

# name of the folder in /urs/share/arduino/libraries/
ARDUINO_LIBS = Wire Time SD SPI

# Jeg ved ikke hvilken af følgende der skal ændres:
# standard is 115200
#AVRDUDE_ARD_BAUDRATE = 115200

#MONITOR_BAUDRATE =  #Det er sikkert denne

$(shell etags *.ino *.cpp)


# set variables locally
ARDUINO_DIR=/usr/share/arduino
# ARDUINO_DIR=~/src/arduino-1.5.7
#ARDMK_DIR=/usr/share
ARDMK_DIR=~/Dropbox/Labitat/libraries/Arduino-Makefile
AVR_TOOLS_DIR= /usr

# suppress printing configuration.
ARDUINO_QUIET=1

include $(ARDMK_DIR)/Arduino.mk
