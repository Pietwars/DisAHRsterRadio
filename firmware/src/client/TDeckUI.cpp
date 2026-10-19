#include "../../config.h"
#if defined(LILYGO_TDECK) && defined(TDECK_SCREEN)
#include "TDeckUI.h"
#include <SPI.h>
#include <math.h>

// Arduino FSPI=0 is a bus enum, not the S3 register peripheral number.
static_assert(SPI_PORT == 2, "TFT must address SPI2 registers on ESP32-S3");
static_assert(SPI_USER_REG(SPI_PORT) == DR_REG_SPI2_BASE + 0x10,
              "Invalid TFT SPI user register");

namespace {
constexpr uint16_t BG = 0x0862, PANEL = 0x10c4, WHITE_ = 0xffff;
constexpr uint16_t GREEN = 0x9e85, ORANGE = 0xfce0, BLUE = 0x4d7f;
constexpr uint8_t KEYBOARD_ADDRESS = 0x55;
constexpr int TRACK_UP = 3, TRACK_DOWN = 15, TRACK_LEFT = 1, TRACK_RIGHT = 2, TRACK_CLICK = 0;
const char *const tabs[] = {"CHAT", "HILFE", "BRETT", "KARTE"};
const char *const quickItems[] = {
  "Trinkwasser", "Strom / Laden", "Medikamente", "Verletzte Person",
  "Evakuierung", "Mir geht es gut", "Biete Hilfe an", "Strom vorhanden"
};
const char *const quickMessages[] = {
  "[HILFE] Trinkwasser benoetigt",
  "[HILFE] Strom zum Laden benoetigt",
  "[HILFE] Medikamente benoetigt",
  "[HILFE] Verletzte Person, Hilfe benoetigt",
  "[INFO] Evakuierung / Sammelpunkt",
  "[INFO] Mir geht es gut, bin hier erreichbar",
  "[ANGEBOT] Biete Hilfe an",
  "[ANGEBOT] Strom/Ladestation verfuegbar"
};
// Landmark overview while vector OSM-to-TFT map rendering is developed separately.
// Coordinates are indicative place centres, not navigation data.
struct Place { const char *name; double lat, lon; };
const Place places[] = {
  {"Heimersheim", 50.5426, 7.1761},
  {"Ahrweiler", 50.5432, 7.0949},
  {"Bad Neuenahr", 50.5449, 7.1377},
  {"Sinzig", 50.5440, 7.2520},
  {"Dernau", 50.5337, 7.0453},
  {"Altenahr", 50.5166, 6.9925}
};
}

String TDeckUI::readable(String input) {
  input.replace("ä","ae"); input.replace("ö","oe"); input.replace("ü","ue");
  input.replace("Ä","Ae"); input.replace("Ö","Oe"); input.replace("Ü","Ue");
  input.replace("ß","ss");
  return input;
}

void TDeckUI::beginDisplay() {
  // LCD, SD and SX1262 share the same SPI bus: deselect the other devices.
  pinMode(LORA_CS, OUTPUT);
  pinMode(BOARD_SD_CS, OUTPUT);
  pinMode(BOARD_TFT_CS, OUTPUT);
  digitalWrite(LORA_CS, HIGH);
  digitalWrite(BOARD_SD_CS, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);
  pinMode(BOARD_TFT_BL, OUTPUT);
  digitalWrite(BOARD_TFT_BL, LOW);
  pinMode(BOARD_TFT_DC, OUTPUT);
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI);
  Serial.printf("[TDECK-UI] before lcd.init: SPI_PORT=%d USER_REG=0x%08lx heap=%u min=%u stack=%u\n",
                SPI_PORT, (unsigned long)SPI_USER_REG(SPI_PORT), ESP.getFreeHeap(),
                ESP.getMinFreeHeap(), (unsigned)uxTaskGetStackHighWaterMark(nullptr));
  Serial.flush();
  lcd_.init();
  Serial.println("[TDECK-UI] lcd.init OK");
  Serial.flush();
  lcd_.setRotation(1);
  lcd_.setTextWrap(false);
  lcd_.fillScreen(BG);
  Serial.println("[TDECK-UI] first fill OK");
  digitalWrite(BOARD_TFT_BL, HIGH);

  Wire.begin(18, 8);
  Wire.setClock(100000);
  for (int pin : {TRACK_UP, TRACK_DOWN, TRACK_LEFT, TRACK_RIGHT, TRACK_CLICK}) pinMode(pin, INPUT_PULLUP);
  pinMode(46, INPUT_PULLUP);
  // GPS UART1: ESP32 RX43, TX44; LilyGO documentation default 9600 baud. No LoRa beacon.
  gnss_.begin(9600, SERIAL_8N1, 43, 44);
  ready_ = true;
  dirty_ = true;
  Serial.println("[TDECK-UI] ST7789 display, keyboard, trackball and GPS started");
}

