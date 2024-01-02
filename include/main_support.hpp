//-----------------------------------------------------------------------------
#ifndef MAIN_SUPPORT_HPP
#define MAIN_SUPPORT_HPP

///#define NO_REPLY               // send ACK replies or not by uncommenting
#define lamp LED_BUILTIN          // use LED_BUILTIN as the POL lamp
#define isHUB (BOARD_ID==HUB_ID)  // for HUB only code switch

#define DEFAULT_INITIAL_PERCENT  50
#define DEFAULT_INITIAL_STATE  true

//-----------------------------------------------------------------------------

#include <Arduino.h>
#include <credentials.h>

#include <functional>
#include <vector>
#include <queue>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <console_support.hpp>
#include "alexa_support.hpp"


//-- Globals, Prototypes and more ----------------------------------------------

uint8_t CustomIpAddress[]  = {192,168,2,BOARD_ID}; // static IP Address
uint8_t CustomMacAddress[] = {0,0,0,0,0,BOARD_ID}; // static MAC Address

String espName = String("esp")+BOARD_ID; // convention is espXXX
std::queue<String>awaiting;                        // UDP message send queue
std::queue<String>pending;                         // awaiting processing queue

extern void handlePendingQueue(void);

static char incoming[MAX_MSG_SIZE+1];    // buffer to hold incoming packet
static char outgoing[MAX_MSG_SIZE+1];    // buffer to hold outgoing packet

AsyncWebServer server(HTTP_PORT);        // Alexa requires port 80

WiFiUDP udpMsg;                          // Initialize UDP manager object
unsigned int udpMsgId = 0;               // UDP message sequencing number - ID
const char ACK[] = "@@";                 // initialize UDP acknowledgment str
unsigned int udpMsgPort = 7000 + HUB_ID; // shared UDP listening port

#if isHUB   // add in some UdpNode natwork topology management
  static uint8_t clientsList[UDPNET_NODES] = {0};
  static uint8_t clientCount = 1; // clientsList[0] is set to self - BOARD_ID
#endif//isHUB

static HTTPClient http;


//-- Common Support Methods ----------------------------------------------------

//-- Set ESP's WiFi IPAddress as static based on it's fixed BOARD_ID (above)
static void setCustomIpAddress() {
  IPAddress staticIP( {192, 168, 2, BOARD_ID} );
  IPAddress gateway(192, 168, 2, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);
  WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS);
}

//-- Set a custom MAC address based on the ESP's WiFi local IPAddress
void setCustomMacAddress() { // WiFi-router's restricts IPAddress[3] to 100-199
  #ifdef ARDUINO_ARCH_ESP32
    esp_wifi_set_mac(WIFI_IF_STA, CustomMacAddress); // Set Address, esp32
  #else  ///ARDUINO_ARCH_ESP8266
    wifi_set_macaddr(STATION_IF, CustomMacAddress); // Set Address, esp8266
  #endif ///ARDUINO_ARCH_ESP32||ARDUINO_ARCH_ESP8266
}

//-- Start or not USB Serial
void serialSetup(void) {
  #ifndef REUSE_RXTX              // not to repurpose Serial's pins
    Serial.begin(SERIAL_BAUD);    // setup Serial as usual
    Serial.setDebugOutput(false);
    Serial.println("\n");
  #else                            // repurpose Serial's RX and TX pins (ESP-01)
    #undef Serial                  // set Serial to NULL_MODE
    pinMode(3, FUNCTION_3);        // Rx  GPIO3
    pinMode(1, FUNCTION_3);        // Tx  GPIO1
    pinMode(3, OUTPUT);            // GPIO3
    pinMode(1, OUTPUT);            // GPIO1
    #define DEBUG_PORT disabled
    #define DEBUG_LEVEL none
  #endif
}

//-- Connect to WLAN based on Credentials
void wifiSetup(void) {
  WiFi.disconnect(true); // JIC avoid a soft reboot hangup issue
  WiFi.mode(WIFI_STA);
  setCustomIpAddress();
  setCustomMacAddress();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("\n\n  Connecting: ...");
  for (size_t tries=19; WiFi.status() != WL_CONNECTED; tries--) {
    if (!tries) {
      Serial.println("\nWiFi Failure ...rebooting!\n");
      delay(2000);
      ESP.restart();
    }
    Serial.print('.');
    delay(500);
  }
  Serial.print("\n   Connected: ");Serial.println(WIFI_SSID);
}//wifiSetup


