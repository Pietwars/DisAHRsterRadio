# Verbinden ohne Passwort

Dieser Entwicklungsstand öffnet den lokalen WLAN-Zugang. Bestehende Releases und
deren Flyer beschreiben teilweise noch den bisherigen Zugang mit Passwort.

1. Gerät einschalten und am Handy `DisAHRsterRadio-XXXXXX` auswählen. Es wird kein WLAN-Passwort eingegeben.
2. Bei „Kein Internet“ mit diesem WLAN verbunden bleiben.
3. `http://192.168.4.1` öffnen. Chat, Karte und Meldungen brauchen kein Benutzerkonto.

Der Zugangspunkt stellt lokale Kommunikation bereit, keinen Internetzugang.
Das WLAN ist unverschlüsselt: Jede Person in Reichweite kann mitlesen und senden.
Alle Nutzer teilen denselben Knoten und dessen Nachrichten. Höchstens 4 Handys je Gerät. Für öffentliche Kommunikation einen eigenen Knoten verwenden.
Gespeicherte frühere AP-Passwörter werden beim Start entfernt; Nachrichten und
Einstellungen (z. B. Funkprofil) bleiben erhalten.

## LoRaLayer2: kombinierte WLAN-/BLE-Testprofile

DisAHRsterRadio verwendet weiterhin ihre eigenen
Funk- und Nachrichtenformate. Die Standardprofile öffnen WLAN ohne Passwort.
Die neuen Profile `t3s3-sx1262-ble`, `t3-v161-sx1276-ble` und
`heltec-v2-sx1276-ble` aktivieren zusätzlich BLE-UART ohne PIN, gleichzeitig mit WLAN.
Das ist kein MeshCore-Companion: Die MeshCore-App ist hierfür nicht kompatibel.
Der UART-Dienst verwendet Nordic-UART-UUIDs (6E400001/2/3-B5A3-F393-E0A9-E50E24DCCA9E).
Schreiben: vollständige disaster-radio-Nachricht, 4–232 Bytes, beispielsweise
`00c|<Name> Hallo`; Notifikationen bilden einen Bytestrom und können nach MTU
aufgeteilt werden. Nur Chat (`c`) und Brett (`b`) werden angenommen.
Der alte BLE-Befehl zum Abschalten des WLANs/Neustarten ist entfernt.

Diese kombinierten Profile nutzen `huge_app.csv` und eine andere Partitionierung.
Vor dem Wechsel Firmware und Daten sichern; vollständige Partitionstabelle,
Anwendung und Dateisystem zusammen flashen. Kein reines App-Update über eine alte
Partitionierung. WLAN-Standardprofile behalten ihre bisherige Partitionstabelle.
Telnet auf Port 23 ist deaktiviert; die Betreiberkonsole bleibt über USB verfügbar.

## Abnahme vor Freigabe

- Frisches Handy ohne gespeichertes WLAN/Kopplung: ohne Passwort/PIN verbinden.
- Update eines vorhandenen Geräts mit bisherigem WLAN-Passwort/BLE-PIN:
  danach ebenfalls offen, Identität und Nachrichten bleiben erhalten.
- Zwei Geräte: Chat und Brett in beide Richtungen, Neustart und Wiederverbindung.
- iPhone und Android: WLAN ohne Internet beibehalten, Bluetooth neu koppeln (nur `*-ble`),
  Karte → Chat/Brett und Bildschirmtastatur prüfen.
- Vier WLAN-Clients plus BLE: wiederholte Nachrichten, RAM-Minimum, keine Resets.
- Über BLE Befehle außer Chat/Brett (z. B. `/`-Befehle, UI-Wechsel) senden:
  abgelehnt. Konfiguration über USB prüfen.
- LoRaLayer2-BLE: MTU 23 und 260, überlange/kurze Frames, Queue voll,
  Abmelden/Neuanmelden und ausgeschaltetes Handy während der Übertragung prüfen.

Die neuen Zugangswege sind ohne Hardwareabnahme Entwicklungsstand.