void TDeckUI::text(int x, int y, const String &value, uint16_t color, uint8_t scale) {
  lcd_.setTextColor(color);
  lcd_.setTextSize(scale);
  lcd_.setCursor(x, y);
  lcd_.print(readable(value));
}

void TDeckUI::header() {
  lcd_.fillRect(0, 0, 320, 28, PANEL);
  for (uint8_t i = 0; i < 4; ++i) {
    if (i == page_) lcd_.fillRoundRect(2+i*80, 3, 76, 22, 4, GREEN);
    text(11+i*80, 10, tabs[i], i==page_?BG:WHITE_);
  }
}

void TDeckUI::bottom(const String &value) {
  lcd_.fillRect(0, 218, 320, 22, PANEL);
  text(5, 225, value.substring(0, 52), WHITE_);
}

void TDeckUI::drawChat() {
  lcd_.fillRect(0,28,320,190,BG);
  int y = 34;
  uint8_t start = count_>8+chatScroll_?count_-8-chatScroll_:0;
  uint8_t stop = min((int)count_,(int)start+8);
  for (uint8_t i=start; i<stop; ++i) {
    String s = readable(messages_[i]);
    while (s.length() && y < 182) {
      text(6,y,s.substring(0,50),i == count_-1?GREEN:WHITE_);
      s.remove(0, min((int)s.length(), 50));
      y += 17;
    }
  }
  lcd_.fillRect(0,187,320,29,PANEL);
  text(5,196,"> "+draft_.substring(max(0,(int)draft_.length()-43)),WHITE_);
  bottom("Tastatur: tippen, Enter=senden | Links/Rechts=Tab");
}

void TDeckUI::drawQuick() {
  lcd_.fillRect(0,28,320,190,BG);
  for (uint8_t i=0;i<8;++i) {
    int x = (i%2)*160+4, y = 32+(i/2)*45;
    uint16_t c = (i==selected_?ORANGE:PANEL);
    lcd_.fillRoundRect(x,y,152,40,4,c);
    text(x+8,y+15,quickItems[i],i==selected_?BG:WHITE_);
  }
  bottom(quickPending_==selected_ && millis()-confirmAt_<3500 ? "Erneut klicken zum Senden" : "Trackball: waehlen | Klick: bestaetigen");
}

void TDeckUI::drawBoard() {
  lcd_.fillRect(0,28,320,190,BG);
  size_t n=board_?board_->count():0;
  text(6,33,String("Aushaenge: ")+(unsigned)n,GREEN);
  if (!n) text(6,61,"Noch keine Aushaenge");
  else {
    if (selected_>=n) selected_=n-1;
    const BoardPost *p=board_->at(selected_);
    if (p) {
      text(6,59,String(selected_+1)+"/"+String(n)+"  "+p->name,ORANGE);
      String s=readable(p->text);
      int y=80;
      while(s.length()&&y<160) { text(8,y,s.substring(0,48));s.remove(0,min(48,(int)s.length())); y+=16; }
      int32_t remainingMs = (int32_t)(p->expiresAt - millis());
      text(8,164,String("Noch ")+(remainingMs>0 ? (uint32_t)(remainingMs/60000UL) : 0UL)+" min",GREEN);
    }
  }
  lcd_.fillRect(0,186,320,30,PANEL);
  text(5,195,"> "+draft_.substring(max(0,(int)draft_.length()-43)));
  bottom("Hoch/Runter=lesen | Text+Enter=neuer Aushang");
}