//-- setup File System
void fsSetup(void) {
  if (LittleFS.begin()) {
    ///server.serveStatic("/", LittleFS, "/");
    Serial.printf("LittleFS mounted\n");
  } else Serial.println("  FS Mount: Failed (Try re-building and uploading file image.)");
}//serverSetup

void devSetup(void) { // setup ESP's defaultDevice for Alexa
  avc4espSetup(espName.c_str());               // defaultDevice's name
  avc4esp.onSetState([](unsigned char value) { // Alexa's on change callback
    pending.push(String(v2p(value)));          // convert v2p and queue
  });
}

//-- Setup OTA
void otaSetup() {
  //-- OTA Setup
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // file system
      Serial.flush();
      type = "filesystem";
      LittleFS.end(); // unmounting FS
    }
    Console.printf("Start updating %s\n",type.c_str());
  });
  ArduinoOTA.onEnd([]() {
    Console.printf("\nEnd\n");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Console.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Console.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Console.printf("Auth Failed\n");
    else if (error == OTA_BEGIN_ERROR) Console.printf("Begin Failed\n");
    else if (error == OTA_CONNECT_ERROR) Console.printf("Connect Failed\n");
    else if (error == OTA_RECEIVE_ERROR) Console.printf("Receive Failed\n");
    else if (error == OTA_END_ERROR) Console.printf("End Failed\n");
  });
  ArduinoOTA.begin();
}


//-- Common Support Methods ----------------------------------------------------

//-- Handle LittleFS File Upload -------------
void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File uploadFile; // File object to store the uploading file.
  if (!index) {
    if (!filename.startsWith("/")) filename = "/"+filename;
    uploadFile = LittleFS.open(filename, "w"); // Create persistent File object.
  }
  if ( uploadFile ) uploadFile.write(data, len);
  if ( final ) {
    if (uploadFile) {
      uploadFile.close();
      request->send(200, "text/plain", (filename+" uploaded!").c_str());
      Serial.printf("%s: File %s upload completed!\n", espName.c_str(), filename.c_str());
      if (filename.equals("cui.htm"))Console.shutdown(); // force reload of all CUIs
    } else {
      Serial.println("File upload failed.");
      request->send(500, "text/plain", "500: file create error.");
    }
  }
}

//-- Server 'on starts with' extensions ----
void handleDownload(AsyncWebServerRequest *request) {
  String target = request->url().substring(9); // /download/ and leave leading /
  while(target.endsWith("/"))target=target.substring(0,target.length()-1); // remove trailing /'s
  if ( target.length() > 0 ) {
    if (LittleFS.exists(target)) request->send(LittleFS, target.c_str(), "application/octet-stream");
    else request->send(404, "text/plain", "404: Not Found ");
  } else request->send( 400, "text/plain", "Download problem." );
  return; //...done.
}

void handleDelete(AsyncWebServerRequest *request) {
  String target = request->url().substring(7); // /deleate/ and leave leading /
  while ( target.endsWith("/") ) target = target.substring(0,target.length()-1);
  if ( target.length() > 0 ) {
    if ( LittleFS.exists(target) ) {
      LittleFS.remove(target);
      request->send( 200, "text/plain", target + " deleted." );
    } else request->send( 400, "text/plain", "Delete problem." );
  } else request->send( 400, "text/plain", "Delete problem." );
  return; //...done.
}

void handleClear(AsyncWebServerRequest *request) { // /clear/ and leave leading /
  String target = request->url().substring(6);
  while ( target.endsWith("/") ) target = target.substring(0,target.length()-1);
  if ( target.length() > 0 ) {
    if ( LittleFS.exists(target) ) {
      File file = LittleFS.open(target, "w");
      file.seek(0, SeekSet);
      file.close();
      request->send( 200, "text/plain", target + " cleared." );
    } else request->send( 400, "text/plain", "Clear problem." );
  } else request->send( 400, "text/plain", "Clear problem." );
  return; //...done.
}

