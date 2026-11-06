
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DHT.h>

// WiFi and ThingSpeak
const char* ssid = "zantechguest";
const char* password = "55555555";
const char* apiKey = "GN4PHNL54WPHV2NZ";
const char* server = "https://api.thingspeak.com/update";

// DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const unsigned long interval = 20000; // 20 seconds
unsigned long lastSend = 0;
unsigned long lastWiFi = 0;
bool firstSend = true;

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("ESP32 DHT11 ThingSpeak Monitor");
  Serial.println("Connecting to WiFi...");
}

void loop() {
  unsigned long now = millis();

  // Reconnect WiFi when disconnected
  if (WiFi.status() != WL_CONNECTED &&
      now - lastWiFi >= 10000) {
    lastWiFi = now;
    WiFi.reconnect();
    Serial.println("Retrying WiFi...");
  }

  // Read sensor and upload every 20 seconds
  if (firstSend || now - lastSend >= interval) {
    firstSend = false;
    lastSend = now;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("ERROR: DHT11 reading failed!");
      return;
    }

    Serial.printf("Temperature: %.1f C | Humidity: %.1f %%\n",
                  temp, hum);

    if (WiFi.status() == WL_CONNECTED) {
      sendToThingSpeak(temp, hum);
    } else {
      Serial.println("WiFi disconnected. Upload skipped.");
    }
  }
}

void sendToThingSpeak(float temp, float hum) {
  WiFiClientSecure client;
  client.setInsecure(); // Demo only: skips certificate validation

  HTTPClient http;
  http.setTimeout(5000);

  if (!http.begin(client, server)) {
    Serial.println("HTTP connection setup failed!");
    return;
  }

  http.addHeader("Content-Type",
                 "application/x-www-form-urlencoded");

  String postData = "api_key=" + String(apiKey);
  postData += "&field1=" + String(temp, 1);
  postData += "&field2=" + String(hum, 1);

  int httpCode = http.POST(postData);

  if (httpCode == 200) {
    String response = http.getString();
    if (response.toInt() > 0) {
      Serial.println("ThingSpeak upload SUCCESS!");
      Serial.println("Entry ID: " + response);
    } else {
      Serial.println("ThingSpeak rejected update: " + response);
    }
  } else {
    Serial.printf("HTTP ERROR: %d\n", httpCode);
  }

  http.end();
}
