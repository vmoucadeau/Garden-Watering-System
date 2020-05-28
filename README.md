# Garden Watering System

Principle of operation :

The system is composed of a master module, this can be an ESP8266 (ESP-12E) or an ESP32, on which an RTC module is connected (DS3231 in my case).
Solenoid valves can be connected directly to the master module (via an H-bridge for latching valves) or
remotely thanks to children modules (which are also ESP8266 but less demanding, it can be a very small ESP-01 alone or with an arduino to have more
of GPIOs)
A web interface is located in the SPIFFS memory of the master module and allows to create/delete cycles/solenoid valves.
The cycles are adjustable with a start time and an end time, and it is possible to select the days of activation.
It is also possible to create several cycles for a single solenoid valve.

Note:

The program is far from being finished, I started it not very long ago and I plan to add several features in the future (ex: rain gauge)
It is not commented/documented at all at the moment... It must be functional for this summer (essential for my garden :) )
Everything has been coded on PlatformIO for Visual Studio Code, a very good alternative to the Arduino IDE I find.
