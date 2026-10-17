# Änderungen

## 0.1.1-alpha · 17.10.2026 · erste Vorabversion (automatisches Release)

- Version 0.1.1-alpha: erste veröffentlichte Vorabversion mit Paketen für LILYGO T3-S3, LILYGO T3 V1.6.1 und Heltec V2 (Hardware-Abnahme steht aus).
- Gemeinsamer Kern: DisAHRsterRadio ist Kern für HaldenFunk (Overlay-Verfahren, siehe README); CI-Schritt „Kern-Abgleich“ läuft nur in Regionalvarianten (`region/CORE`).

- Karten verkleinert mit `tools/shrink_maps.py` (aus HaldenFunk übernommen, Profil `ahrtal`): zusammen ca. 290 → 190 KB (Ahrtal 51 → 32 KB, Altenahr 47 → 24 KB, Dernau 60 → 33 KB, Heimersheim 97 → 82 KB, Sinzig 35 → 19 KB); Orte und POIs unverändert, Screenshot-Vergleich vorher/nachher geprüft.
- Fehler behoben: Die Karten Dernau, Altenahr und Sinzig zeigten „Karte nicht verfügbar“, weil ihnen die Grenzen (bbox) fehlten; `make_map.py` und `shrink_maps.py` ergänzen sie jetzt aus den Daten. Neuer Test `tests/maps.test.py` in der CI.
- **Zentrale Version (17.10.2026):** Datei `VERSION` ist die einzige Quelle; `tools/version.py` schreibt `DAR_VERSION` (config.h), Fußzeile „Weboberfläche DisAHRsterRadio …“ im Gerät-Reiter, Demo-Gerätedaten, README-Versionszeile; `--check` + `tests/version.test.py` laufen in CI. `tools/package.sh` und das Demo-Banner (`build_pages.py`) lesen `VERSION`.
- Projektfamilie: gemeinsame Tabelle mit allen fünf Repositorys (DisasterMesh, DisAHRsterRadio-MeshCore, DisAHRsterRadio, HaldenFunk, HaldenFunk-MeshCore).
- **Automatische Releases:** Workflow `release.yml` baut bei jedem Tag `v*` alle drei Boards, packt `…-factory.bin`, `teile/` (bootloader, partitions, boot_app0, firmware, littlefs), `SHA256SUMS` und deutsche `ANLEITUNG.md`, prüft alles mit `tools/verify_release.py` (bricht bei fehlender Datei/falscher Prüfsumme/fehlender Version ab) und veröffentlicht eine Vorabversion mit Text aus diesem CHANGELOG (`tools/release_notes.py`). Bei Änderungen an der Paketierung läuft derselbe Workflow im Pull Request als Probelauf ohne Veröffentlichung. Paketnamen jetzt mit Version: `DisAHRsterRadio-<Version>-<Board>.zip` (wie bei DisasterMesh und DisAHRsterRadio-MeshCore).
- **Freier Speicher (Heap):** `/api/info` liefert `heap`, `minHeap`, `maxBlock` und die Warnschwelle `heapWarn` (Standard 40 KB, `HEAP_WARN_BYTES` in `config.h`); der Gerät-Reiter zeigt die Werte in KB, unter der Schwelle rot mit ⚠, und aktualisiert sich alle 10 s. Seriell alle 30 s `[HEAP] frei … | Minimum … | groesster Block …` mit Warnung unter der Schwelle.
- **Feste Versionen:** Plattform pioarduino `55.03.312-1` (Arduino-ESP32 3.3.12, ESP-IDF 5.5.5) statt `stable`, Bibliotheken exakt (AsyncTCP 3.5.0, ESPAsyncWebServer 3.12.1, RadioLib 7.8.1, OLED-Treiber 4.6.2, TinyGPSPlus 1.1.0, ArduinoJson 7.4.3 – die Versionen, mit denen CI zuletzt grün baute); CI mit `platformio==6.2.0`, `esptool==5.6.0`.
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
