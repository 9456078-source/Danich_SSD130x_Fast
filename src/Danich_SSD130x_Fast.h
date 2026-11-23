// Danich_SSD130x_Fast v2.0.0 — Унифицированная библиотека 2-в-1 (OLED_NO_BUFFER / OLED_BUFFER)
// Для дисплеев SSD1306 / SSD1309 (I2C через microWire)
#pragma once

#ifndef DANICH_SSD130X_FAST_H
#define DANICH_SSD130X_FAST_H

#include <Arduino.h>
#if defined(__AVR__)
#include <microWire.h>
#else
#include <Wire.h>
#endif
#include "DefaultFonts.h"

// --- Режимы работы буфера ---
constexpr uint8_t OLED_NO_BUFFER = 0; // 0 байт ОЗУ, прямой потоковый вывод
constexpr uint8_t OLED_BUFFER = 1;    // 1024 байт ОЗУ, попиксельная графика + умный update()

// --- Тип контроллера дисплея ---
enum OledChip : uint8_t
{
  OLED_1306 = 0,
  OLED_1309 = 1
};

// --- Настройки позиционирования текста ---
constexpr uint8_t OLED_T = 250; // Выравнивание по верхнему краю
constexpr uint8_t OLED_B = 251; // Выравнивание по нижнему краю
constexpr uint8_t OLED_L = 252; // Выравнивание по левому краю
constexpr uint8_t OLED_C = 253; // Выравнивание по центру
constexpr uint8_t OLED_R = 254; // Выравнивание по правому краю
constexpr uint8_t OLED_N = 255; // Оставить текущее значение

// --- Кодировки текста ---
constexpr uint8_t TXT_CP866 = 0;
constexpr uint8_t TXT_UTF8 = 1;
constexpr uint8_t TXT_WIN1251 = 2;

// --- I2C Адрес дисплея ---
constexpr uint8_t SSD1306_ADDRESS = 0x3C;

// --- Базовые команды SSD1306 / SSD1309 ---
constexpr uint8_t SSD1306_SET_CONTRAST = 0x81;
constexpr uint8_t SSD1306_RAM_ON = 0xA4;
constexpr uint8_t SSD1306_RAM_OFF = 0xA5;
constexpr uint8_t SSD1306_INVERT_OFF = 0xA6;
constexpr uint8_t SSD1306_INVERT_ON = 0xA7;
constexpr uint8_t SSD1306_DISPLAY_OFF = 0xAE;
constexpr uint8_t SSD1306_DISPLAY_ON = 0xAF;
constexpr uint8_t SSD1306_ADDR_MODE = 0x20;
constexpr uint8_t SSD1306_ADDR_COLUMN = 0x21;
constexpr uint8_t SSD1306_ADDR_PAGE = 0x22;
constexpr uint8_t SSD1306_SET_START_LINE = 0x40;
constexpr uint8_t SSD1306_SET_REMAP_R_TO_L = 0xA0;
constexpr uint8_t SSD1306_SET_REMAP_L_TO_R = 0xA1;
constexpr uint8_t SSD1306_SET_MULTIPLEX_RATIO = 0xA8;
constexpr uint8_t SSD1306_SET_REMAP_D_TO_T = 0xC0;
constexpr uint8_t SSD1306_SET_REMAP_T_TO_D = 0xC8;
constexpr uint8_t SSD1306_SET_DISPLAY_OFFSET = 0xD3;
constexpr uint8_t SSD1306_SET_COM_PINS = 0xDA;
constexpr uint8_t SSD1306_SET_DISPLAY_CLOCK = 0xD5;
constexpr uint8_t SSD1306_SET_PRECHARGE_PERIOD = 0xD9;
constexpr uint8_t SSD1306_SET_VCOM_DESELECT = 0xDB;
constexpr uint8_t SSD1306_CHARGE_DCDC_PUMP = 0x8D;

constexpr uint8_t SSD1306_COMMAND = 0x00;
constexpr uint8_t SSD1306_DATA = 0x40;

template <uint8_t _BUFF = OLED_NO_BUFFER>
class Danich_SSD130x_Fast
{
public:
  explicit Danich_SSD130x_Fast(uint8_t address = SSD1306_ADDRESS)
  {
    if (address == 0x78)
      address = 0x3C;
    if (address == 0x7A)
      address = 0x3D;
    dispAddr = address;
  }