void handleCreate(AsyncWebServerRequest *request) { // /create/ and leave leading /
  String target = request->url().substring(7);
  while ( target.endsWith("/") ) target = target.substring(0,target.length()-1);
  if ( target.length() > 0 ) {
    if ( !LittleFS.exists(target) ) {
      File file = LittleFS.open(target, "w");
      file.seek(0, SeekSet);
      file.close();
      request->send( 200, "text/plain", target + " created." );
    } else request->send( 400, "text/plain", "Create problem." );
  } else request->send( 400, "text/plain", "Create problem." );
  return; //...done.
}

//-- Determine File MIME Type
String getContentType(String filename, bool download) {
  if (download) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

//-- Handle LittleFS File Send ---------------
bool handleFileSend( AsyncWebServerRequest *request ) {
  String path = request->url();
  if (path.equals("/upload")) return true;
  if (path.endsWith("/")) path += "index.html";
  String gzPath = path + ".gz";
  if (LittleFS.exists(gzPath)) {
    Serial.println("File: " + path);
    request->send(LittleFS, gzPath.c_str(), (getContentType(gzPath,request->hasParam("download"))).c_str());
    return true;
  } else if (LittleFS.exists(path)) {
    Serial.println("File: " + path);
    request->send(LittleFS, path.c_str(), (getContentType(path,request->hasParam("download"))).c_str());
    return true;
  } else {
    Serial.println(path + "  : File Not Found!");
  }
  return false;
}
    
void handleNotFound(AsyncWebServerRequest *request) {
  //...elsewise, can't figure it out!
  Console.printf("NOT_FOUND: ");
  if (request->method() == HTTP_GET)
    Console.printf("GET");
  else if (request->method() == HTTP_POST)
    Console.printf("POST");
  else if (request->method() == HTTP_DELETE)
    Console.printf("DELETE");
  else if (request->method() == HTTP_PUT)
    Console.printf("PUT");
  else if (request->method() == HTTP_PATCH)
    Console.printf("PATCH");
  else if (request->method() == HTTP_HEAD)
    Console.printf("HEAD");
  else if (request->method() == HTTP_OPTIONS)
    Console.printf("OPTIONS");
  else
    Console.printf("UNKNOWN");

  Console.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

  if (request->contentLength()) {
    Console.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    Console.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
  }
  int i, headers = request->headers();
  for (i=0; i<headers; i++) {
    AsyncWebHeader* h = request->getHeader(i);
    Console.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }
  int params = request->params();
  for (i=0; i<params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if (p->isFile()) {
      Console.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    } else if (p->isPost()) {
      Console.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      Console.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }
  request->send(404, "text/plain", "404 unknown");
}


//-- Setup web server
void serverSetup() {
  // Custom entry point (not required by the library, here just as an example)
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "turn device on!");
    pending.push("on");
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "turn device off!");
    pending.push("off");
  });

  server.on("/ver", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", VERSION);
  });

  // Custom entry points, not required by the library, but here as examples
  server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {
  	char response[192];
    uint8_t len = snprintf(response, sizeof(response),
        " VERSION: %s\nESP Name: %s\n WiFi IP: %s\n     MAC: %s",
        VERSION, WiFi.localIP().toString().c_str(), espName.c_str(), WiFi.macAddress().c_str());
    response[len] = '\0';
    Serial.println(response);
    request->send(200, "text/text", response);
  });

  server.on("/dev", HTTP_GET, [](AsyncWebServerRequest *request) {
  	char response[32];
    uint8_t len = snprintf(response, sizeof(response), "%s/%s/%s/%d/%d",
        espName.c_str(), defaultDevice.name, (defaultDevice.state?"active":"inactive"),
        defaultDevice.value, v2p(defaultDevice.value));
    response[len] = '\0';
    Serial.println(response);
    request->send(200, "text/text", response);
  });

  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("\nFS listing:");
    String response = "";
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      response += dir.fileName() + " / " + dir.fileSize() + "\r\n";
    }
    Serial.print(response);
    request->send( 200, "text/plain", response );
  });

  server.on("/reboot", [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "...rebooting.");
    delay(1000);
    ESP.restart();
  });

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", upload_html);
    //request->send(LittleFS, "/upload.htm", "text/html" );
  });
  server.onFileUpload(handleFileUpload); //...processing

  // Next two callbacks are required for Alexa gen1/gen3 devices compatibility
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (avc4esp.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data))) return;
    // Handle any other body request here...
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
    if (avc4esp.process(request->client(), request->method() == HTTP_GET, request->url(), body)) return;

    //-- Server 'on starts with' extensions ----
    if ( request->url().startsWith("/download/") ) {
      return handleDownload(request);
    }
    if ( request->url().startsWith("/delete/") ) {
      return handleDelete(request);
    }
    if ( request->url().startsWith("/clear/") ) {
      return handleClear(request);
    }
    if ( request->url().startsWith("/create/") ) {
      return handleCreate(request);
    }
    if (request->url().startsWith("/api")) { //...skip other api calls
      request->send(200, "text/plain", "OK");
      return;
    }

    // ... add more if (path.startsWith("/...")) {...} ...as needed

    String msg = request->url().substring(1); // as integer command request?
    if (msg.toInt() > 0 || (char)'0' == msg.charAt(0)) {
      String response("Set device to ");
      request->send(200, "text/plain", (response+=msg));
      pending.push(msg);
      return;
    }

    // Finally, assume it might be a simple LittleFS file transfer request
    if (handleFileSend(request)) return; // LittleFS file send

    handleNotFound(request);

  });

  Console.begin(&server); // Start ConsoleUI using current server instance, and

  server.begin();

}//serverSetup


