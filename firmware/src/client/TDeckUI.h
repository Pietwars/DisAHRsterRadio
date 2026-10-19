#pragma once

// T-Deck Plus experimental on-device UI. Only built with -DTDECK_SCREEN.
// Stable WiFi-only T-Deck and T-Watch profiles remain unchanged.
#if defined(LILYGO_TDECK) && defined(TDECK_SCREEN)

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include "BoardClient.h"
#include "../DisasterClient.h"

class TDeckUI : public DisasterClient {
public:
  void attachBoard(BoardClient *board) { board_ = board; }
  void beginDisplay();
  void loop() override;
  void receive(struct Datagram datagram, size_t len) override;
private:
  enum Page : uint8_t { CHAT = 0, QUICK = 1, BOARD = 2, MAP = 3 };
  Adafruit_ST7789 lcd_{BOARD_TFT_CS, BOARD_TFT_DC, -1};
  HardwareSerial gnss_{1};
  TinyGPSPlus gps_;
  BoardClient *board_ = nullptr;
  Page page_ = CHAT;
  String messages_[12];
  String draft_;
  uint8_t count_ = 0, selected_ = 0;
  uint8_t quickPending_ = 255;
  uint8_t chatScroll_ = 0;
  uint32_t confirmAt_ = 0, lastKey_ = 0, lastPaint_ = 0, lastPosition_ = 0;
  uint32_t lastMessageAt_ = 0;
  bool ready_ = false, dirty_ = true, autoFollow_ = true;
  double latitude_ = 0.0, longitude_ = 0.0;
  int zoom_ = 1;    // 0 = local, 1 = district, 2 = regional
  int viewX_ = 0, viewY_ = 0;

  void draw();
  void header();
  void bottom(const String &text);
  void text(int x, int y, const String &value, uint16_t color = 0xffff, uint8_t scale = 1);
  void drawChat();
  void drawQuick();
  void drawBoard();
  void drawMap();
  void movePage(int delta);
  void handleKey(char ch);
  void handleClick();
  void receiveChat(const char *src, size_t bytes);
  void sendChat(const String &value);
  void sendBoard(const String &value);
  void readKeyboard();
  void readTrackball();
  void readGps();
  void appendMessage(const String &value);
  static String readable(String input);
};
#endif