  // Инициализация (по умолчанию OLED_1309; на ESP32/ESP8266 можно указать пины sda, scl)
  void begin(OledChip chip = OLED_1309, int8_t sda = -1, int8_t scl = -1) { _begin(chip, sda, scl); }

  // --- Управление экраном и буфером ---
  void clrScr(bool fillWhite = false);
  void fillScr(bool fillWhite = true) { clrScr(fillWhite); }
  void invScr(bool invert = true);
  void flipH(bool flip = true);
  void flipV(bool flip = true);
  void displayOff();
  void displayOn();
  void setContrast(uint8_t val);

  // --- Только для режима OLED_BUFFER (в NO_BUFFER компилятор вырежет) ---
  void update();                                               // Умное обновление только изменённой области
  void update(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1); // Выборочное обновление области (в пикселях)
  void autoUpdate(bool enable = true) { flgAutoUpd = enable; }

  // --- Настройки текста и шрифта ---
  void invText(bool invert = true) { insFont.inverted = invert; }
  void setFont(const uint8_t *font);
  void setScale(uint8_t scale = 1)
  {
    if (scale == 3)
      scale = 4;
    txtScale = (scale < 1) ? 1 : (scale > 4 ? 4 : scale);
  }
  void setCoding(uint8_t codingType = TXT_UTF8) { codingName = codingType; }

  uint8_t getFontWidth() const { return insFont.width * txtScale; }
  uint8_t getFontHeight() const { return (_BUFF ? insFont.height : (insFont.height >> 3)) * txtScale; }

  // Явный выбор формата координат для текста и битмапов: true = страницы (0..7), false = пиксели (0..63)
  void setPageCoord(bool usePages) { flgPageCoord = usePages; }

  // Перевод координаты row в нужный формат для текущего режима и высоты шрифта
  uint8_t _resolveY(uint8_t row) const
  {
    if (row >= OLED_T)
      return row;
    uint8_t totalPages = (insFont.height >> 3) * txtScale;
    if (_BUFF)
    {
      if (!flgPageCoord)
        return row; // В BUFFER используются пиксели 0..63
      int8_t topPage = (int8_t)row + 1 - (int8_t)totalPages;
      return (topPage < 0) ? 0 : (uint8_t)(topPage << 3);
    }
    else
    {
      if (flgPageCoord)
        return row; // В NO_BUFFER используются страницы 0..7
      return (row >> 3) + totalPages - 1;
    }
  }

  // --- Управление курсором ---
  void setCursor(uint8_t col, uint8_t row)
  {
    if (col < 128)
      numX = col;
    if (row < 64)
      numY = _resolveY(row);
  }

  void setCursorShift(int8_t x = 0, int8_t y = 0)
  {
    int16_t nx = (int16_t)numX + x;
    int16_t ny = (int16_t)numY + y;
    uint8_t maxY = _BUFF ? 63 : 7;
    numX = (nx < 0) ? 0 : (nx > 127 ? 127 : (uint8_t)nx);
    numY = (ny < 0) ? 0 : (ny > maxY ? maxY : (uint8_t)ny);
  }

  // --- Графика (работает в обоих режимах, в BUFFER попиксельно + заливка и круги) ---
  void drawPixel(int16_t x, int16_t y, bool color = true);
  bool getPixel(int16_t x, int16_t y) const;
  void drawHLine(uint8_t x, uint8_t y, uint8_t len, uint8_t dashLen = 1, uint8_t gapLen = 0);
  void drawVLine(uint8_t x, uint8_t y, uint8_t len, uint8_t dashLen = 1, uint8_t gapLen = 0);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color = true);
  void drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t dashLen = 1, uint8_t gapLen = 0);
  void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color = true);
  void drawCircle(int16_t x0, int16_t y0, uint8_t r, bool fill = false, bool color = true);
  void drawBitmap(uint8_t col, uint8_t row, const uint8_t *bitmap, uint8_t w, uint8_t h, bool invert = false);

  // --- Бегущая строка ---
  uint16_t printScroll(char *str, uint8_t row, int16_t offsetX, uint8_t winX = 0, uint8_t winW = 128);
  uint16_t printScroll(const char *str, uint8_t row, int16_t offsetX, uint8_t winX = 0, uint8_t winW = 128);
  uint16_t printScroll(const __FlashStringHelper *txt, uint8_t row, int16_t offsetX, uint8_t winX = 0, uint8_t winW = 128);

  void resetTicker() { tickerX = 32767; }
  void printTicker(char *str, uint8_t row, uint16_t speedMs = 25, uint8_t winX = 0, uint8_t winW = 128);
  void printTicker(const char *str, uint8_t row, uint16_t speedMs = 25, uint8_t winX = 0, uint8_t winW = 128);
  void printTicker(const __FlashStringHelper *txt, uint8_t row, uint16_t speedMs = 25, uint8_t winX = 0, uint8_t winW = 128);

  // --- Методы вывода (print) ---
