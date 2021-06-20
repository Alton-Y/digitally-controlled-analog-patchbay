#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ADG2188.h>

ADG2188 adg2188;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *wsClient;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    wsClient = client;
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    wsClient = nullptr;
  }

  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void state_response(AsyncWebServerRequest *request)
{
  //adg2188.printState();
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonDocument json(2048);

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

  serializeJson(json, *response);
  request->send(response);
}

void setup()
{
  Serial.begin(115200);

  // SETUP - ADG2188
  adg2188.begin();

  // SETUP - SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

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

  // // TODO: Only required to to allow cross-domain requests
  // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  // DefaultHeaders::Instance().addHeader("Access-Control-Max-Age", "10000");
  // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
  // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
                      if (request->method() == HTTP_OPTIONS)
                      {
                        request->send(200);
                      }
                      else
                      {
                        request->send(404);
                      }
                    });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

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

  server.on(
      "/api/patchbay-state",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
        DynamicJsonDocument json(2048);
        deserializeJson(json, data);
        Serial.println(json.as<String>());
        if (json.containsKey("state"))
        {
          JsonArray array = json["state"].as<JsonArray>();

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
            return;
          }
        }
      });

  server.on("/api/patchbay-state", HTTP_GET, [](AsyncWebServerRequest *request)
            { state_response(request); });
  
  server.on("/static/media/error.webp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/static/media/error.webp", "image/webp");
  });

  // Start webserver
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
}

uint64_t counter = 0;
float v1 = 0;
float r1 = 0.005;

float v2 = 0;
float r2 = 0.01;

unsigned long prevTime;
unsigned long currentTime;
unsigned long deltaTime;

void loop()
{

  ws.cleanupClients();

  char buffer[64];

  currentTime = millis();
  deltaTime = currentTime - prevTime;

  v1 = v1 + deltaTime * r1;
  v2 = v2 + deltaTime * r2;

  if (v1 > 10)
  {
    v1 = 10;
    r1 = -r1;
  }
  else if (v1 < 0)
  {
    v1 = 0;
    r1 = -r1;
  }

  if (v2 > 10)
  {
    v2 = 10;
    r2 = -r2;
  }
  else if (v2 < 0)
  {
    v2 = 0;
    r2 = -r2;
  }

  sprintf(buffer, "{\"timems\":%lu, \"status\":[%.3f, %.3f]}", currentTime, v1, v2);
  ws.textAll(buffer);

  prevTime = currentTime;

  delay(200);
}