//-- Print to IDE USB Monitor Build Summary
void displayBuildSummary(void) {
  Console.printf("     Version: %s\n", VERSION);
  Console.printf("  UDP Server: %d\n", udpMsgPort);
	Console.printf("  Build Date: %s at %s\n", __DATE__, __TIME__);
  if (LittleFS.exists("/favicon.ico")) Console.printf(" File System: Mounted\n");
  Console.printf("  IP address: %s\n", WiFi.localIP().toString().c_str());
  Console.printf(" MAC address: %s\n", WiFi.macAddress().c_str());
  Console.printf("    ESP Name: %s\n", espName.c_str());
}//displayBuildSummary


//-- String to String-Tokens Vector, non-destructive - utility
void str2tokens(String data, std::vector<String> &tokens, 
    size_t max=0, const char delim='/') { // defaults: to all tokens and SSV
  //...strip and tokenize the String data
  size_t i=0, k=data.length();
  while(i<=data.length() && data.charAt(i)==delim){ ++i; } // skip leading /'s
  while(--k>i && data.charAt(k)==delim); // ...and skip ending /'s
  for (size_t n=i; i <= k; ++i) {
    if (data.charAt(i) == delim || i == k) {
      tokens.push_back(data.substring(n,i==k?i+1:i));
      if (max && tokens.size() == max-1) {
        tokens.push_back(data.substring(i+1)); //...anything else as last
        return;
      }
      n = i+1;
    }
  }
  // FYI USAGE:
  // std::vector<String>tokens;     // Results - tokens list vector
  // str2tokens(msg, tokens,5,'/'); // 0-4 where tokens[4] is everything else
  // for(size_t i=0;i<tokens.size();i++)Serial.printf( ... );
  // Project's Message SSV Format: CMND / DEST / ORIG / MSG_ID [ /data ]
  //     command - tokens[0]
  //  destintion - tokens[1]
  //      origin - tokens[2]
  //  message id - tokens[3]
  //  extra data - tokens[4] ...for command processing  (think another SSV list)
}//str2tokens


//-- UDP Messaging Support -----------------------------------------------------

