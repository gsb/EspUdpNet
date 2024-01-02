//-----------------------------------------------------------------------------
#ifndef CONSOLE_SUPPORT_HPP
#define CONSOLE_SUPPORT_HPP

#ifndef ECHO
  #define ECHO 1
#endif// ECHO

#include <Arduino.h>
#include <queue>

extern std::queue<String>pending;
extern void handleWebSocketMessage(void*, uint8_t*, size_t);
extern void processMessage(String);

//-----------------------------------------------------------------------------

#ifndef HIDE_SHOW_WEB_PAGES_HTML
PROGMEM const char CONSOLE_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>Custom Monitor 220101</title>
    <meta name="monitor" content="width=device-width, initial-scale=1">
    <meta http-equiv = 'content-type' content = 'text/html; charset = UTF-8'>
    <link rel="shortcut icon" href="data:image/x-icon;base64,AAABAAEAEBAAAAAAIABoBAAAFgAAACgAAAAQAAAAIAAAAAEAIAAAAAAAQAQAAAAAAAAAAAAAAAAAAAAAAAD///8BAABmPQAAZskAAGbtAABm6wAAZr0AAGYv////Af///wH///8BAABmHwAAZpMAAGbDAABmkQAAZh3///8BAABmPwAAZvkAAGaPAABmJQAAZisAAGahAABm9wAAZjn///8BAABmHwAAZuMAAGbBAABmbwAAZsMAAGbrAABmKQAAZsUAAGaNAABmDwAAZqEAAGaVAABmCQAAZo8AAGbN////AQAAZpEAAGa7AABmEQAAZlEAAGYHAABmpwAAZrkAAGbrAABmLwAAZpsAAGa/AABmwQAAZq0AAGYZAABm7QAAZicAAGanAABmiwAAZicAAGbXAABmkwAAZi8AAGbnAABm4QAAZlMAAGZRAABmZwAAZkUAAGbfAABmEQAAZuUAAGZfAABmUQAAZvEAAGZfAABmuQAAZpsAAGYtAABm6QAAZn0AAGbvAABmQQAAZiUAAGbFAABmqQAAZhcAAGb7AABm1QAAZgsAAGZjAABm2QAAZqcAAGYJAABmmQAAZr////8BAABmhQAAZu0AAGbxAABmtQAAZgsAAGaHAABm/wAAZv8AAGbRAABmOwAAZhMAAGYbAABmlQAAZvUAAGY1////Af///wEAAGYPAABmFf///wEAAGZ9AABm/wAAZq0AAGZ7AABmpwAAZusAAGbtAABm8QAAZs0AAGY3////Af///wH///8B////Af///wEAAGZjAABm9QAAZjkAAGYPAABmSQAAZiEAAGYJAABmMQAAZiMAAGYD////Af///wH///8B////Af///wEAAGYDAABm2wAAZmcAAGYvAABm7QAAZt8AAGb5AABmdf///wH///8B////Af///wH///8B////Af///wH///8BAABmIQAAZusAAGYfAABmuwAAZof///8BAABmSQAAZvMAAGYx////Af///wH///8B////Af///wH///8B////AQAAZh0AAGbrAABmJQAAZqMAAGa7AABmuwAAZhkAAGbPAABmZf///wH///8B////Af///wH///8B////Af///wH///8BAABmyQAAZo8AAGYNAABmnQAAZm8AAGYXAABm6wAAZkf///8B////Af///wH///8B////Af///wH///8B////AQAAZj0AAGb3AABmlwAAZicAAGY7AABm1QAAZrv///8B////Af///wH///8B////Af///wH///8B////Af///wH///8BAABmNwAAZsUAAGbvAABm6wAAZpsAAGYP////Af///wH///8B////Af///wH///8B////Af///wH///8B////Af///wH///8B////Af///wH///8B////Af///wH///8B////Af///wH///8BAAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//wAA//8AAP//AAD//w==" />
    <style>
      *{box-sizing:border-box;}:focus{outline:none;}
      body{overflow-y:hidden;}
      input{font-size:18px;width:calc(99vw - 6px);height:35px;padding:5px;align-self:center;}
      textarea{resize:none;border:none;width:calc(99vw - 6px);height:calc(99vh - 46px);padding:10px;align-self:center;}
    </style>
    <script>
      window.$=document.querySelector.bind(document);
      $.offline=function(){$.isOnline=false;}
      $.scroll2bottom=function(){setTimeout(function(){$.textarea.scrollTop=$.textarea.scrollHeight-50},0);}
      $.addMessage=function(n){$.textarea.value += n;if($.scrollingEnabled)$.scroll2bottom();}
      $.wsInit=function(){
        $.ws=new WebSocket('ws://'+window.location.host+'/ws');
        $.ws.onopen=function(n){ $.isOnline=true; }
        $.ws.onclose=function(n){$.isOnline=false;setTimeout(window.location.reload(),1e3);}
        $.ws.onmessage=function(n){
          if ('string' == typeof n.data){
            $.comFailures=0;$.isOnline=true;
            switch (n.data){
              case "pong":console.log("ponged");break;
              default:$.addMessage(n.data);break;
            }
          }
        }
      }
      $.comCheck=function(){
        if($.isOnline && $.comFailures++>3){$.isOnline=false;$.ws.send('ping');console.log(">> ping");}
        setTimeout($.comCheck,2e3);
      }
      window.onload=function(){
        $.isOnline=false;$.comFailures=0;$.scrollingEnabled=1;$.input=$('input');$.textarea=$('textarea');
        $.textarea.onclick=function(e){
          if(e.altKey){
            $.scrollingEnabled=!$.scrollingEnabled;
            console.log('scrolling '+($.scrollingEnabled?'on':'off'));
          }
        }
        $.wsInit();$.comCheck();$.textarea.innerHTML='';
        $.input.onkeydown=function(n){
          if (13 == n.keyCode){
            switch ($.input.value){
              case "":$.textarea.value=''; break;
              default:console.log('>> '+$.input.value);$.ws.send($.input.value);break;
            }
            $.input.value='';
          }
        }
      }
    </script>
  </head>
  <body>
    <div>
      <label><input type = 'text'/></label>
      <textarea onClick = ''></textarea>
    </div>
  </body>
