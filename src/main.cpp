//-----------------------------------------------------------------------------

#define BOARD_TYPE    NodeMCU 1.0 (ESP-12E Module)


#include <Arduino.h>
#include "main_support.hpp"


//-- Controler Globals, Prototypes and more

//-----------------------------------------------------------------------------
//-- Arduino core setup method ------------------------------------------------
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT); // lamp mode for POL

  udpNetSetup();                // Core mechanics of all udpNet nodes

  displayBuildSummary();        // display short build summary

  Console.printf("    ...Setup: complete\n\n");

  Serial.printf("\n   For Console: CTL-Click http://%s \n\n", WiFi.localIP().toString().c_str());
}


//-- Arduino core loop method -------------------------------------------------
void loop()
{
  //-- Async HeartBeats: is a one-second 'heartbeat' timer loop (optional)
  static unsigned long heartbeats = 0;
  static unsigned long lastUpdated = 0;
  unsigned long currentMillis = millis();
  for (unsigned int delta=0;  // 1-second 'heartbeat' timer loop (optional)
    (delta=(currentMillis-lastUpdated)) >= 1000; // roll-over error possible
    lastUpdated=(currentMillis-(delta%1000)),
    ++heartbeats)
  {
    if (heartbeats) { // do every 1-sec except the very first

      digitalWrite(lamp, !digitalRead(lamp)); // visual POL - optional


      // Add user 1-sec loop actions as needed here like repeat every n-secs:
      //    if ( !(HeartBeats%n) ) { ... }


      if (!(heartbeats%3)) // DEMO: send a test message to a 'DEST' every 3-sec
      {
        #if !isHUB // determine DEST based on node type HUB or not as origin
          uint8_t DEST = HUB_ID; // not HUB so send test message to the HUB
        #else // HUB sends to a random EPN found in HUB's clientsList
          uint8_t DEST = clientsList[random(1,clientCount+1)]; // pick only one
        #endif

        if (DEST) { // build and send DEMO's test message to DEST node
          int n = sprintf(outgoing, "/test/%d/%d/%u/%u/%lu",
              DEST, BOARD_ID, udpMsgId++, ESP.getFreeHeap(), millis()); // DEMO
          outgoing[n] = '\0';
          udpMsg.beginPacket({192,168,2,DEST}, udpMsgPort);
          udpMsg.print(outgoing);
          udpMsg.endPacket();
          udpMsg.flush();
          Console.printf("%d >> %s \n", BOARD_ID, outgoing); // IDE monitor echo
        }
      }

    } else { // do only once, at startup loop '0' (for additional setup)

      // ...user's additional startup requirements as needed ...none for DEMO

    }/// loop zero or not...

  } //HeartBeats

  udpNetLoop(); // Core mechanics for all EspUdpNet nodes - polling routines

  /// Add user polling routines here

}//loop


//-- Message Processing Directives --------------------------------------------
void handlePendingQueue()
{
  if(!pending.size())return;    // None queued, nothing to do, so return

  String msg = pending.front(); // else, get first queued message, remove it 
  pending.pop();                // from queue and begin processing it below.

  std::vector<String>tokens;       // list of String-tokens via spliting message
  str2tokens(msg, tokens, 5, '/'); // capture 5 tokenized fields in SSV string
    // Project's Message SSV Format: CMND / DEST / ORIG / MSG_ID [ /data ]
    //     command - tokens[0]
    //  destintion - tokens[1]
    //      origin - tokens[2]
    //  message id - tokens[3]
    //  extra data - tokens[4] data for command processing, if any needed
  
  if (!tokens.size()) return; // if message is not properly formatted, skip it

  ///Console.printf("%d << %s \n", BOARD_ID, msg.c_str()); // display incoming msg

    /*** State Change Notes ***
     * The 0/1 percent values change the 'state' only mimicking Alexa's
     * ON/OFF device button. Others, 2-100, change the 'percent' and 'value'
     * but not the device's 'state'. This allows consistency w/ Alexa device
     * on/off functionality¹ and allows system directed overrides of the
     * actual Alexa sent values based on the current device environment.
     * 
     * ¹ Alexa's UI forces the 'ON' state when physically manipulating it's
     *   brightness slider. 'avc4esp' defeats this functionality by ignoring 
     *   the slider's "onMove - force to ON state" change requests.
     */

  uint8_t percent = tokens[0].toInt();
  if (percent==1) tokens[0] = "on"; // process as String "ON" command below
  else if (percent==0 && (char)'0'==tokens[0].charAt(0)) tokens[0] = "off";
  else if (percent>1) { // not on/off so process as int command 2-100
    if (percent>100)percent=100; // force boundries as  0 <= percent <= 100
    unsigned char value = p2v(percent);
    ///Console.printf("%d << %s \n", BOARD_ID, msg.c_str()); // display incoming msg
    switch (percent) { // Process 'integer' command (2-100) - from any source
      case 2:
      case 3:  
        // ...as needed to process and set the device's value and/or percentage
      case 100:
        defaultDevice.value = value;
        defaultDevice.percent = percent;
        Console.printf("Specific Digital Processing 2, 3, or 100  [%d]\n", percent);
        break;

      default: // 4...99
        defaultDevice.value = value;
        defaultDevice.percent = percent;
        Console.printf("Standard Digital Processing for '%d'\n", percent);
        break;
    }

    return;
  } // was int command
  
  if (udpNetDirectives(tokens[0])) return;

  //-- Device control actions (examples) 
  if (tokens[0].equals("on")) {
    defaultDevice.state = true;             // set the ESP's state to on
    Console.println("ESP set to active!");
    //digitalWrite(lamp,(HIGH+INVERTED)%2); // visual confirmation
    return;
  }

  if (tokens[0].equals("off")) {
    defaultDevice.state = false;            // set the ESP's state to off
    Console.println("ESP set to inactive!");
    //digitalWrite(lamp,(LOW+INVERTED)%2);  // visual confirmation
    return;
  }

  if (tokens[0].equals("test")) {         // HUB testing - no real processing
    return;
  }

  Console.printf("No String Processing Directive for '%s'\n", msg.c_str());

}//handlePendingQueue

//-----------------------------------------------------------------------------