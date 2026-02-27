/************************************************************
 SMART ENERGY MONITOR
 ESP32 + ACS712 + WIFI ACCESS POINT + WEB DASHBOARD
 230V FIXED VOLTAGE
************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

/* ================== ACCESS POINT ================== */
const char* ap_ssid = "ESP32_Energy_Monitor";
const char* ap_password = "12345678";

/* ================== PIN CONFIG ================== */
#define ACS_PIN 34

WebServer server(80);

/* ================== ENERGY VARIABLES ================== */
float voltage = 230.0;      // Fixed voltage
float currentA = 0.0;
float powerW = 0.0;
float energyWh = 0.0;
float dailyUnit = 0.0;

unsigned long lastMillis = 0;

/* ================== READ CURRENT ================== */
float readCurrent()
{
  const int samples = 500;
  float sum = 0.0;

  for (int s = 0; s < samples; s++)
  {
    float adc = analogRead(ACS_PIN);
    float vout = adc * (3.3 / 4095.0);

    // ACS712-5A sensitivity = 0.185 V/A
    float instCurrent = (vout - 2.5) / 0.185;

    sum += instCurrent * instCurrent;
  }

  float rms = sqrt(sum / samples);
  return abs(rms);
}

/* ================== WEB PAGE ================== */
void handleRoot()
{
  String html = "<!DOCTYPE html><html>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;background:#0f172a;color:#e5e7eb}";
  html += ".card{background:#111827;padding:20px;margin:20px;border-radius:12px}";
  html += "h1{color:#38bdf8}";
  html += "</style></head><body>";

  html += "<h1>⚡ Smart Energy Monitor</h1>";

  html += "<div class='card'>";
  html += "<h2>Voltage : 230 V</h2>";
  html += "<h2>Current : " + String(currentA, 2) + " A</h2>";
  html += "<h2>Power   : " + String(powerW, 1) + " W</h2>";
  html += "<h2>Today Unit : " + String(dailyUnit, 4) + " kWh</h2>";
  html += "</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

/* ================== SETUP ================== */
void setup()
{
  Serial.begin(115200);

  // Start ESP32 Access Point
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();

  Serial.println("Web server started");
}

/* ================== LOOP ================== */
void loop()
{
  server.handleClient();

  if (millis() - lastMillis >= 1000)   // every 1 second
  {
    lastMillis = millis();

    currentA = readCurrent();
    powerW = voltage * currentA;

    energyWh += powerW / 3600.0;       // Wh accumulation
    dailyUnit = energyWh / 1000.0;     // kWh
  }
}