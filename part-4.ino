/************************************************************
 SMART ELECTRICITY MONITORING SYSTEM
 ESP32 + ACS712 + AP + WEB + RELAY + BUZZER
************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

/* ================= ACCESS POINT ================= */
const char* ap_ssid = "ESP32_Energy_Monitor";
const char* ap_password = "12345678";

/* ================= PINS ================= */
#define ACS_PIN     34
#define RELAY_PIN   26
#define BUZZER_PIN  25

/* ================= CONFIG ================= */
#define UNIT_PRICE 8.0   // ₹ per unit

WebServer server(80);

/* ================= VARIABLES ================= */
float voltage = 230.0;
float currentA = 0.0;
float powerW = 0.0;
float energyWh = 0.0;

float dailyUnit = 0.0;
float monthlyUnit = 0.0;

float warningLimit = 1.5;
float cutLimit = 2.0;

unsigned long lastMillis = 0;
unsigned long buzzerStart = 0;
bool buzzerActive = false;

/* ================= CURRENT READ ================= */
float readCurrent()
{
  float sum = 0;
  for (int s = 0; s < 500; s++)
  {
    float adc = analogRead(ACS_PIN);
    float vout = adc * (3.3 / 4095.0);
    float instCurrent = (vout - 2.5) / 0.185; // ACS712 5A
    sum += instCurrent * instCurrent;
  }
  return abs(sqrt(sum / 500));
}

/* ================= SET LIMITS ================= */
void handleSet()
{
  if (server.hasArg("warn"))
    warningLimit = server.arg("warn").toFloat();

  if (server.hasArg("cut"))
    cutLimit = server.arg("cut").toFloat();

  server.sendHeader("Location", "/");
  server.send(303);
}

/* ================= WEB PAGE ================= */
void handleRoot()
{
  float totalBill = monthlyUnit * UNIT_PRICE;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<style>";

  html += "body{font-family:Segoe UI,Arial;background:#0f172a;color:#e5e7eb;margin:0}";
  html += "h1{background:#020617;padding:15px;margin:0;font-size:20px}";
  html += ".container{padding:15px}";
  html += ".row{display:flex;gap:10px;margin-bottom:10px}";
  html += ".card{flex:1;background:#111827;padding:15px;border-radius:10px}";
  html += ".label{font-size:13px;color:#94a3b8}";
  html += ".value{font-size:20px;font-weight:bold}";
  html += ".full{margin-bottom:10px}";
  html += "input{width:120px;padding:6px;margin:5px}";
  html += "button{padding:8px 20px;background:#2563eb;color:white;border:none;border-radius:6px}";
  html += "</style></head><body>";

  html += "<h1>SMART ELECTRICITY MONITORING SYSTEM</h1>";
  html += "<div class='container'>";

  html += "<div class='row'>";
  html += "<div class='card'><div class='label'>Voltage</div><div class='value'>230 V</div></div>";
  html += "<div class='card'><div class='label'>Current</div><div class='value'>" + String(currentA,2) + " A</div></div>";
  html += "</div>";

  html += "<div class='row'>";
  html += "<div class='card'><div class='label'>Power</div><div class='value'>" + String(powerW,1) + " W</div></div>";
  html += "<div class='card'><div class='label'>Live Unit</div><div class='value'>" + String(dailyUnit,3) + " kWh</div></div>";
  html += "</div>";

  html += "<div class='card full'><div class='label'>Today Used Unit</div><div class='value'>" + String(dailyUnit,3) + " kWh</div></div>";
  html += "<div class='card full'><div class='label'>Monthly Used Unit</div><div class='value'>" + String(monthlyUnit,3) + " kWh</div></div>";
  html += "<div class='card full'><div class='label'>Total Bill (₹8 / unit)</div><div class='value'>₹ " + String(totalBill,2) + "</div></div>";

  html += "<div class='card full'>";
  html += "<form action='/set'>";
  html += "Warning Limit (kWh): <input type='number' step='0.1' name='warn' value='" + String(warningLimit) + "'><br>";
  html += "Cut Limit (kWh): <input type='number' step='0.1' name='cut' value='" + String(cutLimit) + "'><br>";
  html += "<button type='submit'>Save Settings</button>";
  html += "</form></div>";

  html += "</div></body></html>";

  server.send(200, "text/html", html);
}

/* ================= SETUP ================= */
void setup()
{
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.softAP(ap_ssid, ap_password);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

/* ================= LOOP ================= */
void loop()
{
  server.handleClient();

  if (millis() - lastMillis >= 1000)
  {
    lastMillis = millis();

    currentA = readCurrent();
    powerW = voltage * currentA;

    energyWh += powerW / 3600.0;
    dailyUnit = energyWh / 1000.0;
    monthlyUnit += powerW / 3600000.0;

    if (dailyUnit >= warningLimit && !buzzerActive)
    {
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerStart = millis();
      buzzerActive = true;
    }

    if (buzzerActive && millis() - buzzerStart >= 10000)
    {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
    }

    if (dailyUnit >= cutLimit)
    {
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}