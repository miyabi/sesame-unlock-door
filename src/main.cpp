#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <M5StickCPlus.h>
#include <WiFi.h>

#include <secrets.h>
// secrets.h includes definitions like below:
// #define WIFI_SSID "<Your Wi-Fi SSID>"
// #define WIFI_PW "<Your Wi-Fi password>"
// #define SESAME_API_KEY "<Your SESAME API key>"
// #define SESAME_QR_CODE "ssm://UI?t=sk&sk=*&l=1&n=*"
// #define GAS_URL "<Your script URL>"

#define DEFAULT_TEXT_SIZE 3
#define BAT_VOLTAGE_MIN 3.2
#define BAT_VOLTAGE_MAX 4.2
#define BATTERY_WIDTH 30
#define BATTERY_HEIGHT 16
#define BATTERY_CHARGE_OFFSET (3 + 4)

void clearScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, BATTERY_HEIGHT + 4);
}

void turnScreen(bool on) {
  clearScreen();
  M5.Axp.ScreenBreath(on ? 15 : 7);
}

void displayBatteryStatus() {
  float voltage = M5.Axp.GetBatVoltage();
  int percentage = round((voltage - BAT_VOLTAGE_MIN) / (BAT_VOLTAGE_MAX - BAT_VOLTAGE_MIN) * 100);
  if (percentage < 0) {
    percentage = 0;
  } else if (percentage > 100) {
    percentage = 100;
  }

  uint16_t color;
  if (percentage < 30) {
    color = RED;
  } else if (percentage < 50) {
    color = YELLOW;
  } else {
    color = GREEN;
  }

  M5.Lcd.drawRoundRect(0, 0, BATTERY_WIDTH, BATTERY_HEIGHT, 1, color);
  M5.Lcd.fillRect(BATTERY_WIDTH + 1, BATTERY_HEIGHT / 2 - BATTERY_HEIGHT / 3 / 2, 2, BATTERY_HEIGHT / 3, color);
  M5.Lcd.fillRect(2, 2, (BATTERY_WIDTH - 4) * percentage / 100, BATTERY_HEIGHT - 4, color);

  float current = M5.Axp.GetBatCurrent();
  if (current > 0) {
    // Charging
    M5.Lcd.fillTriangle(
      BATTERY_WIDTH + BATTERY_CHARGE_OFFSET + 8, 0                 ,
      BATTERY_WIDTH + BATTERY_CHARGE_OFFSET    , BATTERY_HEIGHT / 2,
      BATTERY_WIDTH + BATTERY_CHARGE_OFFSET + 4, BATTERY_HEIGHT / 2,
      color
    );
    M5.Lcd.fillTriangle(
      BATTERY_WIDTH + BATTERY_CHARGE_OFFSET + 1, BATTERY_HEIGHT     - 1,
      BATTERY_WIDTH + BATTERY_CHARGE_OFFSET + 9, BATTERY_HEIGHT / 2 - 1,
      BATTERY_WIDTH + BATTERY_CHARGE_OFFSET + 5, BATTERY_HEIGHT / 2 - 1,
      color
    );
  }

  // M5.Lcd.setTextSize(2);
  // M5.Lcd.printf("%.2f/%.2f\n", voltage, current);
  // M5.Lcd.setTextSize(DEFAULT_TEXT_SIZE);
}

void setupWiFi() {
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("CONNECTING");

  WiFi.begin(WIFI_SSID, WIFI_PW);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");
  }

  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.print("\nCONNECTED!");
}

String request(String url, String method = "GET", String payload = "") {
  HTTPClient http;
  http.begin(url);

  const char *headerKeys[] = { "Location" };
  http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));

  int code;
  if (method.equalsIgnoreCase("POST")) {
    http.addHeader("Content-Type", "application/json");
    code = http.POST(payload);
  } else {
    code = http.GET();
  }

  String location = http.header("Location");
  String response = http.getString();
  http.end();

  // M5.Lcd.printf("%d %s\n", code, response);

  if (code == HTTP_CODE_OK ) {
    return response;
  } else if (code == HTTP_CODE_FOUND) {
    // Redirect
    return request(location);
  }

  return response;
}

void onPressButtonA() {
  turnScreen(true);
  displayBatteryStatus();

  // Unlock
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("UNLOCKING...\n");

  StaticJsonDocument<256> json;
  json["command"] = "unlock";
  json["history"] = "M5StickC Plus";
  json["apiKey"] = SESAME_API_KEY;
  json["qrCode"] = SESAME_QR_CODE;

  String payload;
  serializeJson(json, payload);

  String response = request(GAS_URL, "POST", payload);

  if (response.equals("SUCCEEDED")) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.print("UNLOCKED!");
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.printf("! %s\n", response);
  }

  delay(3000);
  turnScreen(false);
}

void onPressButtonB() {
  // Check battery status
  turnScreen(true);
  displayBatteryStatus();
  delay(3000);
  turnScreen(false);
}

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(DEFAULT_TEXT_SIZE);

  turnScreen(true);
  displayBatteryStatus();

  setupWiFi();

  delay(3000);
  turnScreen(false);
}

void loop() {
  // put your main code here, to run repeatedly:
  M5.update();

  if (M5.BtnA.wasPressed()) {
    onPressButtonA();
  }

  if (M5.BtnB.wasPressed()) {
    onPressButtonB();
  }

  delay(1);
}
