/************************************************************
 SMART ELECTRICITY MONITORING SYSTEM
 ESP32 + ACS712 + AP + RELAY + BUZZER
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

#define UNIT_PRICE 8.0   // Rs per unit

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
    float instCurrent = (vout - 2.5) / 0.185;   // ACS712 5A
    sum += instCurrent * instCurrent;
  }
  return abs(sqrt(sum / 500));
}

/* ================= HANDLE LIMIT SET ================= */
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
  html += "<meta http-equiv='refresh' content='15'>";

  html += "<style>";
  html += "body{margin:0;font-family:Poppins,Segoe UI,Arial;";
  html += "background:linear-gradient(135deg,#0f2027,#203a43,#2c5364);";
  html += "color:white;text-align:center}";
  html += "h1{padding:20px 0;margin:0;font-size:22px;letter-spacing:1px}";
  html += ".container{max-width:750px;margin:auto;padding:20px}";
  html += ".row{display:flex;gap:15px;margin-bottom:15px}";
  html += ".card{flex:1;padding:20px;border-radius:15px;";
  html += "background:rgba(255,255,255,0.08);";
  html += "backdrop-filter:blur(10px);";
  html += "box-shadow:0 8px 20px rgba(0,0,0,0.3)}";
  html += ".label{font-size:14px;color:#cbd5e1}";
  html += ".value{font-size:26px;font-weight:bold;margin-top:8px}";
  html += ".full{margin-bottom:15px}";
  html += ".formCard{padding:20px;border-radius:15px;";
  html += "background:rgba(255,255,255,0.1);";
  html += "box-shadow:0 8px 20px rgba(0,0,0,0.3)}";
  html += "input{width:160px;padding:8px;border-radius:8px;";
  html += "border:none;margin:8px;text-align:center}";
  html += "button{padding:10px 25px;border:none;border-radius:8px;";
  html += "background:#38bdf8;color:black;font-weight:bold;cursor:pointer}";
  html += "</style></head><body>";

  html += "<h1>SMART ELECTRICITY MONITORING SYSTEM</h1>";
  html += "<div class='container'>";

  /* Row 1 */
  html += "<div class='row'>";
  html += "<div class='card'><div class='label'>Voltage</div><div class='value'>230 V</div></div>";
  html += "<div class='card'><div class='label'>Current</div><div class='value'>" + String(currentA,2) + " A</div></div>";
  html += "</div>";

  /* Row 2 */
  html += "<div class='row'>";
  html += "<div class='card'><div class='label'>Power</div><div class='value'>" + String(powerW,1) + " W</div></div>";
  html += "<div class='card'><div class='label'>Live Unit</div><div class='value'>" + String(dailyUnit,3) + " kWh</div></div>";
  html += "</div>";

  html += "<div class='card full'><div class='label'>Today Used Unit</div><div class='value'>" + String(dailyUnit,3) + " kWh</div></div>";
  html += "<div class='card full'><div class='label'>Monthly Used Unit</div><div class='value'>" + String(monthlyUnit,3) + " kWh</div></div>";
  html += "<div class='card full'><div class='label'>Total Bill (Rs 8 per unit)</div><div class='value'>Rs " + String(totalBill,2) + "</div></div>";

  html += "<div class='formCard'>";
  html += "<form action='/set'>";
  html += "<div>Warning Limit (kWh)</div>";
  html += "<input type='number' step='0.1' name='warn' value='" + String(warningLimit) + "'><br>";
  html += "<div>Cut Limit (kWh)</div>";
  html += "<input type='number' step='0.1' name='cut' value='" + String(cutLimit) + "'><br>";
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

  digitalWrite(RELAY_PIN, HIGH);   // Power ON
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

    /* Warning */
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

    /* Power Cut */
    if (dailyUnit >= cutLimit)
    {
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}