//-- Handle Incomming UDP Messages
static void handleIncomingUdpMsg() {
  int packetSize = udpMsg.parsePacket();
  if (packetSize) { // IFF there is a packet available read it in now
    int len = udpMsg.read(incoming, MAX_MSG_SIZE);
    incoming[len] = '\0';
    String msg = incoming;

    ///Serial.printf("%d :: RAW [%s]\n", BOARD_ID, msg.c_str());

    // FYI:
    // just because NO_REPLY is set for this ESP does not stop other peers
    // from sending their ACK responses. Handle that case here as well 
    #ifndef NO_REPLY // client must send ACK reply per valid incoming message
      if (msg.indexOf("@@") == -1) { // msg is NOT a reply so send ACK back
        udpMsg.beginPacket(udpMsg.remoteIP(), udpMsg.remotePort());
        udpMsg.print(ACK);
        udpMsg.endPacket();
        udpMsg.flush();
        delay(0);
      } else { // is a another's ACK message, for now, just skip ACK processing
        ///Serial.printf("%d ++ [%d:%d %lu] :: %s\n", BOARD_ID, udpMsg.remoteIP()[3], udpMsg.remotePort(), millis(), msg.c_str() );
        return; // ...and exit any further incoming message processing
      }
    #endif//NO_REPLY

    // ...regardless, NO_REPLY or not, skip incoming ACKs fron others
    if (msg.indexOf("@@") == -1) { // is not an ACK reply from another node, so
      pending.push(msg); // queue as valid message for deferred processing ASAP
    }

    #if isHUB // check and if needed, save IP[3] as another HUB client
      for (size_t i=1; i < UDPNET_NODES ; i++) {
        if (clientsList[i]) { // non-zero
          if (clientsList[i] == udpMsg.remoteIP()[3]) break; // found - done
        } else { // upon first zero in list, client not found so save board id
          clientsList[(clientCount=i)] = udpMsg.remoteIP()[3];
          break;  // not found but added - done
        }
      }
    #endif//isHUB

  }// else, no packet available just now, so done

}//handleIncomingUdpMsg


//-----------------------------------------------------------------------------

//-- Arduino core supplements: Setup, Loop and processing directives
void udpNetSetup(void)
{
  udpMsgId = 0;                 // reset UDP message ID upon reboot

  #if isHUB                     // manage HUB's UDP clients, nodes, list
    clientCount = 1;            // clean and reset upon reboot
    clientsList[0] = BOARD_ID;  // add self as first in the HUB's clientsList
  #endif//isHUB

  serialSetup();                // alowing repurposing Serial's RX and TX pins

  wifiSetup();                  // w/ custom MAC and IP Addresses

  udpMsg.begin(udpMsgPort);     // EspUdpNet's core messaging mechanics

  fsSetup();                    // setup file system - default LittleFS

  serverSetup();                // full HTTP and WebSockets functionality

  devSetup();                   // ESP's defaultDevice setup

  otaSetup();                   // enable Arduino's OTA for text and data

  Console.shutdown();           // ensure CUIs restart upon reboot
}


void udpNetLoop(void)     // udpNet node's core mechanics - polling methods
{
  handlePendingQueue();   // process first in FIFO pending queue and if any

  handleIncomingUdpMsg(); // handle incoming udpMsg packets from other nodes

  ArduinoOTA.handle();    // process OTA for sketch and/or data

  avc4esp.handle();   // poll and process Alexa's UDP packet requests
}


bool udpNetDirectives(String cmd)
{
  if (cmd.equals("ver")) {
    Console.println(VERSION);
    return true;
  }

  if (cmd.equals("info")) {
    displayBuildSummary();
    return true;
  }

  if (cmd.equals("dev")) {
  	char response[32];
    uint8_t len = snprintf(response, sizeof(response), "%s/%s/%s/%d/%d",
        espName.c_str(), defaultDevice.name, (defaultDevice.state?"active":"inactive"),
        defaultDevice.value, v2p(defaultDevice.value));
        response[len] = '\0';
    Console.println(response);
    return true;
  }

  if (cmd.equals("list")) {         // HUB testing - no real processing
    Serial.println("\nFS listing:");
    String response = "";
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      response += dir.fileName() + " / " + dir.fileSize() + "\r\n";
    }
    Console.print(response);
    return true;
  }

  ///GSB - also add file options: clear, delete and create

  return false;
}///processCoreDirectives

#endif//MAIN_SUPPORT_HPP
//-----------------------------------------------------------------------------