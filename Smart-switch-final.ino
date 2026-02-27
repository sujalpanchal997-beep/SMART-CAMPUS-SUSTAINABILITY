#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

/* ========= WIFI ACCESS POINT ========= */
const char* ssid = "sujal's led";
const char* password = "Sujal@1326";

/* ========= PINS ========= */
#define RELAY1 D1   // LED1
#define RELAY2 D2   // LED2 (PIR)
#define RELAY3 D5   // LED3
#define RELAY4 D6   // FAN
#define PIR_PIN D7  // PIR Sensor

ESP8266WebServer server(80);

/* ========= STATES ========= */
bool led1 = false;
bool led3 = false;
bool fan  = false;

unsigned long t1 = 0, t3 = 0, tf = 0;
unsigned long d1 = 0, d3 = 0, df = 0;

/* ========= SINGLE PAGE UI ========= */
String card(String name, String url, bool state) {
  String s = "<div class='card'>";
  s += "<h2>" + name + "</h2>";
  s += "<p>Status: ";
  s += state ? "<span class='on'>ON</span>" : "<span class='off'>OFF</span>";
  s += "</p>";
  s += "<form action='/" + url + "/on'>";
  s += "<input type='number' name='sec' placeholder='Timer in seconds (optional)'>";
  s += "<button class='onbtn'>ON</button>";
  s += "</form>";
  s += "<a href='/" + url + "/off'><button class='offbtn'>OFF</button></a>";
  s += "</div>";
  return s;
}

String webpage() {
  String p;
  p += "<!DOCTYPE html><html><head>";
  p += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  p += "<title>Smart Electricity Saving System</title>";
  p += "<style>";
  p += "body{font-family:Arial;background:#f2f2f2;margin:0;text-align:center}";
  p += "h1{background:#1e1e2f;color:#fff;padding:15px;margin:0}";
  p += ".card{background:#fff;margin:15px;padding:20px;border-radius:10px;";
  p += "box-shadow:0 4px 8px rgba(0,0,0,0.2)}";
  p += ".on{color:green;font-weight:bold}";
  p += ".off{color:red;font-weight:bold}";
  p += "input{padding:8px;width:220px;margin:5px}";
  p += "button{padding:10px 25px;margin:5px;border:none;border-radius:5px}";
  p += ".onbtn{background:green;color:white}";
  p += ".offbtn{background:red;color:white}";
  p += "</style></head><body>";

  p += "<h1>SMART ELECTRICITY SAVING SYSTEM</h1>";

  p += card("LED 1", "led1", led1);

  p += "<div class='card'>";
  p += "<h2>LED 2 (PIR)</h2>";
  p += "<p>Motion detected: ON</p>";
  p += "<p>No motion: OFF</p>";
  p += "</div>";

  p += card("LED 3", "led3", led3);
  p += card("FAN", "fan", fan);

  p += "</body></html>";
  return p;
}

void handleRoot() {
  server.send(200, "text/html", webpage());
}

void setup() {
  Serial.begin(9600);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(RELAY3, HIGH);
  digitalWrite(RELAY4, HIGH);

  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);

  server.on("/led1/on", [](){
    d1 = server.arg("sec").toInt() * 1000;
    led1 = true;
    digitalWrite(RELAY1, LOW);
    t1 = millis();
    handleRoot();
  });
  server.on("/led1/off", [](){
    led1 = false;
    digitalWrite(RELAY1, HIGH);
    handleRoot();
  });

  server.on("/led3/on", [](){
    d3 = server.arg("sec").toInt() * 1000;
    led3 = true;
    digitalWrite(RELAY3, LOW);
    t3 = millis();
    handleRoot();
  });
  server.on("/led3/off", [](){
    led3 = false;
    digitalWrite(RELAY3, HIGH);
    handleRoot();
  });

  server.on("/fan/on", [](){
    df = server.arg("sec").toInt() * 1000;
    fan = true;
    digitalWrite(RELAY4, LOW);
    tf = millis();
    handleRoot();
  });
  server.on("/fan/off", [](){
    fan = false;
    digitalWrite(RELAY4, HIGH);
    handleRoot();
  });

  server.begin();
}

void loop() {
  server.handleClient();

  /* PIR REAL-TIME CONTROL */
  if (digitalRead(PIR_PIN) == HIGH) {
    digitalWrite(RELAY2, LOW);
  } else {
    digitalWrite(RELAY2, HIGH);
  }

  /* OPTIONAL AUTO OFF */
  if (led1 && d1 > 0 && millis() - t1 > d1) {
    led1 = false;
    digitalWrite(RELAY1, HIGH);
  }
  if (led3 && d3 > 0 && millis() - t3 > d3) {
    led3 = false;
    digitalWrite(RELAY3, HIGH);
  }
  if (fan && df > 0 && millis() - tf > df) {
    fan = false;
    digitalWrite(RELAY4, HIGH);
  }
}