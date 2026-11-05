/*
  ideaspark ESP8266 0.96" OLED  <->  Handy (per WLAN-Hotspot, ohne App)

  Ablauf:
   1. Board flashen (Board: "NodeMCU 1.0 (ESP-12E Module)", Upload 115200).
   2. Handy mit WLAN "ESP8266-OLED" verbinden (Passwort unten).
   3. Im Handy-Browser http://192.168.4.1 öffnen.
   4. Text senden -> erscheint auf dem OLED; LED an/aus schalten.

  Benötigte Bibliotheken (Bibliotheksverwalter):
   - Adafruit SSD1306
   - Adafruit GFX Library
  ESP8266WiFi / ESP8266WebServer gehören zum ESP8266-Boardpaket.

  ACHTUNG: I2C-Pins (SDA=14, SCL=12) und Adresse 0x3C sind der übliche Wert
  für dieses Board, aber von mir nicht am Gerät getestet. Bei schwarzem
  Display zuerst diese Werte bzw. die Pins in der Board-Beschreibung prüfen.
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- Einstellungen ----
const char* AP_SSID = "ESP8266-OLED";
const char* AP_PASS = "12345678";      // mindestens 8 Zeichen
const uint8_t PIN_SDA = 14;            // GPIO14 (D5)
const uint8_t PIN_SCL = 12;            // GPIO12 (D6)
const uint8_t OLED_ADDR = 0x3C;
const uint8_t LED_PIN = LED_BUILTIN;   // GPIO2, LOW = an

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer server(80);

String lastText = "Bereit";
bool ledOn = false;

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP8266</title>
<style>
 body{font-family:sans-serif;max-width:480px;margin:0 auto;padding:16px}
 input,button{width:100%;font-size:18px;padding:12px;margin:6px 0;box-sizing:border-box}
 .row{display:flex;gap:8px}
</style></head><body>
<h2>ESP8266 Steuerung</h2>
<input id="t" maxlength="40" placeholder="Text für das Display">
<button onclick="send()">Text anzeigen</button>
<div class="row">
 <button onclick="led(1)">LED an</button>
 <button onclick="led(0)">LED aus</button>
</div>
<p id="s">Status: -</p>
<script>
async function call(u){
  try{const r=await fetch(u);document.getElementById('s').textContent='Status: '+await r.text();}
  catch(e){document.getElementById('s').textContent='Status: keine Verbindung';}
}
function send(){call('/text?v='+encodeURIComponent(document.getElementById('t').value));}
function led(x){call('/led?v='+x);}
call('/status');
</script></body></html>
)HTML";

void draw() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("WLAN: ")); display.println(AP_SSID);
  display.print(F("IP:   ")); display.println(WiFi.softAPIP());
  display.print(F("LED:  ")); display.println(ledOn ? F("an") : F("aus"));
  display.drawFastHLine(0, 30, 128, SSD1306_WHITE);
  display.setCursor(0, 36);
  display.setTextSize(1);
  display.print(lastText);
  display.display();
}

void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_PIN, on ? LOW : HIGH);   // aktiv-low
}

void handleRoot()   { server.send_P(200, "text/html; charset=utf-8", PAGE); }
void handleStatus() { server.send(200, "text/plain", String("LED ") + (ledOn ? "an" : "aus") + ", Text: " + lastText); }

void handleText() {
  lastText = server.arg("v");
  lastText.trim();
  if (lastText.length() == 0) lastText = "(leer)";
  draw();
  server.send(200, "text/plain", "Text gesendet");
}

void handleLed() {
  setLed(server.arg("v") == "1");
  draw();
  server.send(200, "text/plain", ledOn ? "LED an" : "LED aus");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  setLed(false);

  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED nicht gefunden - Pins/Adresse pruefen"));
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print(F("Hotspot aktiv, IP: "));
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/text", handleText);
  server.on("/led", handleLed);
  server.begin();
  draw();
}

void loop() {
  server.handleClient();
}
