#include <FastLED.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "esp_wifi.h"

WiFiServer wifiServer(3030);

IPAddress local_IP(192, 168, 50, 184);
IPAddress gateway(192, 168, 50, 1);
IPAddress subnet(255, 255, 255, 0);

const char* ssid = "<example>";
const char* password = "<example>";

FASTLED_USING_NAMESPACE

#define DATA_PIN     22
#define LED_TYPE     WS2811
#define COLOR_ORDER  RGB
#define BRIGHTNESS   255
#define TARGET_DELAY 15

#define NUM_LEDS    200
CRGB leds[NUM_LEDS];

void show_leds(int pre_delay=0) {
  // There seems to be a weird timing bug with my WS2811 strips
  // Delaying for at least 1 ms after showing the lights fixes it
  pre_delay = max(pre_delay, 1);
  delay(pre_delay);
  FastLED.show();
}

void setup_wifi() {
  esp_wifi_set_ps(WIFI_PS_NONE);
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 50) {
    delay(200);
    // Flash first LED blue as wifi status
    if (leds[0] == CRGB(CRGB::Blue)) {
      leds[0] = CRGB::Black;
      show_leds();
    } else {
      leds[0] = CRGB::Blue;
      show_leds();
    }
  }
  leds[0] = CRGB::Green;
  show_leds();
  if (WiFi.status() == WL_CONNECTED) {
    // Success!
    Serial.println("Connected to wifi!");
    leds[0] = CRGB::Blue;
    show_leds();
    delay(200);
  } else {
    // Wifi connection failed! Let's sleep for a bit and hope that rebooting will help
    Serial.println("Wifi connection failed!");
    leds[0] = CRGB::Red;
    show_leds();
    delay(5000);
    ESP.restart();
  }
}

void setup_OTA() {
  ArduinoOTA
    .onStart([]() {
    })
    .onEnd([]() {
    })
    .onError([](ota_error_t error) {
      leds[0] = CRGB::Red;
      show_leds();
      delay(5000);
      ESP.restart();
    });
    ArduinoOTA.begin();
    wifiServer.begin();

    leds[0] = CRGB::Green;
    show_leds();
}

void setup() {
  Serial.begin(19200);
  
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(UncorrectedColor);
  FastLED.setBrightness(BRIGHTNESS);

  setup_wifi();
  setup_OTA();
}

void loop()
{
  // TODO: Reorganize the main loop into a state machine

  ArduinoOTA.handle();

  // Fade leds since we're not running a show
  leds[0].fadeToBlackBy(1);
  show_leds();
  
  WiFiClient client = wifiServer.available();
  if (client) {
    while (client.connected()) {
      int ledIndex = 0;
      int count = 0;
      // Tell the client we're ready for data
      client.println(); 
      int targetTime = millis()+TARGET_DELAY;
      while (client.available()) {
        client.read((uint8_t*)leds, NUM_LEDS*3);
        show_leds(targetTime-millis());
        targetTime += TARGET_DELAY;
      }
    }
 
    client.stop();
    Serial.println("Client disconnected");
  }
}