</html>
)rawliteral";
PROGMEM const char alert_html[] = R"rawliteral(
<!DOCTYPE html><html><head><link rel="shortcut icon" href="data:image/x-icon;base64,"/><style>.alert {font-family:Arial,Helvetica,sans-serif;font-size:20px;margin:50px auto auto auto;width:70%;border:3px solid rgba(0,0,0,0.5);padding:16px;box-shadow:0 4px 8px 0 rgba(0,0,0,0.2);text-align:center;background-color:#f1f1f1;}</style></head<body><div class="alert"><p>Close some active windows and try again.</p></div></body></html>
)rawliteral";
PROGMEM const char upload_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <title>ESP8266 File Uploader</title>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width,initial-scale=1.0">
    <meta name="referrer" content="no-referrer">
    <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate" />
    <meta http-equiv="Pragma" content="no-cache" />
    <meta http-equiv="Expires" content="0" />
    <link href="favicon.ico" rel="shortcut icon" type="image/x-icon" />
    <style type="text/css">
      body{font-family:sans-serif;color: #444;background-color:#ddd;}
    </style>
  </head>
  <body>
    <h2>ESP8266 'Drag-N-Drop' File Upload</h2>
    <form method="POST" enctype="multipart/form-data">
      <input type="file" name="data" onchange="document.getElementById('auto_submit').click()" >
      <input id="auto_submit" style="display:none;" type="submit" value="upload">
    </form>
  </body>
</html>
)rawliteral";
#endif///HIDE_SHOW_WEB_PAGES_HTML