#ifdef RENESAS_CORTEX_M4
  void print(int val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
  void print(unsigned int val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
#else
  void print(int8_t val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
  void print(uint8_t val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
  void print(int16_t val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
  void print(uint16_t val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
#endif

  void print(int32_t val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
  void print(uint32_t val, uint8_t col = OLED_N, uint8_t row = OLED_N, uint8_t base = DEC);
  void print(double val, uint8_t col = OLED_N, uint8_t row = OLED_N, int decimals = 2);

  void print(char *str, uint8_t col = OLED_N, uint8_t row = OLED_N);
  void print(const char *str, uint8_t col = OLED_N, uint8_t row = OLED_N);
  void print(const String &str, uint8_t col = OLED_N, uint8_t row = OLED_N);
  void print(char c, uint8_t x = OLED_N, uint8_t y = OLED_N);
  void print(const __FlashStringHelper *txt, uint8_t x = OLED_N, uint8_t y = OLED_N);
  void printPadZero(uint16_t val, uint8_t width, uint8_t x = OLED_N, uint8_t y = OLED_N);

  uint8_t numX = 0;
  uint8_t numY = 0;

private:
  uint8_t dispAddr;
  uint8_t codingName = TXT_UTF8;
  uint8_t txtScale = 1;

  uint32_t tTicker = 0;
  int16_t tickerX = 32767;

  // Видеобуфер (0 байт в OLED_NO_BUFFER, 1024 байт в OLED_BUFFER)
  uint8_t arrBuffer[_BUFF ? 1024 : 0];
  uint8_t dirtyX0 = 128, dirtyX1 = 0;
  uint8_t dirtyP0 = 8, dirtyP1 = 0;
  bool flgAutoUpd = false;
  bool flgPageCoord = (_BUFF == 0); // По умолчанию: в NO_BUFFER страницы (true), в BUFFER пиксели (false)

  struct
  {
    const uint8_t *font = nullptr;
    uint8_t width = 0;
    uint8_t height = 0;
    uint8_t firstSymbol = 0;
    uint8_t sumSymbol = 0;
    bool inverted = false;
    bool isFontSet = false;
    uint8_t startSpace[3] = {0};
    uint8_t sumSpace[3] = {0};
  } insFont;

  void _begin(OledChip chip, int8_t sda, int8_t scl);
  void _setPixelBuf(int16_t x, int16_t y, bool color);
  void _markDirty(uint8_t x0, uint8_t x1, uint8_t p0, uint8_t p1);
  void _print(char *str, uint8_t col = OLED_N, uint8_t row = OLED_N);
  uint16_t _printScroll(char *str, uint8_t row, int16_t offsetX, uint8_t winX, uint8_t winW);
  void _printTicker(char *str, uint8_t row, uint8_t winX, uint8_t winW);
  char *_codingCP866(char *strUTF8);
  uint16_t _getGlyphIndex(uint8_t ch) const;

  void _setWindow(uint8_t col0, uint8_t col1, uint8_t page0, uint8_t page1);
  void _sendCommand(uint8_t cmd);
  void _sendCommand2(uint8_t cmd, uint8_t arg);
};

#include "Danich_SSD130x_Fast.tpp"

// Удобные псевдонимы без угловых скобок
using DanichOLED = Danich_SSD130x_Fast<OLED_NO_BUFFER>;
using DanichOLED_Buf = Danich_SSD130x_Fast<OLED_BUFFER>;

#endif // DANICH_SSD130X_FAST_H
