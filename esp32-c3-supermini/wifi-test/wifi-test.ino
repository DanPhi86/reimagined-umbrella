/*
  ESP32-C3 Super Mini – WLAN-Test / RC-Telemetrie per Smartphone

  Der ESP32-C3 spannt ein eigenes WLAN (Access Point) auf. Das Smartphone
  verbindet sich direkt damit (kein Router nötig – im Feld wichtig) und
  öffnet http://192.168.4.1 im Browser. Die Seite zeigt live:
    - Akkuspannung (Spannungsteiler an GPIO1)
    - Strom (optionaler Stromsensor an GPIO0)
    - RC-Empfängerkanal als PWM-Pulsbreite 1000–2000 µs (GPIO3)
    - WLAN-Clients, Uptime, freier Heap

  Arduino IDE:
    Board:              "ESP32C3 Dev Module"
    USB CDC On Boot:    "Enabled"   (sonst keine Ausgabe im Serial Monitor)
    Upload-Modus:       bei Problemen BOOT halten + RESET drücken

  Nur Bibliotheken aus dem ESP32-Arduino-Core (WiFi, WebServer) – keine Extras.
  Pinbelegung: siehe PINOUT.md
*/

#include <WiFi.h>
#include <WebServer.h>

// ---------------------------------------------------------------- Konfiguration
static const char *AP_SSID     = "RC-Telemetrie";
static const char *AP_PASSWORD = "12345678";   // mind. 8 Zeichen – bitte ändern!
static const uint8_t AP_CHANNEL = 1;           // WLAN-Kanal 1..13
static const uint8_t AP_MAX_CLIENTS = 2;       // wenig Clients = stabiler

// Pins (siehe PINOUT.md) – nur ADC1-Pins (GPIO0–4) für Analogwerte
static const uint8_t PIN_VBAT   = 1;   // ADC1_CH1: Akkuspannung über Teiler
static const uint8_t PIN_CURR   = 0;   // ADC1_CH0: Stromsensor (optional)
static const uint8_t PIN_RC_IN  = 3;   // RC-Empfänger Signal (PWM 1–2 ms)
static const uint8_t PIN_LED    = 8;   // Onboard-LED, active LOW

// Spannungsteiler: Akku+ -- R1 -- GPIO1 -- R2 -- GND
// Beispiel 2S-LiPo (max 8,4 V): R1 = 100 kΩ, R2 = 33 kΩ -> Faktor 4,03
// Am Pin dürfen NIE mehr als 3,3 V anliegen!
static const float VBAT_R1 = 100000.0f;
static const float VBAT_R2 = 33000.0f;

// Stromsensor (z. B. ACS712-5A mit Spannungsteiler auf 3,3 V) – an Sensor anpassen
static const float CURR_OFFSET_MV  = 1650.0f;  // Ausgangsspannung bei 0 A (am Pin)
static const float CURR_MV_PER_AMP = 185.0f;   // Empfindlichkeit (am Pin)

static const uint32_t RC_TIMEOUT_MS = 500;     // länger kein Puls -> "kein Signal"

// ---------------------------------------------------------------- Zustand
WebServer server(80);

volatile uint32_t rcRiseUs  = 0;
volatile uint16_t rcPulseUs = 0;
volatile uint32_t rcLastMs  = 0;

// ISR: kurz halten! Misst die Pulsbreite zwischen steigender und fallender Flanke.
void IRAM_ATTR onRcEdge() {
  uint32_t now = micros();
  if (digitalRead(PIN_RC_IN)) {
    rcRiseUs = now;
  } else {
    uint32_t width = now - rcRiseUs;
    if (width >= 800 && width <= 2200) {   // plausible RC-Pulsbreite
      rcPulseUs = (uint16_t)width;
      rcLastMs  = millis();
    }
  }
}

// Mittelwert über mehrere Messungen (ADC rauscht)
static float readMilliVolts(uint8_t pin) {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(pin);
  return sum / 16.0f;
}

