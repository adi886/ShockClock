#include <Wire.h>
#include "rgb_lcd.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
WiFiServer server(80);
#else
#include <WiFi.h>
WiFiServer server(80);
#endif

// --- LCD ---
rgb_lcd lcd;

// --- PIN SETUP ---
const int lightSensor = A1;
const int knob        = A0;
const int buzzer      = 2;

// --- WIFI ---
const char* ssid     = "Eddys";
const char* password = "eddyshaw";

// --- ALARM VARIABLES ---
int  correctCode  = 0;
int  enteredCode  = 0;
bool alarmOn      = false;
bool armed        = false;   // must be armed before light sensor can trigger

// --- NON-BLOCKING BUZZER ---
unsigned long lastBeep     = 0;
unsigned long lastIncrease = 0;
int beepInterval = 800;

// =============================================================
// API CONTRACT
//   GET /arm      -> arms the light-sensor trigger
//   GET /disarm   -> disarms the light-sensor trigger
//   GET /trigger  -> starts the alarm immediately
//   GET /disable  -> stops the alarm
//   GET /status   -> JSON: { "light", "entered", "alarmOn", "armed" }
//                    NOTE: correctCode intentionally NOT exposed
// =============================================================

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setRGB(0, 120, 255);
  randomSeed(analogRead(A2));
  pinMode(buzzer, OUTPUT);

  Serial.println("Connecting to WiFi...");
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  server.begin();

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.print("IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  lcd.clear();
  lcd.print("System Ready");
}

void loop() {
  int lightValue = analogRead(lightSensor);

  // Only trigger from light sensor when armed
  if (armed && lightValue < 500 && !alarmOn) {
    startAlarm();
  }

  if (alarmOn) {
    readKnob();
    updateBuzzer();   // non-blocking -- no delay() here
    checkCode();
  }

  handleWebClient();
}

// ------------------------------------------------------------
// START ALARM
// ------------------------------------------------------------
void startAlarm() {
  if (alarmOn) return;                 // guard: don't restart mid-alarm
  alarmOn      = true;
  correctCode  = random(100, 999);
  beepInterval = 800;                  // reset escalation each trigger
  lastIncrease = millis();
  lastBeep     = millis();

  lcd.clear();
  lcd.setRGB(255, 0, 0);
  lcd.print("ALARM TRIGGERED");
  delay(1500);
  lcd.clear();
  lcd.print("Code: ");
  lcd.print(correctCode);
  lcd.setCursor(0, 1);
  lcd.print("Enter: ");
}

// ------------------------------------------------------------
// DISABLE ALARM  (guarded -- safe to call from anywhere)
// ------------------------------------------------------------
void disableAlarm() {
  if (!alarmOn) return;                // guard: only run if alarm is active
  alarmOn = false;
  noTone(buzzer);

  lcd.clear();
  lcd.setRGB(0, 255, 0);
  lcd.print("ALARM DISABLED");
  delay(2000);

  lcd.setRGB(0, 120, 255);
  lcd.clear();
  lcd.print("System Ready");
}

// ------------------------------------------------------------
// READ POTENTIOMETER
// ------------------------------------------------------------
void readKnob() {
  int knobValue = analogRead(knob);
  enteredCode = map(knobValue, 0, 1023, 0, 999);

  lcd.setCursor(7, 1);
  lcd.print("    ");
  lcd.setCursor(7, 1);
  lcd.print(enteredCode);
}

// ------------------------------------------------------------
// NON-BLOCKING BUZZER  (no delay -- uses millis)
// ------------------------------------------------------------
void updateBuzzer() {
  unsigned long now = millis();

  // Beep on schedule
  if (now - lastBeep >= (unsigned long)beepInterval) {
    tone(buzzer, 1000, 100);
    lastBeep = now;
  }

  // Escalate every 10 seconds
  if (now - lastIncrease >= 10000) {
    lastIncrease = now;
    beepInterval = max(100, beepInterval - 100);
    // analogWrite(buzzer) removed -- conflicts with tone()
  }
}

// ------------------------------------------------------------
// CHECK CODE
// ------------------------------------------------------------
void checkCode() {
  if (enteredCode == correctCode) {
    disableAlarm();
  }
}

// ------------------------------------------------------------
// WEB SERVER  -- implements the API contract above
// ------------------------------------------------------------
void handleWebClient() {
  WiFiClient client = server.available();
  if (!client) return;

  String request = client.readStringUntil('\r');
  client.flush();

  // Route: /arm
  if (request.indexOf("/arm") != -1) {
    armed = true;
    lcd.clear();
    lcd.print("Armed");
    sendResponse(client, "OK");
    return;
  }

  // Route: /disarm
  if (request.indexOf("/disarm") != -1) {
    armed = false;
    lcd.clear();
    lcd.print("Disarmed");
    sendResponse(client, "OK");
    return;
  }

  // Route: /trigger
  if (request.indexOf("/trigger") != -1) {
    startAlarm();
    sendResponse(client, "OK");
    return;
  }

  // Route: /disable
  if (request.indexOf("/disable") != -1) {
    disableAlarm();
    sendResponse(client, "OK");
    return;
  }

  // Route: /status  (JSON -- correctCode intentionally excluded)
  if (request.indexOf("/status") != -1) {
    String json = "{";
    json += "\"light\":"   + String(analogRead(lightSensor)) + ",";
    json += "\"entered\":" + String(enteredCode) + ",";
    json += "\"alarmOn\":" + String(alarmOn ? "true" : "false") + ",";
    json += "\"armed\":"   + String(armed   ? "true" : "false");
    json += "}";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println(json);
    client.stop();
    return;
  }

  // Default: 404
  client.println("HTTP/1.1 404 Not Found");
  client.println();
  client.stop();
}

// ------------------------------------------------------------
// HELPER: plain-text 200 response
// ------------------------------------------------------------
void sendResponse(WiFiClient client, String body) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  client.println(body);
  client.stop();
}