typedef std::function<void(void *arg, uint8_t *data, size_t len)>RecvMsgHandler; // Callback method typedef.
class ConsoleClass{
  private:
      AsyncWebServer *_server;
      AsyncWebSocket *_ws;
      RecvMsgHandler _RecvFunc = NULL;
  public:

    void begin(AsyncWebServer *server, const char* url="/") {
      _server = server;
      _ws = new AsyncWebSocket("/ws"); // Define WebSocket object, ws, using server URL "/ws".

      // On Server's root-request display the Console WebPage w/ or w/o login
      _server->on(url,[&](AsyncWebServerRequest *request){
        #ifdef INC_LOGIN // ...using login or not.
          if(!request->authenticate(LOGIN_NAME, LOGIN_PASS))
            return request->requestAuthentication();
        #endif
        _ws->cleanupClients(MAX_CUIS); // Limits clients list to manage runtime heap size.
        if (_ws->count() >= MAX_CUIS)  // MAX_CUIS consoles already open, don't allow another.
          request->send_P(200, "text/html", alert_html); //...internal
          else request->send_P(200, "text/html", CONSOLE_HTML); //...or
          ///else request->send(LittleFS, "/cui.html", "text/html");
      });

      _ws->onEvent([&](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) -> void {
        switch (type) {
          case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
          case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
          case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
          case WS_EVT_PONG:
          case WS_EVT_ERROR:
            break;
        }
      });

      _server->addHandler(_ws); // Add a WebSocket handler to the async server.
    }

    void msgCallback(RecvMsgHandler _recv){_RecvFunc = _recv;}

    ///old termplate:   void print(String m){if(_ws->count())_ws->textAll(m);if(ECHO||!_ws->count())Serial.print(m);}

    void print(String m){_ws->textAll(m);if(ECHO)Serial.print(m);}
    void print(const char *m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}
    void print(char *m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}
    void print(int m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}
    void print(uint8_t m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}
    void print(uint16_t m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}
    void print(uint32_t m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}
    void print(double m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}
    void print(float m){_ws->textAll(String(m));if(ECHO)Serial.print(m);}

    void println(String m){_ws->textAll(m+"\n");if(ECHO)Serial.println(m);}
    void println(const char *m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}
    void println(char *m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}
    void println(int m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}
    void println(uint8_t m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}
    void println(uint16_t m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}
    void println(uint32_t m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}
    void println(float m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}
    void println(double m){_ws->textAll(String(m)+"\n");if(ECHO)Serial.println(String(m));}

    void printf(const char * printf_formatting,...) {
      static char buf[MAX_MSG_SIZE+1]; ///GSB fix this!!
      size_t n = 0;
      size_t len = sizeof(buf)-1;
      va_list printf_argptr;
      va_start(printf_argptr, printf_formatting);
      n = vsnprintf(&buf[0], len, printf_formatting, printf_argptr);
      va_end(printf_argptr);
      buf[n] = '\0';
      _ws->textAll(buf);
      if(ECHO)Serial.print(buf);
    }

    void send(String m){_ws->textAll(m);}
    void shutdown(){_ws->closeAll();} // force reload of all opened CUIs
};
ConsoleClass Console; // Instantiate a global object

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//-- Receive incoming message from the ConsoleUI WebPage (user input field)
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg; //...to look into the info object.
  if (info->final && info->index == 0 && !!info->len && info->len == len && info->opcode == WS_TEXT) {
    String msg = "";
    for (size_t ndx=0; ndx<len && ndx<MAX_MSG_SIZE; ndx++) msg += (char)data[ndx];

    // Do some pre-processing here, for example only:
    if(msg.equals("reboot")) { // Software reboot node.
      Console.println("\n~~ rebooting...\n");
      delay(1000);
      ESP.restart();
    }

    if (msg.equals("ping")) Console.send("pong");
    else pending.push(msg); // queue for processing
  }
}

#endif//CONSOLE_SUPPORT_HPP
//-----------------------------------------------------------------------------
