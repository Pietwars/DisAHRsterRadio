# DisAHRsterRadio

![DisAHRsterRadio Ahrtal](docs/images/disahrsterradio-header.jpg)

**Offline-Chat über LoRa-Mesh für das Ahrtal** – ein Gerät spannt ein eigenes WLAN auf, man verbindet sich mit dem Handy und schreibt im Browser. Die Nachrichten laufen per LoRa von Gerät zu Gerät weiter, ganz ohne Internet oder Mobilfunk.

Dieses Projekt ist ein modernisierter Fork von **[sudomesh/disaster-radio](https://github.com/sudomesh/disaster-radio)** mit der Ahrtal-Weboberfläche aus **[Pietwars/DisasterMesh](https://github.com/Pietwars/DisasterMesh)** – aber **ohne MeshCore**: Funk und Routing laufen über das ursprüngliche disaster-radio-Konzept (Layer1 + LoRaLayer2).

Verwandte Repositorys: [Pietwars/DisasterMesh](https://github.com/Pietwars/DisasterMesh) (Hauptprojekt, MeshCore 1.18 mit großer MeshAhrtal-Oberfläche) · [Pietwars/DisAHRsterRadio-MeshCore](https://github.com/Pietwars/DisAHRsterRadio-MeshCore) (dieselbe Oberfläche wie hier, aber auf MeshCore 1.18 – kompatibel zu MeshCore-Geräten).

> ⚠️ **Wichtig:** DisAHRsterRadio ist **kein zertifiziertes Notrufsystem** und ersetzt keine Behörden-Kommunikation. In Lebensgefahr immer **112** wählen, wenn irgendwie möglich.

## Ehrlicher Stand (Oktober 2026)

| Punkt | Stand |
|---|---|
| Firmware baut für alle 3 Boards | ✅ (lokal und per GitHub Actions) |
| Auf echter Hardware getestet | ❌ **noch nicht** – weder Funk, WLAN noch Web-UI auf einem Gerät geprüft |
| Funkverbindung zwischen zwei Geräten | ❓ ungetestet |
| Kompatibel mit altem disaster-radio-Funk (915 MHz/US) | ❌ nein, EU868-Standard |
| Kompatibel mit MeshCore/DisasterMesh-Geräten | ❌ nein, anderes Funkprotokoll |

## Unterstützte Boards

| PlatformIO-Env | Board | Funkchip |
|---|---|---|
| `t3s3-sx1262` | LILYGO T3-S3 (Hauptziel) | SX1262 (TCXO 1,8 V) |
| `t3-v161-sx1276` | LILYGO T3 V1.6.1 | SX1276 |
| `heltec-v2-sx1276` | Heltec WiFi LoRa 32 V2 | SX1276 |

Heltec V2 (WLAN-Fix): Brownout-Detektor früh aus, WLAN startet zuletzt (2 s nach LoRa/Display), WLAN-Sendeleistung 8,5 dBm (Variante `heltec-v2-sx1276-wifi11` mit 11 dBm), softAP auf Kanal 1 mit Prüfung und Neuversuch alle 5 s, LoRa-Sendepause während des WLAN-Starts, Startmarken `[BOOT …]` und alle 30 s WLAN-Name/Passwort seriell und auf dem OLED.

## Was gegenüber disaster-radio modernisiert wurde

- PlatformIO mit **pioarduino** (aktuelles Arduino-ESP32 3.3.x / ESP-IDF 5.5) statt altem Git-Stand der espressif32-Plattform
- **RadioLib 7.x** statt sandeepmistry/arduino-LoRa bzw. uraltem RadioLib-Commit; eine gemeinsame `Layer1_RadioLib` für SX1262 und SX1276 (SX1262 jetzt mit BUSY-Pin, TCXO, DIO2-HF-Schalter)
- **LoRaLayer2** (Routing von disaster-radio) liegt angepasst in `firmware/lib/LoRaLayer2` (Sende-/Empfangsleistung und SF zur Laufzeit, Konfigurationsausgabe auch für RadioLib)
- **ESP32Async/ESPAsyncWebServer + AsyncTCP** (gepflegte Forks), **LittleFS** statt SPIFFS, AsyncSDServer entfernt (SD über ESPAsyncWebServer)
- **EU868-Standard:** 869,525 MHz, 125 kHz, SF9, 14 dBm (Duty-Cycle-Begrenzung von LoRaLayer2 aktiv)
- WLAN-Zugangspunkt **`DisAHRsterRadio-XXXXXX`** mit **geräteindividuellem Zufallspasswort** (einmal erzeugt, im NVS gespeichert, beim Start seriell ausgegeben)
- BLE-UART von disaster-radio ist vorerst deaktiviert (Bluedroid-API passt nicht zu Arduino-ESP32 3.x auf dem S3; Build-Flag `HAS_BLE`)

## Weboberfläche (MeshAhrtal)

![Weboberfläche](docs/images/dar-ui.png)

- Chat an alle (disaster-radio-WebSocket `/ws`, Format `<ID 2 Byte>c|<Name> Text`, Zustellbestätigung ✓ = vom Gerät angenommen, *nicht* beim Empfänger)
- Schnellmeldungen (Trinkwasser, Strom, Medikamente, Verletzt, Evakuierung, „Mir geht es gut“, Hilfe-Angebote)
- Standort: GPS des Telefons (funktioniert im Browser oft nur über HTTPS) oder Auswahl aus der Ortsliste Ahrtal (Ahrweiler … Sinzig)
- **Schwarzes Brett:** Aushänge (optional „wichtig/angeheftet“, Laufzeit 6 h – 7 Tage) werden auf jedem Gerät in LittleFS (`/board.txt`) gespeichert, per Funk als eigener Datagramm-Typ `b` verteilt, anhand einer zufälligen 8-stelligen ID entdoppelt und von jedem Gerät einmal weitergesendet (Mehrsprung). Alle ~5 min sendet jedes Gerät einen gespeicherten Aushang erneut, damit später eingeschaltete Geräte nachziehen. Max. 30 Aushänge pro Gerät; `GET /api/board`
- **Offline-Karte:** fünf OSM-Pakete aus DisasterMesh (Ahrtal-Übersicht, Heimersheim mit Gebäuden, Dernau/Ahrweiler, Altenahr, Sinzig), kompakt und gzip-komprimiert (zusammen ca. 280 KB) im LittleFS; Anzeige mit Orten, Versorgungspunkten, eigener Position und Positionen aus Chat-/Brett-Nachrichten (Koordinaten im Text). Erzeugen mit `tools/make_map.py`. © OpenStreetMap-Mitwirkende, ODbL 1.0 (`web/static/map/ODbL-1.0.txt`)
- Geräteinfo über `GET /api/info` (JSON)
- Demo ohne Gerät: `web/static/index.html?demo=1` lokal öffnen

Die MeshCore-spezifischen Funktionen aus DisasterMesh (Kanäle, Direktnachrichten, Karten-Download/-Import, eigene POIs, Admin) sind **nicht** übernommen.

![Schwarzes Brett](docs/images/dar-brett.png) ![Karte](docs/images/dar-karte.png)

## Erste Schritte

```bash
pip install platformio
cd firmware
pio run -e t3s3-sx1262 -t upload      # Firmware
pio run -e t3s3-sx1262 -t uploadfs    # Weboberfläche (LittleFS)
pio device monitor                     # zeigt WLAN-Name und Passwort
```

Dann mit dem WLAN `DisAHRsterRadio-XXXXXX` verbinden und `http://192.168.4.1` öffnen (oder `http://disahrster.local`). Seriell/Telnet (Port 23) gibt es weiterhin die disaster-radio-Konsole.

## Fertige Flash-Pakete

`tools/package.sh` baut je Board ein ZIP mit `factory.bin` (ab 0x0), Einzelteilen, `SHA256SUMS` und `ANLEITUNG.md` (esptool-js, Update nur `firmware.bin` an 0x10000).

## Bekannte Grenzen / offene Arbeit

- Hardwaretest auf allen drei Boards (Funk zwischen zwei Geräten, Reichweite, Stromaufnahme, Heltec-Brownout)
- Pinbelegung T3-S3 und T3 V1.6.1 nach Datenblatt/LILYGO-Beispielen gesetzt, nicht gemessen
- Keine Verschlüsselung/Signatur der Nachrichten (alle mit gleicher Frequenz können mitlesen und Namen fälschen)
- Nachrichtenlänge max. ca. 220 Byte; kein Direktchat, keine Kanäle
- **Chat ist nur Einzelsprung:** LoRaLayer2 leitet Broadcasts nicht weiter (upstream-Verhalten). Nur das Schwarze Brett wird über mehrere Geräte weitergereicht
- Brett: Ablaufzeit läuft nur, solange das Gerät an ist (keine Uhr); Aushänge können nicht gelöscht werden; jeder kann „wichtig“ setzen; `littlefs.bin` neu flashen löscht die Aushänge
- Karte: keine Navigation, keine Kacheln, keine Gebäude außer Heimersheim; Datenstand 16.10.2026
- Einstellung `loraFrq` ist ganzzahlig (869 = 869,525 MHz)
- OLED-, GPS-, SD- und History-Code von disaster-radio übernommen, ungetestet

## Lizenz und Danksagung

- Grundlage: [sudomesh/disaster-radio](https://github.com/sudomesh/disaster-radio) und [sudomesh/LoRaLayer2](https://github.com/sudomesh/LoRaLayer2) – vielen Dank an Sudo Mesh und alle Beteiligten. Git-Historie von disaster-radio ist erhalten, Original-README: [README.upstream.md](README.upstream.md).
- `firmware/`: GPLv3 und AGPLv3 (dual, wie upstream) – siehe `LICENSE-GPL-3.0.txt`, `LICENSE-AGPL-3.0.txt`
- `web/`: AGPLv3 (wie upstream). Die neue Ahrtal-Oberfläche stammt vom Autor aus DisasterMesh (dort MIT) und wird hier unter AGPLv3 veröffentlicht – MIT ist damit kompatibel.
- Kopfbild: aus [Pietwars/DisasterMesh](https://github.com/Pietwars/DisasterMesh).
