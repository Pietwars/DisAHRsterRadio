# Konzept: DisAHRsterRadio „Omi“ – die ganz einfache Version

Stand: 17.10.2026 · Entwurf · Branch `omi/einfach` · Eigentümer: Peter Müller

## Ziel

Ein Notfunk-Gerät für Menschen, die Technik nicht mögen.
Man schaltet es ein. Es funktioniert.
Man verbindet das Handy mit dem WLAN. Die Seite öffnet sich.
Man schreibt. Man drückt „Senden“. Fertig.

Leitsatz: **Was man erklären muss, fliegt raus.**
Es bleibt nur der Kern: Chat über LoRa-Funk mit offenem WLAN.

## Was die Omi-Version hat

| Teil | So ist es |
|---|---|
| Gerät | **Nur an/aus.** Kein Menü. Keine Einstellung am Gerät. Die Knöpfe tun nichts (es gibt keinen Knopf-Code; BOOT/PRG ist nicht belegt, RST startet nur neu). |
| Display (OLED) | **Immer derselbe Text**, keine Seiten: „Funk an“ · „Handy mit WLAN verbinden:“ · `Notfunk-XXXXXX` · „ohne Passwort“. Bei Funkfehler: „Funk FEHLER – Bitte Gerät aus- und wieder einschalten.“ Beim Start kurz „Bitte warten …“. Keine Nachrichten, keine IP, keine MAC, keine Zähler. Alle 5 min 0–2 Pixel Versatz gegen Einbrennen (unsichtbar). |
| WLAN | Offen, ohne Passwort. Name **`Notfunk-XXXXXX`** (statt `DisAHRsterRadio-XXXXXX`, kürzer und selbsterklärend). |
| Seite öffnet sich selbst | **Captive Portal**: kleiner DNS-Server im ESP32 beantwortet jede Adresse mit 192.168.4.1; unbekannte Seiten (z. B. Internet-Test von Android/iOS/Windows) werden auf die Startseite umgeleitet. Das Handy zeigt „Im Netzwerk anmelden“ bzw. öffnet die Seite selbst. Fallback: `http://192.168.4.1`. |
| Weboberfläche | **Ein einziger Bildschirm**: oben „Notfunk“ + Zustand („Funk bereit ✓“ / „Nicht verbunden. Bitte warten …“), Mitte Nachrichtenliste, unten großes Feld „Ihre Nachricht“ + großer Knopf **Senden**. Große Schrift (22 px), Schwarz auf Weiß, dicke Rahmen. Eigene Nachrichten: „Ich:“ (blau), fremde: „Von …:“. Nach dem Senden: „wird gesendet …“ → „gesendet ✓“. Keine Tabs, kein Menü, keine Einstellungen. |
| Absendername | **Kein Namensdialog.** Der Name ist automatisch der WLAN-Name des Geräts (`Notfunk-XXXXXX`). Er steht auf dem Gerät/Zettel; andere können so sehen, von welchem Gerät die Nachricht kommt. |
| Funk | Fest **EU Narrow** (869,618 MHz, 62,5 kHz, SF8, CR 4/8, 10 % Duty Cycle). Gespeicherte Einstellungen im NVS werden ignoriert, also auch ein früher umgestelltes Gerät läuft sicher auf Standard. Routing, Weiterleiten, Duty-Cycle laufen automatisch im Hintergrund. |
| Kompatibilität | **Gleiches Profil, gleiches Paketformat** (`c|<Name> Text`) → Omi-Geräte und normale DisAHRsterRadio-Geräte sprechen miteinander. Das Schwarze Brett (`b|…`) wird intern weiter verarbeitet/weitergeleitet (BoardClient bleibt), nur nicht angezeigt. |

## Was weggelassen wird

