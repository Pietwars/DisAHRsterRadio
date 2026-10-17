# Änderungen

## 0.1.1-dev · Entwicklungsstand (kein Release)

- **Vereinheitlichung (17.10.2026):** README mit Abschnitt „Projektfamilie DisAHRsterRadio“ (in allen drei Repositorys identisch), Versionszeile, ehrlichem Hardwarestand (LILYGO T3 V1.6.1 lief beim Nutzer, Heltec V2 Brownout, T3-S3 ungetestet); Vorschaubilder aller sechs Bereiche neu aus der Live-Demo (`docs/images/funktion-*.jpg`), alte `dar-*.png` entfernt; Demo meldet Version 0.1.1-dev.
- **Flyer:** `docs/flyer/DisAHRsterRadio-Flyer.pdf` (Druck, QR-Codes zu Repository und Demo) und `DisAHRsterRadio-Flyer-digital.pdf` (klickbare Links), in der Live-Demo unter `/flyer/` verlinkt.
- **Live-Demo** auf GitHub Pages: https://pietwars.github.io/DisAHRsterRadio/ (simulierte Daten, Hinweisbanner).
- **Weboberfläche MeshAhrtal:** Foto als vollflächiger Hintergrund mit Schriftzug in voller Breite, schmale Titelleiste, transparente Flächen, bessere Lesbarkeit.
- **Heltec V2 WLAN-Fix:** Brownout-Detektor aus, WLAN startet zuletzt, softAP-Prüfung mit Neuversuch, Status alle 30 s, LoRa-Sendepause beim WLAN-Start, Variante mit 11 dBm (ungeprüft).

## 0.1.0-dev · erster Stand

- Modernisierter Fork von sudomesh/disaster-radio ohne MeshCore: pioarduino (Arduino-ESP32 3.3), RadioLib 7, LoRaLayer2, ESPAsyncWebServer, LittleFS, EU868-Standard.
- Boards LILYGO T3-S3 (SX1262), LILYGO T3 V1.6.1 (SX1276), Heltec WiFi LoRa 32 V2 (SX1276).
- WLAN `DisAHRsterRadio-XXXXXX` mit geräteindividuellem Zufallspasswort.
- Ahrtal-Weboberfläche: Chat, Schnellmeldungen, Standort, Schwarzes Brett (Mehrsprung), Offline-Karten aus OpenStreetMap.
- Flash-Pakete mit `factory.bin` und deutscher Anleitung (`tools/package.sh`).

**Im Notfall immer 112.**
