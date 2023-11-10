#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

float temp;
float hum;
unsigned long count = 1;
const char* ssid = "IR_Lab";
const char* password = "ccsadmin";
const char* server = "http://192.168.0.74:3000/senser";
WiFiClient client;
HTTPClient http;
DHT dht11(D4, DHT11);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 0);

void ReadDHT11() {
  temp = dht11.readTemperature();
  hum = dht11.readHumidity();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  dht11.begin();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  timeClient.begin();
}

void loop() {
  static unsigned long lastTime = 0;
  unsigned long timerDelay = 15000;

  if ((millis() - lastTime) > timerDelay) {
    ReadDHT11();
    
    // Update the time from the NTP server
    timeClient.update();
    
    // Get the current date and time
    String currentDateTime = getFormattedDateTime();

    if (isnan(hum) || isnan(temp)) {
      Serial.println("Failed");
    } else {
      Serial.printf("Humidity: %.2f%%\n", hum);
      Serial.printf("Temperature: %.2f°C\n", temp);

      StaticJsonDocument<200> jsonDocument;
      jsonDocument["humidity"] = hum;
      jsonDocument["temperature"] = temp;
      jsonDocument["dateTime"] = currentDateTime; // Add date and time to JSON

      String jsonData;
      serializeJson(jsonDocument, jsonData);

      http.begin(client, server);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        String payload = http.getString();
        Serial.println("Returned payload:");
        Serial.println(payload);
        count += 1;
      } else {
        Serial.println("Error on sending POST: " + String(httpResponseCode));
      }
      http.end();
    }
    lastTime = millis();
  }
}

String getFormattedDateTime() {
  // Get the current date and time
  time_t now = timeClient.getEpochTime() + 7 * 3600; // Add 7 hours (GMT+7)
  struct tm *tmstruct = localtime(&now);

  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           tmstruct->tm_year + 1900, tmstruct->tm_mon + 1, tmstruct->tm_mday,
           tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);

  return String(buffer);
}
