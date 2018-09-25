# NodeMCU proof-of-concept : securing IOT with Oauth and crypto


### Dependencies

 Go on preferences menu and add this `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to the `Additional Boards Manager URLs` field.

 * ESP8266WiFi
 * ESP8266mDNS
 * ESP8266WebServer
 * [ESP8266RestServer](https://github.com/tigerwill90/esp8266-restclient)
 * [AES](https://github.com/spaniakos/AES)
 * [Base64](https://github.com/adamvr/arduino-base64)
 * [ArduinoJson v5.13.2](https://github.com/bblanchon/ArduinoJson)
 * [Arduino IDE](https://www.arduino.cc/en/main/software)

### Getting started

You need first to add all dependencies above.

```
git clone https://github.com/tigerwill90/ArduinoUno-Oauth.git
Open the sketch "nodemcu.ino" in Arduino IDE
Select the right board and port
compile and upload the sketch
```

### Version
v0.2-dev
