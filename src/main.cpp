#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ADG2188.h>

ADG2188 adg2188;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient* wsClient;

bool state = true;
int x = 0;
int y = 0;

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    wsClient = client;
  } else if(type == WS_EVT_DISCONNECT){
    wsClient = nullptr;
  }
}

void state_response(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(2048);
  if (request->hasParam("x") && request->hasParam("y"))
  {
    uint8_t x = request->getParam("x")->value().toInt();
    uint8_t y = request->getParam("y")->value().toInt();
    uint8_t state = (uint8_t)adg2188.getState(x, y);
    json["state"] = state;
  }
  else
  {
    const size_t CAPACITY = JSON_ARRAY_SIZE(64);
    // allocate the memory for the document
    StaticJsonDocument<CAPACITY> doc;
    json["state"] = doc.to<JsonArray>();

    adg2188.updateState();
    for (uint8_t x = 0; x < 8; x++)
    {
      for (uint8_t y = 0; y < 8; y++)
      {
        uint8_t state = (uint8_t)adg2188.getState(x, y, false);
        json["state"].add(state);
      }
    }
  }

  serializeJson(json, *response);
  request->send(response);
}

void setup()
{
  Serial.begin(115200);

  // SETUP - ADG2188
  adg2188.begin();

  // SETUP - WIFI
  WiFi.begin("ECMAScript-IOUT", "647rAy@6272836");
  uint32_t notConnectedCounter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.println("Wifi connecting...");
    notConnectedCounter++;
    if (notConnectedCounter > 50)
    { // Reset board if not connected after 5s
      Serial.println("Resetting due to Wifi not connecting...");
      ESP.restart();
    }
  }
  Serial.print("Wifi connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize webserver URLs
  server.on("/api/wifi-info", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument json(2048);
    json["status"] = "ok";
    json["ssid"] = WiFi.SSID();
    json["ip"] = WiFi.localIP().toString();
    serializeJson(json, *response);
    request->send(response);
  });

  server.on("/api/patchbay-state", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    state_response(request);
  });

  server.on("/api/patchbay-state", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (request->hasParam("state"))
    {
      const size_t CAPACITY = JSON_ARRAY_SIZE(64);
      StaticJsonDocument<CAPACITY> doc;
      deserializeJson(doc, request->getParam("state")->value());
      JsonArray array = doc.as<JsonArray>();

      if (array.size() == 64)
      {
        uint8_t i = 0;
        for (uint8_t x = 0; x < 8; x++)
        {
          for (uint8_t y = 0; y < 8; y++)
          {
            bool state = array[i] == 1;
            if (i == 63)
            {
              adg2188.set(state, x, y, true);
            }
            else
            {
              adg2188.set(state, x, y, false);
            }
            i++;
          }
        }
        state_response(request);
      }
      else
      {
        request->send(400, "application/json", "{}");
      }
    }
  });

  // Start webserver
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
}

uint64_t counter = 0;
void loop() {
  // If client is connected ...
  if(wsClient != nullptr && wsClient->canSend()) {
    // .. send hello message :-)
    wsClient->text("Hello client");
    Serial.print("Hello client ");
    Serial.println(counter);
    counter++;
  }

  delay(50);
}
