
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
- Test message trafficing is managed via a 1-sec interval loop.

## Caution
EspUdpNet is a DIY in-house project by a non-developer, myself, and not ready for prime-time implementation.
