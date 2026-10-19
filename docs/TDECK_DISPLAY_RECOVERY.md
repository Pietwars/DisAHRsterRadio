# T-Deck Plus Standalone 0.9.4: Bedienung, Crashanalyse und Wiederherstellung

Releaseprofil `tdeck-standalone`; Entwicklungsprofil `tdeck-screen-test`.
Version 0.9.4 wird als zusätzliches Release veröffentlicht. v0.9.3 bleibt unverändert.
Start, lokale Bedienung und Hilfen sind vom Nutzer bestätigt; GPS/Karte im Freien und umfassende Funk-/Dauertests stehen noch aus.

## Bedienfix nach erfolgreichem Geraetestart

Der Nutzer hat den Start und die laufende Anwendung nach dem SPI-Fix bestaetigt.
Der nachfolgende Bedienfix bleibt im selben experimentellen Profil:

- Trackball-Richtungen zaehlen jetzt Signalwechsel statt ruhender LOW-Pegel.
  Vier zusammenhaengende Impulse pro Schritt, mindestens 350 ms Abstand;
  keine aufgestauten Schritte. Ein Klick wird entprellt und erst nach Loslassen
  erneut akzeptiert. Ruhende Pins und gehaltene Klicks loesen keine Wiederholung aus.
- Tastatur wird alle 20 ms per I2C gelesen, unabhaengig von GPIO46. Die
  Hersteller-Firmware fragt die Tastatur ebenfalls direkt ab. Der Startlog meldet
  das Ergebnis der Tastatur- und Touch-Erkennung. Der I2C-Timeout ist begrenzt.
- Das vorhandene Tal-und-Funk-Bild aus `docs/images/disahrsterradio-header.jpg`
  erscheint fuer etwa 1,8 Sekunden, proportional eingepasst. Es liegt in Flash;
  die Ausgabe nutzt nur eine einzelne Pixelzeile im RAM.
- GT911-Touch auf beiden moeglichen I2C-Adressen: Tabs antippen, Hilfen mit zwei
  getrennten Tippgesten bestaetigen, SEND bei Chat/Brett, Wischen zum Blaettern,
  Zoom/Zentrierung unten auf der Karte. Mehrfinger-Gesten werden verworfen.
- Beim ersten Start mit erkanntem Touch: drei Zielkreuze nacheinander antippen
  und jeweils loslassen; anschliessend die Mitte zur Kontrolle antippen.
  Nur eine gueltige, erfolgreich gepruefte Ausrichtung wird in einem eigenen
  NVS-Namensraum gespeichert. Der Log meldet einen fehlgeschlagenen Schreibversuch.
  Unter **SETUP / Touch neu kalibrieren** laesst sich der Ablauf wiederholen.
  Trackball-Klick oder Escape bricht die Kalibrierung ab, ohne die alte zu ersetzen.

Nach dem Flashen bitte pruefen: keine Bewegung ohne Rollen, eine bewusste
Rollbewegung ergibt einen Menueschritt, langer Klick sendet keine Hilfe selbst,
Tastaturtext und Enter funktionieren. Dann alle Touch-Zielkreuze durchlaufen,
Tab-Treffer und Hilfebestaetigung testen, Neustart und gespeicherte Kalibrierung
pruefen. Noch keine Hardware-Abnahme dieser neuen Eingabe-/Touch-Erweiterung.

