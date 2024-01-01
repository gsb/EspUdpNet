
# ESP UDP Network

EspUdpNet is a WLAN of ESP8266s in a 'hub-and-spokes' topology with UDP text messaging, Alexa Voice Controls and HTML monitor page.


## Features

- PlatformIO project based
- Hub-and-Spokes WLAN topology
- 'Plug-And-Play' ESP node management
- Intra-node UDP formatted text messaging
- Alexa Voice Controls including
- HTML custom user interface page example included
- File system 'LittleFS' with files management tools
- Arduino OTA

## Demo Template
This PlatformIO project directory is an EspUdpNet example template. The platformio.ini is the configuration master for building any network node, Hub and end-point nodes alike.

- 'build_flags' define a unique BOARD_ID for each build
- upon build/upload, setup code registers the ESP with a unique WLAN IPAddress - 192.168.2.{BOARD_ID}
- 'build_flags' include the Hub's BOARD_ID in all ESP builds
- ...other common and target specific network build options are maintained within platformio.ini

## Notes
- All intra-node message traffic is routed through the Hub.
- Each node has a direct connection to Alexa for control and monitoring.
- Each node has WLAN browser HTML monitoring and control.
- Message trafficing is managed via a 1-sec internal loop¹.

¹ My physical device sensors and controlers have set and/or get latencies precluding 'true' real-time access. Also, I often use smoothd-data-sampling via a circular readings storage queue for most physical device sensors. And finally, Alexa voice controls and user commands from web page input forms or HTTP GET requests are far from truely real-time operations making 1-sec message looping practical.