void TDeckUI::drawMap() {
  lcd_.fillRect(0,28,320,190,BG);
  if(!gps_.location.isValid()||gps_.location.age()>10000) {
    text(8,40,"GPS: Warte auf Satelliten...",ORANGE);
    text(8,60,"Im Freien positionieren");
    text(8,85,"Noch keine automatische",GREEN);
    text(8,101,"Zentrierung moeglich.");
    bottom("GPS ueber UART1 | Satelliten: "+String(gps_.satellites.value()));
    return;
  }
  double baseLat=latitude_+viewY_*0.005, baseLon=longitude_+viewX_*0.005;
  const double scale=zoom_==0?24000.0:(zoom_==1?8000.0:2500.0);
  for(int x=15;x<320;x+=40)lcd_.drawFastVLine(x,30,168,PANEL);
  for(int y=40;y<200;y+=40)lcd_.drawFastHLine(0,y,320,PANEL);
  for(const auto &p:places) {
    int x=160+(int)round((p.lon-baseLon)*cos(baseLat*0.01745329)*scale);
    int y=120-(int)round((p.lat-baseLat)*scale);
    if(x<8||x>305||y<43||y>190) continue;
    lcd_.fillCircle(x,y,3,BLUE);
    text(x+4,y-7,p.name,WHITE_);
  }
  int meX=160-(int)round((baseLon-longitude_)*cos(baseLat*0.01745329)*scale);
  int meY=120+(int)round((baseLat-latitude_)*scale);
  if(meX>5&&meX<315&&meY>39&&meY<191) {lcd_.fillCircle(meX,meY,5,GREEN);lcd_.drawCircle(meX,meY,8,WHITE_);}
  text(6,201,String(latitude_,5)+", "+String(longitude_,5),GREEN);
  bottom(String(zoom_==0?"Nah":zoom_==1?"Ort":"Region")+" | +/- zoom | 0 zentriert");
}

void TDeckUI::draw() {
  if(!ready_)return;
  header();
  switch(page_) {case CHAT:drawChat();break;case QUICK:drawQuick();break;case BOARD:drawBoard();break;case MAP:drawMap();break;}
  dirty_=false;
  lastPaint_=millis();
}

void TDeckUI::appendMessage(const String &s) {
  if(count_==12) {for(int i=1;i<12;i++)messages_[i-1]=messages_[i]; count_=11;}
  messages_[count_++]=s.substring(0,160);
  lastMessageAt_=millis();
  chatScroll_=0;
  dirty_=true;
}

void TDeckUI::receiveChat(const char *src, size_t bytes) {
  String s;
  for(size_t i=0;i<bytes && i<180;++i) {char c=src[i];if(!c)break;if(c!='\r'&&c!='\n')s+=c;}
  if(s.length() && s[0]!='~')appendMessage(s);
}

void TDeckUI::receive(struct Datagram datagram,size_t len) {
  if(len<DATAGRAM_HEADER+4)return;
  size_t bytes=len-DATAGRAM_HEADER;
  if(datagram.type=='c' && datagram.message[2]=='c' && datagram.message[3]=='|')receiveChat((char*)datagram.message+4,bytes-4);
  else if(datagram.type=='b')dirty_=true;
}

void TDeckUI::sendChat(const String &value) {
  if(!server||!value.length())return;
  String body="<TDeck> "+value;
  if(gps_.location.isValid() && gps_.location.age()<10000 && value.startsWith("["))body+=" @ "+String(latitude_,5)+","+String(longitude_,5);
  if(body.length()>MESSAGE_LENGTH-5)return;
  Datagram d={};
  memset(d.destination,0xff,ADDR_LENGTH);
  d.type='c';
  d.message[2]='c';d.message[3]='|';
  memcpy((char*)d.message+4,body.c_str(),body.length());
  server->transmit(this,d,DATAGRAM_HEADER+4+body.length());
  appendMessage(body);
}

void TDeckUI::sendBoard(const String &value) {
  if(!server||!value.length())return;
  String body=value.substring(0,115);
  char id[9];snprintf(id,sizeof(id),"%08lx",(unsigned long)esp_random());
  String payload=String(id)+"|-|1440|TDeck|"+body;
  if(payload.length()>MESSAGE_LENGTH-5)return;
  Datagram d={};
  memset(d.destination,0xff,ADDR_LENGTH);
  d.type='B';d.message[2]='b';d.message[3]='|';
  memcpy((char*)d.message+4,payload.c_str(),payload.length());
  server->transmit(this,d,DATAGRAM_HEADER+4+payload.length());
  dirty_=true;
}

