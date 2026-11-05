# ESP32-C3 Super Mini – Pinout & Belegung (RC-Telemetrie)

> Hinweis: Die Anordnung der Pins auf der Platine (links/rechts) stammt aus
> gängigen Super-Mini-Pinout-Grafiken. Es gibt Nachbauten – **vor dem Verdrahten
> die Beschriftung auf deiner Platine prüfen.** Die GPIO-Funktionen selbst
> gelten für den ESP32-C3 allgemein.

## Grafik (Draufsicht, USB-C oben)

```
                  ┌───────[ USB-C ]───────┐
                  │   (GPIO18 D- / GPIO19 D+, nicht herausgeführt)
          5V  ●───┤                       ├───●  GPIO5   (A5, MISO*)
         GND  ●───┤                       ├───●  GPIO6   (MOSI*)
         3V3  ●───┤   ┌───────────────┐   ├───●  GPIO7   (SS*)
 GPIO4 (A4,SCK*) ●┤   │  ESP32-C3     │   ├───●  GPIO8   LED(blau, active LOW) • SDA • Strapping
 GPIO3 (A3)      ●┤   │  (RISC-V)     │   ├───●  GPIO9   BOOT-Taster • SCL • Strapping
 GPIO2 (A2)      ●┤   └───────────────┘   ├───●  GPIO10
 GPIO1 (A1)      ●┤  [RST]         [BOOT] ├───●  GPIO20  (RX0)
 GPIO0 (A0)      ●───┤                    ├───●  GPIO21  (TX0)
                  └───────────────────────┘
   * = Standard-Zuordnung im Arduino-Core für diese Platine (frei umlegbar)
```

## Belegung für RC-Telemetrie (wifi-test.ino)

```
  Akku+ ──[100k]──┬──[33k]── GND          Empfänger Signal ──── GPIO3
                  └────────── GPIO1       Empfänger GND   ──── GND
                                          Stromsensor OUT ──── GPIO0 (optional)
```

| Pin | Funktion im Projekt | Hinweis |
|-----|--------------------|---------|
| GPIO0 | ADC1_CH0 – Stromsensor (optional) | Ausgang nie über 3,3 V |
| GPIO1 | ADC1_CH1 – Akkuspannung über Teiler | 100 kΩ / 33 kΩ für bis 8,4 V (2S) |
| GPIO3 | RC-Empfänger PWM-Eingang (Interrupt) | Empfängersignal ist meist 3,3 V; bei 5 V-Signal Spannungsteiler/Pegelwandler |
| GPIO8 | Onboard-LED | active LOW, Strapping-Pin – nicht extern beschalten |
| GPIO9 | BOOT-Taster | Strapping-Pin – beim Booten nicht auf LOW ziehen |
| GPIO20/21 | UART0 (RX/TX) | frei nutzbar (z. B. GPS, CRSF/MAVLink), da Serial über USB-CDC läuft |
| GPIO4–7, 10 | frei | I²C/SPI/Servo-Ausgang möglich |

## Tabelle: alle Pins mit Erläuterung

| Pin | GPIO | ADC | Besonderheiten | Empfehlung |
|-----|------|-----|----------------|------------|
| 5V | – | – | USB-5V bzw. Eingang 5 V (nicht 3,3 V-tolerant!) | Versorgung, max. wenige 100 mA entnehmen |
| GND | – | – | Masse | gemeinsame Masse mit Empfänger/Sensoren |
| 3V3 | – | – | Ausgang des Onboard-Reglers | Sensoren mit geringem Strom |
| 0 | GPIO0 | ADC1_CH0 | A0; Deep-Sleep-Wakeup-fähig | Analogeingang |
| 1 | GPIO1 | ADC1_CH1 | A1; Deep-Sleep-Wakeup-fähig | Analogeingang |
| 2 | GPIO2 | ADC1_CH2 | A2; **Strapping-Pin** (Boot-Modus) | beim Start nicht auf LOW ziehen; im Zweifel meiden |
| 3 | GPIO3 | ADC1_CH3 | A3 | Analog- oder Digitaleingang |
| 4 | GPIO4 | ADC1_CH4 | A4; Standard-SCK | Analog / SPI |
| 5 | GPIO5 | ADC2_CH0 | A5; Standard-MISO | digital bevorzugen (ADC2 laut Espressif-Errata unzuverlässig) |
| 6 | GPIO6 | – | Standard-MOSI | digital / SPI |
| 7 | GPIO7 | – | Standard-SS | digital / SPI |
| 8 | GPIO8 | – | **Strapping-Pin**, Onboard-LED (active LOW), Standard-SDA | nicht extern belasten |
| 9 | GPIO9 | – | **Strapping-Pin**, BOOT-Taster, Standard-SCL | nicht extern auf LOW ziehen; I²C-SCL nur mit Vorsicht |
| 10 | GPIO10 | – | allgemein nutzbar | digital |
| 20 | GPIO20 | – | UART0-RX | Serielle Schnittstelle |
| 21 | GPIO21 | – | UART0-TX | Serielle Schnittstelle |
| – | GPIO18/19 | – | USB D-/D+ (nativer USB) | nicht herausgeführt |
| – | GPIO12–17 | – | interner Flash | nie verwenden |

Strapping-Pins des ESP32-C3: **GPIO2, GPIO8, GPIO9**.

## Wi-Fi-Hinweise

- Der ESP32-C3 unterstützt nur **2,4 GHz** (802.11 b/g/n) und BLE 5.
- Ohne Router: der Sketch nutzt den **Access-Point-Modus** – Smartphone verbindet
  sich direkt mit dem WLAN „RC-Telemetrie“, Seite unter `http://192.168.4.1`.
  (Android meldet „kein Internet“ – ggf. „trotzdem verbunden bleiben“ wählen,
  Mobilfunkdaten ggf. kurz ausschalten.)
- Antenne ist auf der Platine klein: Abstand zu Metall/Carbon/Akku halten,
  Reichweite deutlich geringer als bei der Fernsteuerung selbst. Die WLAN-Telemetrie
  ist **kein Ersatz** für die Steuerstrecke und kein Failsafe.
- Bei Verbindungsproblemen: Sendeleistung per `WiFi.setTxPower(WIFI_POWER_8_5dBm)`
  reduzieren (Zeile im Sketch auskommentiert).
