/*
  ESP32  <->  Handy per Bluetooth Low Energy (BLE)

  Nutzt den "Nordic UART Service" (NUS). Dafür gibt es fertige Handy-Apps:
   - Android/iOS: "Serial Bluetooth Terminal" (BLE-Modus) oder "nRF Connect"
   - Android/iOS: "LightBlue"
  Verbinden mit dem Gerät "ESP32-BLE" und Text senden.

  Befehle (Text, Zeilenende egal):
   LED1  -> LED an
   LED0  -> LED aus
   STATUS-> Status per Notify zurückschicken
   alles andere -> wird als Text auf dem OLED angezeigt (falls vorhanden)

  Bibliotheken: ESP32-Boardpaket (enthält BLE), optional für das OLED:
   Adafruit SSD1306 + Adafruit GFX. Ohne OLED: USE_OLED auf 0 setzen.

  ACHTUNG: Nicht auf Hardware getestet. OLED-Pins/Adresse sind Annahmen und
  hängen vom konkreten Board ab - bitte an dein Board anpassen.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define USE_OLED 1
#if USE_OLED
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  const uint8_t PIN_SDA = 5;        // anpassen!
  const uint8_t PIN_SCL = 4;        // anpassen!
  const uint8_t OLED_ADDR = 0x3C;
  Adafruit_SSD1306 display(128, 64, &Wire, -1);
#endif

const char* DEVICE_NAME = "ESP32-BLE";
const uint8_t LED_PIN = 2;          // Onboard-LED, bei manchen Boards anders
const bool LED_ACTIVE_LOW = false;

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Handy -> ESP32
#define TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 -> Handy

BLEServer* server = nullptr;
BLECharacteristic* txChar = nullptr;
volatile bool connected = false;
volatile bool restartAdv = false;
bool ledOn = false;
String lastText = "Bereit";

void setLed(bool on) {
  ledOn = on;
  digitalWrite(LED_PIN, (on != LED_ACTIVE_LOW) ? HIGH : LOW);
}

void draw() {
#if USE_OLED
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(DEVICE_NAME);
  display.print(F("BLE: ")); display.println(connected ? F("verbunden") : F("wartet"));
  display.print(F("LED: ")); display.println(ledOn ? F("an") : F("aus"));
  display.drawFastHLine(0, 30, 128, SSD1306_WHITE);
  display.setCursor(0, 36);
  display.print(lastText);
  display.display();
#endif
}

void notify(const String& s) {
  if (!connected || !txChar) return;
  txChar->setValue((uint8_t*)s.c_str(), s.length());
  txChar->notify();
}

class ServerCb : public BLEServerCallbacks {
  void onConnect(BLEServer*) override { connected = true; }
  void onDisconnect(BLEServer*) override { connected = false; restartAdv = true; }
};

class RxCb : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String s = String(c->getValue().c_str());   // passt zu core 2.x und 3.x
    s.trim();
    if (s.length() == 0) return;
    if (s.equalsIgnoreCase("LED1"))        { setLed(true);  notify("LED an\n"); }
    else if (s.equalsIgnoreCase("LED0"))   { setLed(false); notify("LED aus\n"); }
    else if (s.equalsIgnoreCase("STATUS")) { notify(String("LED ") + (ledOn ? "an" : "aus") + ", Text: " + lastText + "\n"); }
    else                                   { lastText = s;  notify("OK: " + s + "\n"); }
    draw();
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  setLed(false);

#if USE_OLED
  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) Serial.println(F("OLED nicht gefunden"));
#endif

  BLEDevice::init(DEVICE_NAME);
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCb());

  BLEService* svc = server->createService(SERVICE_UUID);
  txChar = svc->createCharacteristic(TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  txChar->addDescriptor(new BLE2902());
  BLECharacteristic* rx = svc->createCharacteristic(
      RX_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx->setCallbacks(new RxCb());
  svc->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println(F("BLE gestartet, Name: ESP32-BLE"));
  draw();
}

void loop() {
  static bool lastConn = false;
  if (restartAdv) {                 // nach Trennung wieder sichtbar werden
    delay(300);
    BLEDevice::startAdvertising();
    restartAdv = false;
  }
  if (lastConn != connected) { lastConn = connected; draw(); }
  delay(20);
}
