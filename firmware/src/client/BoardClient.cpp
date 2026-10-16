#include "BoardClient.h"
#include <ArduinoJson.h>

static String clean(const String &s, bool allowPipe)
{
    String o;
    for (size_t i = 0; i < s.length(); i++)
    {
        char c = s[i];
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        if (!allowPipe && c == '|') c = '/';
        o += c;
    }
    return o;
}

static uint32_t remainingMin(const BoardPost &p)
{
    int32_t ms = (int32_t)(p.expiresAt - millis());
    return ms <= 0 ? 0 : (uint32_t)ms / 60000UL + 1;
}

bool BoardClient::parse(const char *s, size_t len, BoardPost &p, uint32_t &ttl)
{
    String str;
    str.reserve(len);
    for (size_t i = 0; i < len && s[i]; i++) str += s[i];
    int a = str.indexOf('|'), b = str.indexOf('|', a + 1), c = str.indexOf('|', b + 1), d = str.indexOf('|', c + 1);
    if (a != 8 || b < 0 || c < 0 || d < 0) return false;
    p.id = str.substring(0, 8);
    for (size_t i = 0; i < 8; i++) if (!isxdigit(p.id[i])) return false;
    p.flags = str.substring(a + 1, b).indexOf('P') >= 0 ? "P" : "-";
    ttl = str.substring(b + 1, c).toInt();
    if (ttl == 0 || ttl > BOARD_MAX_TTL_MIN) ttl = ttl == 0 ? BOARD_DEFAULT_TTL_MIN : BOARD_MAX_TTL_MIN;
    p.name = clean(str.substring(c + 1, d), false).substring(0, 24);
    p.text = clean(str.substring(d + 1), true).substring(0, 180);
    return p.text.length() > 0;
}

int BoardClient::find(const String &id)
{
    for (size_t i = 0; i < _posts.size(); i++) if (_posts[i].id == id) return i;
    return -1;
}

void BoardClient::add(BoardPost &p)
{
    if (_posts.size() >= BOARD_MAX_POSTS)
    {
        // zuerst nicht angeheftete, davon die mit der kuerzesten Restzeit entfernen
        int victim = -1;
        for (int pass = 0; pass < 2 && victim < 0; pass++)
            for (size_t i = 0; i < _posts.size(); i++)
                if ((pass == 1 || _posts[i].flags != "P") &&
                    (victim < 0 || (int32_t)(_posts[i].expiresAt - _posts[victim].expiresAt) < 0))
                    victim = i;
        _posts.erase(_posts.begin() + victim);
    }
    _posts.push_back(p);
    _dirty = true;
}

void BoardClient::send(BoardPost &p)
{
    struct Datagram dg;
    memset(&dg, 0, sizeof(dg));
    memset(dg.destination, 0xff, ADDR_LENGTH);
    dg.type = BOARD_TYPE_LORA;
    int n = snprintf((char *)dg.message, MESSAGE_LENGTH, "%c%cb|%s|%s|%lu|%s|%s", 0, 0, p.id.c_str(), p.flags.c_str(),
                     (unsigned long)remainingMin(p), p.name.c_str(), p.text.c_str());
    if (n >= MESSAGE_LENGTH) n = MESSAGE_LENGTH - 1;
    server->transmit(this, dg, n + DATAGRAM_HEADER);
}

void BoardClient::receive(struct Datagram datagram, size_t len)
{
    if (datagram.type != BOARD_TYPE_LORA && datagram.type != BOARD_TYPE_LOCAL) return;
    if (len < DATAGRAM_HEADER + 4) return;
    size_t mlen = len - DATAGRAM_HEADER;
    if (datagram.message[2] != 'b' || datagram.message[3] != '|') return;
    BoardPost p;
    uint32_t ttl;
    if (!parse((const char *)&datagram.message[4], mlen - 4, p, ttl)) return;
    if (find(p.id) >= 0) return; // schon bekannt -> nicht erneut verteilen
    p.receivedAt = millis();
    p.expiresAt = millis() + ttl * 60000UL;
    if (datagram.type == BOARD_TYPE_LOCAL)
    {
        add(p);
        send(_posts.back()); // sofort an Funk und alle Browser
    }
    else
    {
        // ueber Funk empfangen: einmal mit Zufallsverzoegerung weiterleiten (Mehrsprung-Flooding)
        p.rebroadcastPending = true;
        p.rebroadcastAt = millis() + 1500 + (esp_random() % 6000);
        add(p);
    }
    save();
}

void BoardClient::expire()
{
    for (int i = _posts.size() - 1; i >= 0; i--)
        if ((int32_t)(_posts[i].expiresAt - millis()) <= 0) { _posts.erase(_posts.begin() + i); _dirty = true; }
}

void BoardClient::setup() { load(); _lastSync = millis(); }

void BoardClient::loop()
{
    uint32_t now = millis();
    if (now - _lastExpiry > 10000) { _lastExpiry = now; expire(); }
    for (auto &p : _posts)
        if (p.rebroadcastPending && (int32_t)(now - p.rebroadcastAt) >= 0) { p.rebroadcastPending = false; send(p); break; }
    // Abgleich fuer spaeter hinzukommende Knoten: alle 5 min (+Zufall) einen Aushang erneut senden
    if (!_posts.empty() && now - _lastSync > 300000UL + (esp_random() % 60000))
    {
        _lastSync = now;
        _syncIdx = (_syncIdx + 1) % _posts.size();
        send(_posts[_syncIdx]);
    }
    if (_dirty && now - _lastSave > 600000UL) save();
}

void BoardClient::load()
{
    if (!_fs || !_fs->exists(_path)) return;
    File f = _fs->open(_path, "r");
    if (!f) return;
    while (f.available() && _posts.size() < BOARD_MAX_POSTS)
    {
        String line = f.readStringUntil('\n');
        BoardPost p;
        uint32_t ttl;
        if (parse(line.c_str(), line.length(), p, ttl) && find(p.id) < 0)
        {
            p.receivedAt = millis();
            p.expiresAt = millis() + ttl * 60000UL;
            _posts.push_back(p);
        }
    }
    f.close();
    Serial.printf(" --> Schwarzes Brett: %u Aushaenge geladen\r\n", (unsigned)_posts.size());
}

void BoardClient::save()
{
    _lastSave = millis();
    _dirty = false;
    if (!_fs) return;
    File f = _fs->open(_path, "w");
    if (!f) return;
    for (auto &p : _posts)
        f.printf("%s|%s|%lu|%s|%s\n", p.id.c_str(), p.flags.c_str(), (unsigned long)remainingMin(p), p.name.c_str(), p.text.c_str());
    f.close();
}

String BoardClient::toJson()
{
    expire();
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (auto &p : _posts)
    {
        JsonObject o = arr.add<JsonObject>();
        o["id"] = p.id;
        o["pinned"] = p.flags == "P";
        o["name"] = p.name;
        o["text"] = p.text;
        o["ageMin"] = (millis() - p.receivedAt) / 60000UL;
        o["ttlMin"] = remainingMin(p);
    }
    String out;
    serializeJson(doc, out);
    return out;
}
