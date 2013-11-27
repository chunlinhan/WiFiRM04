# WiFiRM04 Library
WiFiRM04 library is an Arduino WiFi library for Hi-Link HLK-RM04 module. (http://www.hlktech.com/en/productshow.asp?id=142)
This library is compatible with Arduino WiFi library and all examples of Arduino WiFi library could be compiled with this
library with just a little modification.

Currently, this library doesn't support UDP feature and assign a static IP in client mode.

All features are tesed with firmware V1.78 and I have no idea if they could work with older firmware.

## How to Install

1. Download the WiFiRM04 library source.
1. In the Arduino IDE, go to the Sketch -> Import Library... -> Add Library...
1. Find the directory that contains the WiFiRM04 library source, and open it
1. Check if you could see "WiFiRM04" under Sketch -> Import Library...

## How to Use

Since this library is compatible with Arduino WiFi library, you could check Arduino web site to learn how to use it.
(http://arduino.cc/en/Reference/WiFi)

## Configuration

First and most important, *MUST* modify Arduino's serial driver to increase Rx buffer size or add hardware flow control
support, because HLK-RM04 will try to output all data at once. If Rx buffer of your serial port is too small(or no hardware
flow control support), you will lose those data.

By default, this library will use two UARTs to communicate with HLK-RM04 module. (firmware V1.78 can support two UARTs)
If you don't want to use second UART of HLK-RM04, just modify utility/wl_definitions.h and change the definition
of MAX_SOCK_NUM to 1. (but the second serial port will still be initialized in utilit/at_drv.cpp)

The serial ports I used are Serial1 and Serial2, you could change them by modifying utilit/at_drv.cpp and change
the definition of AT_DRV_SERIAL and AT_DRV_SERIAL1.

Strongly suggest to use ES/RST pin to enter the AT command mode, but if you still want to change mode by
sending specific serial data over serial port, just modify utilit/at_drv.cpp and mark #define USE_ESCAPE_PIN.

Others like baud rate and the GPIO used to pull ES/RST pin could be found in the from of utilit/at_drv.cpp.
You could understand their usage by checking the source and comments.

## Other Resources
User manual of HLK-RM04 module. (http://www.hlktech.net/inc/lib/download/download.php?DId=19)

This user manual doesn't contain the AT commands of lateset official firmware. (V1.78)
You could find more useful information from the OpenWrt forum. (https://forum.openwrt.org/viewtopic.php?id=42142)
