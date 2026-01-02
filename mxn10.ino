#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define SSDP_PORT 1900
#define SSDP_ADDR "239.255.255.250"

WiFiUDP udp;

String prevState = "null";
unsigned long disabledAtMillis = 0;

String discoverStreamMagicIP() {
  Serial.println("Sending SSDP discovery...");

  udp.begin(SSDP_PORT);

  String search =
    "M-SEARCH * HTTP/1.1\r\n"
    "HOST: " SSDP_ADDR ":" + String(SSDP_PORT) + "\r\n" 
    "MAN: \"ssdp:discover\"\r\n"
    "MX: 3\r\n"
    "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n\r\n";

  udp.beginPacket(SSDP_ADDR, SSDP_PORT);
  udp.print(search);
  udp.endPacket();

  unsigned long start = millis();
  while (millis() - start < 10000) {
    int size = udp.parsePacket();
    if (size) {
      char buf[1024];
      int len = udp.read(buf, sizeof(buf) - 1);
      buf[len] = 0;

      String response = String(buf);
      // Serial.println("Response: " + response);

      if (response.indexOf("StreamMagic") >= 0) {
        String mxn10_ip = udp.remoteIP().toString();
        Serial.println("Found MXN10 (StreamMagic) at " + mxn10_ip);
        return mxn10_ip;
      }
    }
  }

  Serial.println("MXN10 not found");
  return "";
}

String getPlaybackState(String playStateJson) {
  // Filter: Only keep params -> data -> state
  JsonDocument filter;
  filter["params"]["data"]["state"] = true;
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, playStateJson, DeserializationOption::Filter(filter));

  if (!error) {
    String state = doc["params"]["data"]["state"].as<String>();
    if (state != "null") {
      return state;
    } else {
      Serial.println("Unable to find state in response: " + playStateJson);
      return "";
    }
  }
}

void processPlaybackStateChange(String state) {
  Serial.println("Playback state changed from " + prevState + " to " + state);
  // Do nothing if paused or disabled. 
  // Marantz will power off automatically in 30 minutes
  if (state == "play" || prevState == "not_ready") {
    // MXN10 woke up or started playing
    unsigned long deltaMinutes;
    if (disabledAtMillis != 0) {
      deltaMinutes = (millis() - disabledAtMillis) / (1000 * 60);
    } else {
      deltaMinutes = 999;
    }
    
    Serial.print(deltaMinutes);
    Serial.println(" minute(s) passed since the last detected status change");
    if (deltaMinutes > 30) {
      // Marantz automatically powered off after 30 minutes of no signal. 
      // There is currently no way to detect the current state of the Marantz.
      // With assimption that it was not enabled manually, enabling it here
      // If this is incorrect, will be changed manually.
      for (int i = 5; 0 <= i; i--) {
        sendAmpButton("PowerOn");
        delay(250);
      }
      delay(2000);
      for (int i = 5; 0 <= i; i--) {
        sendAmpButton("InputCD");
        delay(250);
      }
    }
  } else if (prevState == "play") {
    // Was playing, but stopped
    disabledAtMillis = millis();
  }
  prevState = state;
}

void processEvent(String payload) {
  if (payload.indexOf("play_state") > 0) {
    String state = getPlaybackState(payload);
    if (state != prevState) {
      processPlaybackStateChange(state);
    }
  } else {
    Serial.printf("[WS] Received: %s\n", payload);
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("[WS] Connected!");
      webSocket.sendTXT("{\"path\":\"/zone/play_state\", \"params\": { \"update\": 1}}");
      break;
    case WStype_TEXT:
      processEvent(String((char*)payload));
      break;
    case WStype_ERROR:
      Serial.println("[WS] Error!");
      break;
    case WStype_PONG:
      break;
    default:
      Serial.println("[WS] Type unknown: " + String(type));
      break;
  }
}
