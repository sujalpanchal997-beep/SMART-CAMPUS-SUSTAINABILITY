/************************************************************
 SMART ENERGY MONITOR – PROFESSIONAL DASHBOARD
************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <math.h>

/* ========== AP CONFIG ========== */
const char* ap_ssid = "ESP32_Energy_Monitor";
const char* ap_password = "12345678";

/* ========== PINS ========== */
#define ACS_PIN     34
#define RELAY_PIN   26
#define BUZZER_PIN  25

WebServer server(80);

/* ========== VARIABLES ========== */
float voltage = 230.0;
float currentA = 0.0;
float powerW = 0.0;
float energyWh = 0.0;

float dailyUnit = 0.0;
float monthlyUnit = 0.0;

float warningLimit = 1.5;
float cutLimit = 2.0;

#define UNIT_PRICE 8.0

unsigned long lastMillis = 0;
unsigned long buzzerStart = 0;
bool buzzerActive = false;

/* ========== CURRENT FUNCTION ========== */
float readCurrent()
{
  float sum = 0;
  for (int s = 0; s < 500; s++)
  {
    float adc = analogRead(ACS_PIN);
    float vout = adc * (3.3 / 4095.0);
    float instCurrent = (vout - 2.5) / 0.185;
    sum += instCurrent * instCurrent;
  }
  return abs(sqrt(sum / 500));
}

/* ========== HANDLE LIMIT SET ========== */
void handleSet()
{
  if (server.hasArg("warn"))
    warningLimit = server.arg("warn").toFloat();

  if (server.hasArg("cut"))
    cutLimit = server.arg("cut").toFloat();

  server.sendHeader("Location","/");
  server.send(303);
}

/* ========== WEB PAGE ========== */
void handleRoot()
{
  float totalBill = monthlyUnit * UNIT_PRICE;

  String html = "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<style>";
  html += "body{font-family:Arial;background:#111;color:#0f0;text-align:center}";
  html += ".row{display:flex;justify-content:center}";
  html += ".card{flex:1;margin:10px;padding:15px;background:#222;border-radius:10px}";
  html += "input{padding:5px;margin:5px}";
  html += "</style></head><body>";

  html += "<h2>⚡ Smart Energy Monitor</h2>";

  html += "<div class='row'>";
  html += "<div class='card'>Voltage<br>" + String(voltage) + " V</div>";
  html += "<div class='card'>Current<br>" + String(currentA,2) + " A</div>";
  html += "</div>";

  html += "<div class='row'>";
  html += "<div class='card'>Power<br>" + String(powerW,1) + " W</div>";
  html += "<div class='card'>Total Unit<br>" + String(dailyUnit,3) + " kWh</div>";
  html += "</div>";

  html += "<div class='card'>Today Used Unit: " + String(dailyUnit,3) + " kWh</div>";
  html += "<div class='card'>Month Used Unit: " + String(monthlyUnit,3) + " kWh</div>";
  html += "<div class='card'>Total Bill: ₹" + String(totalBill,2) + "</div>";

  html += "<form action='/set'>";
  html += "Warning Limit: <input type='number' step='0.1' name='warn' value='" + String(warningLimit) + "'><br>";
  html += "Cut Limit: <input type='number' step='0.1' name='cut' value='" + String(cutLimit) + "'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

/* ========== SETUP ========== */
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

/* ========== LOOP ========== */
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

    // Warning
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

    // Cut Power
    if (dailyUnit >= cutLimit)
    {
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}