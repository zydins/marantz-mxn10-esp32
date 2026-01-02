#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsClient.h>
#include "pm6006_web.h"

const char* WIFI_SSID     = "<Wi-FI Name>";
const char* WIFI_PASSWORD = "<Wi-FI Password>";

#define IR_TX_PIN 32 // PIN to which Mono Jack is attached

AsyncWebServer server(80);
WebSocketsClient webSocket;

void setupWebServer() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("marantz");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  esp_wifi_set_ps(WIFI_PS_NONE);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);

  if (MDNS.begin("amp")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://amp.local");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", buildPm6006Page());
  });

  server.on("/press", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasArg("btn")) {
      String btn = request->arg("btn");
      sendAmpButton(btn);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing btn");
    }
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
  Serial.println("HTTP server ready");
}

void setupWebSocketConnection() {
  String ip = discoverStreamMagicIP();
  if (ip == "") {
    // Default fallback
    ip = "192.168.178.155";
  }
  Serial.println("MXN10 IP: " + ip);

  webSocket.begin(ip, 80, "/smoip");

  String headers = "";
  headers += "Origin: http://" + ip + "\r\n";
  headers += "Cache-Control: no-cache\r\n";
  headers += "Pragma: no-cache\r\n";
  headers += "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36\r\n";
  
  webSocket.setExtraHeaders(headers.c_str());
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(2000, 3000, 2);
  Serial.println("WebSocket ready");
}

void setup() {
  Serial.begin(115200);
  Serial.println("PM6006 ESP32 Controller");
  pinMode(IR_TX_PIN, OUTPUT);
  setupWebServer();
  setupWebSocketConnection();
}

void loop() {
  webSocket.loop(); 
}