- Tabs **Karte**, **Brett**, **Standort** (Ortslisten), **Gerät/Funkprofil**
- **Schnellmeldung-Knöpfe** (bewusst weggelassen: jeder Knopf muss erklärt werden; „Mir geht es gut“ ist schnell getippt) → siehe offene Fragen
- **Namensdialog** (automatischer Gerätename)
- Funkprofil-Auswahl (`/api/presets`, `/api/preset` sind im Omi-Build nicht vorhanden)
- **BLE** (keine `*-ble`-Omi-Envs)
- Serielle **Konsole**/Chat und Telnet (serielles Protokoll für Helfer bleibt)
- Chat-Befehle (`!!`, `!~` – Name löschen, auf BLE umschalten + Neustart)
- Beitrittsmeldungen („… ist beigetreten“) und Technik-Hinweise („keine SD-Karte …“) in der Oberfläche
- Nachrichtenanzeige und Statusleiste (MAC/IP/Zähler) auf dem OLED
- Debug-/Speicheranzeige, Versionsanzeige, Hintergrundbild, Karten (spart LittleFS: nur `index.html` ~5 KB + Icon)

## Umsetzung (isoliert, wenig Änderungen am Hauptcode)

- Build-Flag **`-D DAR_OMI`**, eigene Envs: `t3-v161-omi`, `heltec-v2-omi`, `t3s3-omi` (erben von den normalen Envs).
- Eigene Weboberfläche **`web/omi/`** (statt `web/static/`), umgeschaltet per `firmware/omi_data.py` (setzt `PROJECT_DATA_DIR` nur für Omi-Envs).
- Alle Firmware-Abweichungen als `#ifdef DAR_OMI`-Blöcke in `main.ino` und ein Block in `WebSocketClient.cpp`. Normale Builds bleiben unverändert.
- Captive Portal mit `DNSServer` aus Arduino-ESP32 (im Framework enthalten, wenig Flash, kein neues lib_dep).

### Captive Portal – Einschätzung

Machbar und risikoarm: DNSServer-Wildcard + Weiterleitung im `onNotFound` ist das Standardmuster (WiFiManager etc.).
Es nimmt den schwierigsten Schritt weg („192.168.4.1 eintippen“).
Risiken, die auf echten Handys zu prüfen sind:
- iOS öffnet ein Mini-Fenster („Captive Network Assistant“). Darin läuft die Seite (inkl. WebSocket), aber beim Schließen fragt iOS evtl. „Ohne Internet verwenden?“ → in der Anleitung mit „Im Netzwerk anmelden“ abgefangen; muss getestet werden.
- Android zeigt „Im Netzwerk anmelden“ (Benachrichtigung); einige Geräte wechseln ohne Internet auf mobile Daten. Captive Portal hilft eher, weil Android das WLAN dann als „Anmeldung nötig“ statt „kein Internet“ behandelt.
- DNS-Anfragen vieler Apps landen beim ESP32 → etwas Last; wird nur bei Verbindungen beantwortet, unkritisch.

## Offene Fragen an Peter

1. **WLAN-Name** `Notfunk-XXXXXX` ok – oder beim bekannten `DisAHRsterRadio-XXXXXX` bleiben?
2. **Absendername** = WLAN-Name des Geräts ok? Alternative: Helfer schreibt beim Flashen einen festen Namen ein (z. B. „Erna Dernau“) – ohne Dialog für die Nutzerin.
3. **Schnellmeldungen**: wirklich keine – oder genau **ein** Knopf „Mir geht es gut“?
4. **Brett-Aushänge** (z. B. „Trinkwasser am Rathaus“) nur lesend unten anzeigen – oder ganz weglassen (jetzt: weggelassen)?
5. **OLED**: nur fester Text (jetzt so) – oder zusätzlich „Neue Nachricht“, wenn eine ankommt?
6. **Ansprache** „Sie“ (jetzt) oder „du“?
7. **Captive Portal** muss auf echten Handys getestet werden (iPhone, 2–3 Android). Falls es Probleme macht: die zwei kleinen Blöcke (DNS + Umleitung) lassen sich leicht entfernen; 192.168.4.1 geht immer.
8. Omi-Envs auch in **Release-Pakete** / Flasher-Seite aufnehmen (`release.yml`, `tools/package.sh`)? Jetzt: nur CI-Build.
9. Aufkleber am Gerät mit WLAN-Name (Zettel-Feld zum Ausfüllen ist vorbereitet) – oder QR-Code zum WLAN-Verbinden? (QR wäre ein Extra, das man erklären muss.)