void TDeckUI::movePage(int delta) {
  int n=((int)page_+delta+4)%4;
  page_=(Page)n;
  selected_=0;
  chatScroll_=0;
  draft_="";
  quickPending_=255;
  dirty_=true;
}

void TDeckUI::handleClick() {
  if(page_==CHAT) {if(draft_.length()){sendChat(draft_);draft_="";}}
  else if(page_==BOARD) {if(draft_.length()){sendBoard(draft_);draft_="";}}
  else if(page_==QUICK) {
    if(quickPending_==selected_&&millis()-confirmAt_<3500){
      sendChat(quickMessages[selected_]);
      quickPending_=255;
      page_=CHAT;
    } else {quickPending_=selected_;confirmAt_=millis();}
  }else if(page_==MAP){zoom_=(zoom_+1)%3;viewX_=viewY_=0;}
  dirty_=true;
}

void TDeckUI::handleKey(char ch) {
  if(ch=='[') {movePage(-1);return;}
  if(ch==']') {movePage(1);return;}
  if(page_==MAP){
    if(ch=='+'||ch=='=')zoom_=max(0,zoom_-1);
    if(ch=='-')zoom_=min(2,zoom_+1);
    if(ch=='0')viewX_=viewY_=0;
    if(ch=='w'||ch=='W')--viewY_;
    if(ch=='s'||ch=='S')++viewY_;
    if(ch=='a'||ch=='A')--viewX_;
    if(ch=='d'||ch=='D')++viewX_;
    dirty_=true;return;
  }
  if(page_==QUICK)return;
  if(ch=='\r'||ch=='\n') {handleClick();return;}
  if(ch==8||ch==127) {if(draft_.length())draft_.remove(draft_.length()-1);dirty_=true;return;}
  if(ch>=32&&ch<=126 && draft_.length()<112){draft_+=(char)ch;dirty_=true;}
}

void TDeckUI::readKeyboard(){
  // The keyboard asserts GPIO46 when a key is ready; avoid constant I2C traffic.
  if(digitalRead(46) != LOW) return;
  Wire.requestFrom((uint8_t)KEYBOARD_ADDRESS,(uint8_t)1);
  if(Wire.available()){char ch=(char)Wire.read();if(ch)handleKey(ch);}
}

void TDeckUI::readTrackball(){
  if(millis()-lastKey_<170)return;
  int action=0;
  if(!digitalRead(TRACK_LEFT))action=1;
  else if(!digitalRead(TRACK_RIGHT))action=2;
  else if(!digitalRead(TRACK_UP))action=3;
  else if(!digitalRead(TRACK_DOWN))action=4;
  else if(!digitalRead(TRACK_CLICK))action=5;
  if(!action)return;
  lastKey_=millis();
  if(action==1)movePage(-1);
  else if(action==2)movePage(1);
  else if(action==5)handleClick();
  else if(page_==QUICK) {selected_=(selected_+(action==3?7:1))%8;quickPending_=255;dirty_=true;}
  else if(page_==CHAT) {const int maxScroll=max(0,(int)count_-8);chatScroll_=constrain((int)chatScroll_+(action==3?1:-1),0,maxScroll);dirty_=true;}
  else if(page_==BOARD){const size_t n=board_?board_->count():0;if(n){selected_=(selected_+n+(action==3?n-1:1))%n;dirty_=true;}}
  else if(page_==MAP){zoom_=constrain(zoom_+(action==3?-1:1),0,2);dirty_=true;}
}

void TDeckUI::readGps() {
  while(gnss_.available())gps_.encode(gnss_.read());
  if(gps_.location.isValid()&&gps_.location.age()<10000){
    if(latitude_!=gps_.location.lat()||longitude_!=gps_.location.lng()){
      latitude_=gps_.location.lat();longitude_=gps_.location.lng();dirty_=true;
    }
  }
  if(millis()-lastPosition_>30000){lastPosition_=millis();if(page_==MAP)dirty_=true;}
}

void TDeckUI::loop() {
  if(!ready_)return;
  readGps();readKeyboard();readTrackball();
  if(quickPending_!=255&&millis()-confirmAt_>3500){quickPending_=255;dirty_=true;}
  if(dirty_&&millis()-lastPaint_>130)draw();
}
#endif
