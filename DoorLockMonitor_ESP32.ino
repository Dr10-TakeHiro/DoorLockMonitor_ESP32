#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "MyAPSettings.h" // Contain following 3 constants.
// const char *SSID_AP = "hogehoge";
// const char *PASSWORD_AP = "hogehoge";
// const char *IFTTT_URL = "http://maker.ifttt.com/trigger/...";

const gpio_num_t DOOR_LOCK_SW_PIN = GPIO_NUM_2;

enum DoorLockStatus { LOCKED, UNLOCKED, INVALID };
RTC_DATA_ATTR DoorLockStatus last_status =
    INVALID; // Keep last status in RTC SLOW memory.

void setup() {
  Serial.begin(115200);
  pinMode(DOOR_LOCK_SW_PIN, INPUT_PULLUP);
}
void loop() {
  Serial.printf("Last status: %s.\n",
                DoorLockStatusToString(last_status).c_str());
  DoorLockStatus current_status = GetDoorLockStatus();
  Serial.printf("Current status: %s.\n",
                DoorLockStatusToString(current_status).c_str());
  if (last_status == current_status) {
    Serial.println("Enter deep sleep mode.");
    esp_sleep_enable_ext0_wakeup(
        DOOR_LOCK_SW_PIN,
        digitalRead(DOOR_LOCK_SW_PIN) == HIGH
            ? LOW
            : HIGH); // Wake up by any change on DOOR_LOCK_SW_PIN;
    esp_deep_sleep_start();
  }
  PostIFTTT(String("Door ") + DoorLockStatusToString(current_status));
  last_status = current_status;
  delay(1000);
}

String DoorLockStatusToString(DoorLockStatus status) {
  switch (status) {
  case LOCKED:
    return "Locked";
  case UNLOCKED:
    return "Unlocked";
  default:
    return "Invalid";
  }
}
DoorLockStatus GetDoorLockStatus() {
  if (digitalRead(DOOR_LOCK_SW_PIN) == HIGH) {
    return LOCKED;
  }
  return UNLOCKED;
}

void ConnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_AP, PASSWORD_AP);
  for (int retry_count = 0; retry_count < 10; retry_count++) {
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
}
void PostIFTTT(String str) {
  ConnectWiFi();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  StaticJsonDocument<JSON_OBJECT_SIZE(1)> json_request;

  json_request["value1"] = str.c_str();

  char buffer[255];
  serializeJson(json_request, buffer, sizeof(buffer));
  Serial.println(buffer);
  HTTPClient http;

  http.begin(IFTTT_URL);
  http.addHeader("Content-Type", "application/json");
  http.POST((uint8_t *)buffer, strlen(buffer));
  http.end();
}