Herstellerreferenz: [LilyGO UnitTest](https://github.com/Xinyuan-LilyGO/T-Deck/blob/master/examples/UnitTest/UnitTest.ino).

## Befund vom 19.10.2026

Der vollstaendige Nutzerlog zeigt `StoreProhibited`, `EXCCAUSE=0x1d`,
`EXCVADDR=0x00000010`, PC `0x420171f1`. LittleFS, WLAN und LoRa sind erfolgreich
initialisiert. Direkt nach `[BOOT 795 ms] TDeck Display` folgt die Panic.
Das ist ein ungueltiger Schreibzugriff beim Display-Start, kein nachgewiesener
Brownout, Watchdog oder Speicherplatzmangel.

Das Originalpaket aus Actions-Lauf `39239362411` (Checkout `06b238a`, PR-Head
`f4a8d3d`) enthaelt in `firmware.bin` den ELF-Hash
`3b320a93fefe7e5243dbf71a4cdab81e582d737c84ab9ee5bd2c3f513c31e1d0`.
Er passt zum Log. Der bisherige Workflow hat ELF und Map nicht archiviert;
das BIN allein liefert keine exakte Symbol-/Zeilendekodierung. Eine neu gebaute
ELF darf nur verwendet werden, wenn ihr SHA256 zum Crash passt.

### Ursache im verwendeten Quellstand

TFT_eSPI 2.5.43 setzt auf dem S3 standardmaessig `SPI_PORT=FSPI` fuer direkte
Registerzugriffe. Arduino-ESP32 3.3.12 definiert `FSPI=0` als Arduino-Busnummer,
waehrend die Registermakros die Hardware-Peripherienummer **2** benoetigen.
`SPI_USER_REG(0)` ergibt `0x10`; `SET_BUS_WRITE_MODE` schreibt dorthin.
Das erklaert Exception, Zieladresse und Zeitpunkt. Ohne Original-ELF ist die
Zuordnung des gemeldeten PC zu einer bestimmten Quellzeile nicht bewiesen.

Der Testzweig uebernimmt nur diese eine Zeile aus dem Upstream-Fix
[83d4d16](https://github.com/Bodmer/TFT_eSPI/commit/83d4d16) in die fest gepinnte
Bibliothek. Der Build prueft den Header-Hash und bricht bei unerwartetem Inhalt ab.
Compile-Zusicherungen pruefen Port und Registeradresse. Der bestehende gemeinsame
`SPI`-Verweis fuer TFT und RadioLib bleibt erhalten. Kein Wechsel auf HSPI,
kein pauschaler Core-/Bibliothekswechsel, keine geaenderte Partitionstabelle.
`LOAD_GLCD` aktiviert zusaetzlich die benoetigte Schrift fuer die lokale UI.

### Core-Dump-Meldungen sind Folgefehler

`Core dump flash config is corrupted`, `Elf write init failed` und
`Core dump write failed` erscheinen nach der eigentlichen Panic beim Versuch,
den Crash zu speichern. Die verwendete Partitionstabelle enthaelt **keine**
Coredump-Partition; dazu passt `No core dump partition found` beim Neustart.
Das beweist keine beschaedigte Anwendungsfirmware oder defektes Flash.
`rst:0xc (RTC_SW_CPU_RST)` ist hier der anschliessende Software-Neustart.
`Saved PC` ist kein Ersatz fuer die Exception-PC-Adresse. Flash-Loeschen oder
eine neue Partition beheben den falschen TFT-Registerzugriff nicht.

### Weitere gepruefte Risiken

- SPI-Pins: SCK40, MISO38, MOSI41; getrennte CS fuer LoRa9, TFT12, SD39.
  TFT/SD werden vor LoRa inaktiv gesetzt und beim Display-Start erneut
  abgewählt. Im T-Deck-Profil ist SD nicht eingebunden (`SD_SCK` fehlt).
- Der LoRa-Interrupt setzt nur ein Flag; Funk-SPI und TFT zeichnen im normalen
  Loop. Web-Callbacks koennen weiterhin gemeinsame Client-/Brettdaten beruehren;
  deren Nebenlaeufigkeit ist ein separater Belastungstest, kein Beleg fuer diese
  fruehe Panic. Ein neuer vollstaendiger Mutex-Umbau gehoert nicht in diesen Fix.
- Keine Sprite-/Vollbildpuffer-Allokation im UI-Start (ein RGB565-Vollbild waere
  153600 Byte). Nachrichtenanzahl und Eingabetext sind begrenzt. Es gibt keinen
  Heap-/Stack-Corruption-Hinweis im vorliegenden Log; ein Langzeittest bleibt
  erforderlich. Der neue Log meldet Heap-Minimum und Stackreserve vor `lcd.init`.
- PSRAM ist nicht erforderlich und wird nicht neu aktiviert. LittleFS und WLAN
  laufen bereits. Die TFT-Frequenz bleibt bei 27 MHz, die LoRa-Einstellungen
  bleiben unveraendert. Die Kartenansicht ist weiterhin nur schematisch.

## Test auf dem Geraet

1. Einstellungen und benoetigte Brettdaten sichern. Bei Bedarf zuerst ein
   vollstaendiges 16-MB-Flash-Backup lesen (siehe unten). Antenne anschliessen.
2. Das ZIP **DisAHRsterRadio-0.9.4-tdeck-standalone.zip** aus dem neuen
   Release laden; Entwicklungsstände nur aus dem zugehörigen erfolgreichen CI-Lauf. `debug/build.json` nennt Commit und ELF-Hash. Pruefsummen
   gegen `SHA256SUMS` vergleichen. Das alte ZIP mit ELF-Praefix `3b320a93f`
   enthaelt den Fehler.
3. Bei unveraenderter Partitionstabelle vorzugsweise nur `teile/firmware.bin`
   auf `0x10000` schreiben. Das behaelt NVS und LittleFS bei. Kein Erase Flash.
   Fuer eine Erstinstallation das im ZIP vorhandene `*-factory.bin` auf `0x0`
   schreiben; es ersetzt auch den LittleFS-Bereich und damit dortige Daten.
4. Serielle Ausgabe mit 115200 Baud ab Reset aufnehmen. Erwartet:
   `SPI_PORT=2 USER_REG=0x60024010`, danach `lcd.init OK`, `first fill OK`,
   die UI-Startmeldung und `[BOOT ...] fertig`. Keine neue Panic.
5. Zehn Kaltstarts pruefen. WLAN-Weboberflaeche `http://192.168.4.1` benutzen,
   Chat in beide Richtungen testen, waehrend Tabs wechseln und gezeichnet wird.
   Alle acht Hilfen mit Zweifachbestaetigung, Brett, Tastatur und Trackball testen.
   GPS im Freien testen. Anschliessend mindestens 30 Minuten Parallelbetrieb;
   Heap-Minimum und neue Reset-/Panic-Zeilen festhalten.
6. Bei Fehler den gesamten Log einschliesslich Guru-Meditation und Registerdump,
   ZIP/CI-Lauf und `debug/build.json` aufheben. Die bestätigte lokale Bedienung ersetzt keinen Funk-/Dauertest oder GPS-Test im Freien.

## Recovery ohne Erase Flash

Bei der Bootschleife Trackball gedrueckt halten und einschalten/Reset ausloesen,
dann loslassen: ROM-Downloadmodus. Den neu erschienenen USB-Port auswaehlen.
In Espressifs [Web-Flasher](https://espressif.github.io/esptool-js/) kann auf
`0x10000` die gesicherte Stable-**T-Deck**-`firmware.bin` aus v0.9.3 geschrieben
werden, sofern die hier unveraenderte Partitionstabelle vorhanden ist.
Das setzt das Geraet zurueck auf die WLAN-Bedienung; es aendert keinen Git-Tag
oder Release. Nicht versehentlich das T-Watch-Paket verwenden.

Alternativ mit esptool 5.x (`COMx` durch den richtigen Port ersetzen):

```text
python -m esptool --chip esp32s3 --port COMx read-flash 0x0 0x1000000 tdeck-backup.bin
python -m esptool --chip esp32s3 --port COMx write-flash 0x10000 teile/firmware.bin
```

Im Downloadmodus starten, Backup **vor** dem Schreiben lesen. Bei unklarer oder
abweichender Partitionierung nach Backup das vollstaendige, gepruefte Stable-
Factory-Paket auf `0x0` schreiben. Das kann LittleFS-Daten ersetzen; ein kompletter
Erase ist fuer den hier analysierten Fehler kein notwendiger Reparaturschritt.

## Backtrace exakt dekodieren

Nur mit SHA-passender ELF. Das aktuelle Debug-ELF gehoert zum neuen Build und
kann die alte Panic nicht exakt dekodieren. SHA256 der ELF berechnen; er muss
zum Hash im Panic-Log passen. Danach das zum ESP32-S3-Toolchain-Paket gehoerende
`xtensa-esp32s3-elf-addr2line` (bzw. `xtensa-esp-elf-addr2line`) aufrufen:

```text
xtensa-esp32s3-elf-addr2line -pfiaC -e firmware.elf 0x420171f1 0x420171ee 0x420172c1 0x4201d933 0x4201ab55 0x420294ee 0x4037e7f5
```

Quellen: [gepinntes TFT-S3-Header](https://github.com/Bodmer/TFT_eSPI/blob/V2.5.43/Processors/TFT_eSPI_ESP32_S3.h),
[Arduino-ESP32 SPI-Enums](https://github.com/espressif/arduino-esp32/blob/3.3.12/cores/esp32/esp32-hal-spi.h),
[vergleichbarer Upstream-Befund](https://github.com/Bodmer/TFT_eSPI/issues/3925).