// ---------------------------------------------------------------- Webseite
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>RC-Telemetrie</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:12px}
 h1{font-size:1.2rem;margin:0 0 12px}
 .grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
 .card{background:#222;border-radius:10px;padding:12px}
 .l{font-size:.75rem;color:#999}.v{font-size:1.8rem;font-weight:600}
 .bar{height:10px;background:#333;border-radius:5px;margin-top:8px;overflow:hidden}
 .bar>div{height:100%;width:0;background:#4caf50}
 .bad{color:#ff5252}
</style></head><body>
<h1>RC-Telemetrie <span id="st" class="l">…</span></h1>
<div class="grid">
 <div class="card"><div class="l">Akku</div><div class="v" id="vbat">–</div></div>
 <div class="card"><div class="l">Strom</div><div class="v" id="curr">–</div></div>
 <div class="card"><div class="l">RC-Kanal</div><div class="v" id="rc">–</div>
  <div class="bar"><div id="rcbar"></div></div></div>
 <div class="card"><div class="l">WLAN-Clients</div><div class="v" id="cl">–</div></div>
 <div class="card"><div class="l">Uptime</div><div class="v" id="up">–</div></div>
 <div class="card"><div class="l">Freier Heap</div><div class="v" id="heap">–</div></div>
</div>
<script>
const $=id=>document.getElementById(id);
async function tick(){
 try{
  const r=await fetch('/api',{cache:'no-store'});const d=await r.json();
  $('vbat').textContent=d.vbat.toFixed(2)+' V';
  $('curr').textContent=d.curr.toFixed(1)+' A';
  if(d.rc>0){$('rc').textContent=d.rc+' µs';$('rc').className='v';
   $('rcbar').style.width=Math.max(0,Math.min(100,(d.rc-1000)/10))+'%';}
  else{$('rc').textContent='kein Signal';$('rc').className='v bad';$('rcbar').style.width='0';}
  $('cl').textContent=d.clients;
  $('up').textContent=Math.floor(d.up/60)+' min '+(d.up%60)+' s';
  $('heap').textContent=Math.round(d.heap/1024)+' kB';
  $('st').textContent='● online';$('st').className='l';
 }catch(e){$('st').textContent='● offline';$('st').className='l bad';}
}
setInterval(tick,500);tick();
</script></body></html>)HTML";

// ---------------------------------------------------------------- Handler
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleApi() {
  float vbat = readMilliVolts(PIN_VBAT) * (VBAT_R1 + VBAT_R2) / VBAT_R2 / 1000.0f;
  float curr = (readMilliVolts(PIN_CURR) - CURR_OFFSET_MV) / CURR_MV_PER_AMP;

  uint16_t rc = 0;
  if (millis() - rcLastMs < RC_TIMEOUT_MS && rcLastMs != 0) rc = rcPulseUs;

  char buf[160];
  snprintf(buf, sizeof(buf),
           "{\"vbat\":%.2f,\"curr\":%.2f,\"rc\":%u,\"clients\":%d,\"up\":%lu,\"heap\":%lu}",
           vbat, curr, (unsigned)rc, WiFi.softAPgetStationNum(),
           (unsigned long)(millis() / 1000), (unsigned long)ESP.getFreeHeap());
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", buf);
}

// ---------------------------------------------------------------- Setup / Loop
void setup() {
  Serial.begin(115200);          // USB-CDC (GPIO18/19), GPIO20/21 bleiben frei
  delay(500);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);   // active LOW -> aus

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // Messbereich bis ca. 2,5 V am Pin

  pinMode(PIN_RC_IN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_RC_IN), onRcEdge, CHANGE);

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);          // geringere Latenz
  // Manche Super-Mini-Boards sind mit voller Sendeleistung instabil:
  // bei Verbindungsproblemen diese Zeile aktivieren.
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CLIENTS)) {
    Serial.println("FEHLER: Access Point konnte nicht gestartet werden");
  }
  Serial.print("AP '");
  Serial.print(AP_SSID);
  Serial.print("' bereit, IP: ");
  Serial.println(WiFi.softAPIP());   // normalerweise 192.168.4.1

  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.begin();
}

void loop() {
  server.handleClient();

  // LED: blinkt langsam im Leerlauf, leuchtet dauerhaft sobald ein Client verbunden ist
  static uint32_t lastToggle = 0;
  if (WiFi.softAPgetStationNum() > 0) {
    digitalWrite(PIN_LED, LOW);
  } else if (millis() - lastToggle > 1000) {
    lastToggle = millis();
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
  }
}
