// Профили инициализации
static const uint8_t init_cmds_1309[] PROGMEM = {
    SSD1306_DISPLAY_OFF,
    SSD1306_SET_DISPLAY_CLOCK, 0x40,
    SSD1306_SET_MULTIPLEX_RATIO, 0x3F,
    SSD1306_SET_DISPLAY_OFFSET, 0x00,
    SSD1306_SET_START_LINE | 0,
    SSD1306_ADDR_MODE, 0x00,
    SSD1306_SET_REMAP_L_TO_R,
    SSD1306_SET_REMAP_T_TO_D,
    SSD1306_SET_COM_PINS, 0x12,
    SSD1306_SET_CONTRAST, 0xCF,
    SSD1306_SET_PRECHARGE_PERIOD, 0x18,
    SSD1306_SET_VCOM_DESELECT, 0x00,
    SSD1306_RAM_ON,
    SSD1306_INVERT_OFF,
    SSD1306_DISPLAY_ON};

static const uint8_t init_cmds_1306[] PROGMEM = {
    SSD1306_DISPLAY_OFF,
    SSD1306_SET_DISPLAY_CLOCK, 0x80,
    SSD1306_SET_MULTIPLEX_RATIO, 0x3F,
    SSD1306_SET_DISPLAY_OFFSET, 0x00,
    SSD1306_SET_START_LINE | 0,
    SSD1306_CHARGE_DCDC_PUMP, 0x14,
    SSD1306_ADDR_MODE, 0x00,
    SSD1306_SET_REMAP_L_TO_R,
    SSD1306_SET_REMAP_T_TO_D,
    SSD1306_SET_COM_PINS, 0x12,
    SSD1306_SET_CONTRAST, 0x8F,
    SSD1306_SET_PRECHARGE_PERIOD, 0xF1,
    SSD1306_SET_VCOM_DESELECT, 0x40,
    SSD1306_RAM_ON,
    SSD1306_INVERT_OFF,
    SSD1306_DISPLAY_ON};

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_begin(OledChip chip, int8_t sda, int8_t scl)
{
#if defined(ESP32) || defined(ESP8266)
  if (sda >= 0 && scl >= 0)
    Wire.begin(sda, scl);
  else
    Wire.begin();
#else
  (void)sda;
  (void)scl;
  Wire.begin();
#endif

  const uint8_t *table = (chip == OLED_1309) ? init_cmds_1309 : init_cmds_1306;
  uint8_t size = (chip == OLED_1309) ? sizeof(init_cmds_1309) : sizeof(init_cmds_1306);

  Wire.beginTransmission(dispAddr);
  Wire.write(SSD1306_COMMAND);
  for (uint8_t i = 0; i < size; i++)
  {
    Wire.write(pgm_read_byte(&table[i]));
  }
  Wire.endTransmission();

  clrScr(false);
  if (_BUFF)
    update(0, 0, 127, 63);
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::clrScr(bool color)
{
  uint8_t fillByte = color ? 0xFF : 0x00;
  if (_BUFF)
  {
    memset(arrBuffer, fillByte, 1024);
    _markDirty(0, 127, 0, 7);
    if (flgAutoUpd)
      update();
  }
  else
  {
    _setWindow(0, 127, 0, 7);
    Wire.beginTransmission(dispAddr);
    Wire.write(SSD1306_DATA);
    for (uint16_t i = 0; i < 1024; i++)
    {
#if !defined(__AVR__)
      if (i > 0 && (i & 63) == 0)
      {
        Wire.endTransmission();
        Wire.beginTransmission(dispAddr);
        Wire.write(SSD1306_DATA);
      }
#endif
      Wire.write(fillByte);
    }
    Wire.endTransmission();
  }
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_markDirty(uint8_t x0, uint8_t x1, uint8_t p0, uint8_t p1)
{
  if (_BUFF)
  {
    if (x0 < dirtyX0)
      dirtyX0 = x0;
    if (x1 > dirtyX1)
      dirtyX1 = x1;
    if (p0 < dirtyP0)
      dirtyP0 = p0;
    if (p1 > dirtyP1)
      dirtyP1 = p1;
  }
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::update()
{
  if (_BUFF)
  {
    if (dirtyX0 > dirtyX1 || dirtyP0 > dirtyP1)
      return;
    _setWindow(dirtyX0, dirtyX1, dirtyP0, dirtyP1);
    Wire.beginTransmission(dispAddr);
    Wire.write(SSD1306_DATA);
#if !defined(__AVR__)
    uint8_t txCount = 0;
#endif
    for (uint8_t p = dirtyP0; p <= dirtyP1; p++)
    {
      uint16_t rowBase = (uint16_t)p << 7;
      for (uint8_t x = dirtyX0; x <= dirtyX1; x++)
      {
#if !defined(__AVR__)
        if (txCount >= 64)
        {
          Wire.endTransmission();
          Wire.beginTransmission(dispAddr);
          Wire.write(SSD1306_DATA);
          txCount = 0;
        }
        txCount++;
#endif
        Wire.write(arrBuffer[rowBase + x]);
      }
    }
    Wire.endTransmission();

    dirtyX0 = 128;
    dirtyX1 = 0;
    dirtyP0 = 8;
    dirtyP1 = 0;
  }
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::update(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  if (_BUFF)
  {
    if (x0 > 127)
      x0 = 127;
    if (x1 > 127)
      x1 = 127;
    if (y0 > 63)
      y0 = 63;
    if (y1 > 63)
      y1 = 63;
    if (x0 > x1)
    {
      uint8_t t = x0;
      x0 = x1;
      x1 = t;
    }
    if (y0 > y1)
    {
      uint8_t t = y0;
      y0 = y1;
      y1 = t;
    }

    uint8_t p0 = y0 >> 3;
    uint8_t p1 = y1 >> 3;
    _setWindow(x0, x1, p0, p1);
    Wire.beginTransmission(dispAddr);
    Wire.write(SSD1306_DATA);
#if !defined(__AVR__)
    uint8_t txCount = 0;
#endif
    for (uint8_t p = p0; p <= p1; p++)
    {
      uint16_t rowBase = (uint16_t)p << 7;
      for (uint8_t x = x0; x <= x1; x++)
      {
#if !defined(__AVR__)
        if (txCount >= 64)
        {
          Wire.endTransmission();
          Wire.beginTransmission(dispAddr);
          Wire.write(SSD1306_DATA);
          txCount = 0;
        }
        txCount++;
#endif
        Wire.write(arrBuffer[rowBase + x]);
      }
    }
    Wire.endTransmission();
  }
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::invScr(bool invert) { _sendCommand(SSD1306_INVERT_OFF + (invert ? 1 : 0)); }

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::flipH(bool flip) { _sendCommand(flip ? SSD1306_SET_REMAP_R_TO_L : SSD1306_SET_REMAP_L_TO_R); }

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::flipV(bool flip) { _sendCommand(flip ? SSD1306_SET_REMAP_D_TO_T : SSD1306_SET_REMAP_T_TO_D); }

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::displayOff() { _sendCommand(SSD1306_DISPLAY_OFF); }

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::displayOn() { _sendCommand(SSD1306_DISPLAY_ON); }

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::setContrast(uint8_t val) { _sendCommand2(SSD1306_SET_CONTRAST, val); }
//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_setPixelBuf(int16_t x, int16_t y, bool color)
{
  if (_BUFF)
  {
    if (x < 0 || x > 127 || y < 0 || y > 63)
      return;
    uint8_t ux = (uint8_t)x;
    uint8_t uy = (uint8_t)y;
    uint8_t p = uy >> 3;
    uint16_t idx = ((uint16_t)p << 7) + ux;
    uint8_t mask = (uint8_t)(1U << (uy & 7));
    if (color)
      arrBuffer[idx] |= mask;
    else
      arrBuffer[idx] &= ~mask;
    _markDirty(ux, ux, p, p);
  }
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::drawPixel(int16_t x, int16_t y, bool color)
{
  if (x < 0 || x > 127 || y < 0 || y > 63)
    return;
  if (_BUFF)
  {
    _setPixelBuf(x, y, color);
    if (flgAutoUpd)
      update();
  }
  else
  {
    uint8_t ux = (uint8_t)x;
    uint8_t page = (uint8_t)y >> 3;
    _setWindow(ux, ux, page, page);
    Wire.beginTransmission(dispAddr);
    Wire.write(SSD1306_DATA);
    Wire.write(color ? (uint8_t)(1U << (y & 7)) : 0x00);
    Wire.endTransmission();
  }
}

template <uint8_t _BUFF>
bool Danich_SSD130x_Fast<_BUFF>::getPixel(int16_t x, int16_t y) const
{
  if (_BUFF)
  {
    if (x < 0 || x > 127 || y < 0 || y > 63)
      return false;
    return (arrBuffer[(((uint16_t)(y >> 3)) << 7) + x] & (1U << (y & 7))) != 0;
  }
  return false;
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::drawHLine(uint8_t x, uint8_t y, uint8_t len, uint8_t dashLen, uint8_t gapLen)
{
  if (x >= 128 || y >= 64 || len == 0)
    return;
  if (x + len > 128)
    len = 128 - x;
  if (dashLen == 0)
    dashLen = 1;

  uint8_t period = dashLen + gapLen;
  uint8_t step = 0;

  if (_BUFF)
  {
    for (uint8_t i = 0; i < len; i++)
    {
      if (gapLen == 0 || step < dashLen)
        _setPixelBuf(x + i, y, true);
      if (gapLen > 0 && ++step >= period)
        step = 0;
    }
    if (flgAutoUpd)
      update();
  }
  else
  {
    uint8_t page = y >> 3;
    uint8_t mask = (uint8_t)(1U << (y & 7));
    _setWindow(x, x + len - 1, page, page);
    Wire.beginTransmission(dispAddr);
    Wire.write(SSD1306_DATA);
    for (uint8_t i = 0; i < len; i++)
    {
      Wire.write((gapLen == 0 || step < dashLen) ? mask : 0x00);
      if (gapLen > 0 && ++step >= period)
        step = 0;
    }
    Wire.endTransmission();
  }
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::drawVLine(uint8_t x, uint8_t y, uint8_t len, uint8_t dashLen, uint8_t gapLen)
{
  if (x >= 128 || y >= 64 || len == 0)
    return;
  if (y + len > 64)
    len = 64 - y;
  if (dashLen == 0)
    dashLen = 1;

  uint8_t period = dashLen + gapLen;

  if (_BUFF)
  {
    uint8_t step = 0;
    for (uint8_t i = 0; i < len; i++)
    {
      if (gapLen == 0 || step < dashLen)
        _setPixelBuf(x, y + i, true);
      if (gapLen > 0 && ++step >= period)
        step = 0;
    }
    if (flgAutoUpd)
      update();
  }
  else
  {
    uint8_t yEnd = y + len - 1;
    uint8_t pageStart = y >> 3;
    uint8_t pageEnd = yEnd >> 3;
    _setWindow(x, x, pageStart, pageEnd);
    Wire.beginTransmission(dispAddr);
    Wire.write(SSD1306_DATA);

    for (uint8_t p = pageStart; p <= pageEnd; p++)
    {
      uint8_t pageByte = 0x00;
      uint8_t bitStart = (p == pageStart) ? (y & 7) : 0;
      uint8_t bitEnd = (p == pageEnd) ? (yEnd & 7) : 7;

      for (uint8_t b = bitStart; b <= bitEnd; b++)
      {
        uint8_t offset = (uint8_t)(((p << 3) + b) - y);
        if (gapLen == 0 || (offset % period) < dashLen)
          pageByte |= (uint8_t)(1U << b);
      }
      Wire.write(pageByte);
    }
    Wire.endTransmission();
  }
}
//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color)
{
  if (_BUFF)
  {
    bool prevUpd = flgAutoUpd;
    flgAutoUpd = false;

    int16_t dx = abs(x1 - x0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = -abs(y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    for (;;)
    {
      _setPixelBuf(x0, y0, color);
      if (x0 == x1 && y0 == y1)
        break;
      int16_t e2 = err << 1;
      if (e2 >= dy)
      {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx)
      {
        err += dx;
        y0 += sy;
      }
    }

    flgAutoUpd = prevUpd;
    if (flgAutoUpd)
      update();
  }
  else
  {
    if (y0 == y1 && y0 >= 0 && y0 < 64)
    {
      if (x0 > x1)
      {
        int16_t t = x0;
        x0 = x1;
        x1 = t;
      }
      if (x1 >= 0 && x0 < 128)
      {
        uint8_t sx = (x0 < 0) ? 0 : (uint8_t)x0;
        uint8_t ex = (x1 > 127) ? 127 : (uint8_t)x1;
        drawHLine(sx, (uint8_t)y0, ex - sx + 1);
      }
    }
    else if (x0 == x1 && x0 >= 0 && x0 < 128)
    {
      if (y0 > y1)
      {
        int16_t t = y0;
        y0 = y1;
        y1 = t;
      }
      if (y1 >= 0 && y0 < 64)
      {
        uint8_t sy = (y0 < 0) ? 0 : (uint8_t)y0;
        uint8_t ey = (y1 > 63) ? 63 : (uint8_t)y1;
        drawVLine((uint8_t)x0, sy, ey - sy + 1);
      }
    }
  }
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::drawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t dashLen, uint8_t gapLen)
{
  if (w == 0 || h == 0)
    return;
  bool prevUpd = flgAutoUpd;
  if (_BUFF)
    flgAutoUpd = false;

  drawHLine(x, y, w, dashLen, gapLen);
  if (h > 1)
    drawHLine(x, y + h - 1, w, dashLen, gapLen);
  if (h > 2)
  {
    drawVLine(x, y + 1, h - 2, dashLen, gapLen);
    if (w > 1)
      drawVLine(x + w - 1, y + 1, h - 2, dashLen, gapLen);
  }

  if (_BUFF)
  {
    flgAutoUpd = prevUpd;
    if (flgAutoUpd)
      update();
  }
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)
{
  if (x >= 128 || y >= 64 || w == 0 || h == 0)
    return;
  if (x + w > 128)
    w = 128 - x;
  if (y + h > 64)
    h = 64 - y;

  if (_BUFF)
  {
    for (uint8_t dy = 0; dy < h; dy++)
    {
      for (uint8_t dx = 0; dx < w; dx++)
      {
        _setPixelBuf(x + dx, y + dy, color);
      }
    }
    if (flgAutoUpd)
      update();
  }
  else
  {
    uint8_t pStart = y >> 3;
    uint8_t pEnd = (y + h - 1) >> 3;
    uint8_t fillByte = color ? 0xFF : 0x00;
    for (uint8_t p = pStart; p <= pEnd; p++)
    {
      _setWindow(x, x + w - 1, p, p);
      Wire.beginTransmission(dispAddr);
      Wire.write(SSD1306_DATA);
      for (uint8_t i = 0; i < w; i++)
        Wire.write(fillByte);
      Wire.endTransmission();
    }
  }
}
//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::drawCircle(int16_t x0, int16_t y0, uint8_t r, bool fill, bool color)
{
  if (_BUFF)
  {
    int16_t f = 1 - (int16_t)r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * (int16_t)r;
    int16_t x = 0;
    int16_t y = r;

    auto drawSpan = [&](int16_t sx, int16_t ex, int16_t py)
    {
      if (py < 0 || py > 63)
        return;
      if (sx > ex)
      {
        int16_t t = sx;
        sx = ex;
        ex = t;
      }
      if (ex < 0 || sx > 127)
        return;
      if (sx < 0)
        sx = 0;
      if (ex > 127)
        ex = 127;
      for (int16_t px = sx; px <= ex; px++)
        _setPixelBuf(px, py, color);
    };

    if (fill)
    {
      drawSpan(x0 - r, x0 + r, y0);
    }
    else
    {
      _setPixelBuf(x0, y0 + r, color);
      _setPixelBuf(x0, y0 - r, color);
      _setPixelBuf(x0 + r, y0, color);
      _setPixelBuf(x0 - r, y0, color);
    }

    while (x < y)
    {
      if (f >= 0)
      {
        y--;
        ddF_y += 2;
        f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x;

      if (fill)
      {
        drawSpan(x0 - x, x0 + x, y0 + y);
        drawSpan(x0 - x, x0 + x, y0 - y);
        drawSpan(x0 - y, x0 + y, y0 + x);
        drawSpan(x0 - y, x0 + y, y0 - x);
      }
      else
      {
        _setPixelBuf(x0 + x, y0 + y, color);
        _setPixelBuf(x0 - x, y0 + y, color);
        _setPixelBuf(x0 + x, y0 - y, color);
        _setPixelBuf(x0 - x, y0 - y, color);
        _setPixelBuf(x0 + y, y0 + x, color);
        _setPixelBuf(x0 - y, y0 + x, color);
        _setPixelBuf(x0 + y, y0 - x, color);
        _setPixelBuf(x0 - y, y0 - x, color);
      }
    }
    if (flgAutoUpd)
      update();
  }
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::drawBitmap(uint8_t col, uint8_t row, const uint8_t *bitmap, uint8_t w, uint8_t h, bool invert)
{
  if (!bitmap || w == 0 || h == 0)
    return;

  uint8_t pages = (h + 7) >> 3;
  uint8_t x = (col == OLED_L) ? 0 : (col == OLED_C ? (128 - w) >> 1 : (col == OLED_R ? 128 - w : col));
  if (x >= 128)
    return;

  uint8_t drawW = (x + w > 128) ? (128 - x) : w;
  uint8_t invMask = invert ? 0xFF : 0x00;

  if (_BUFF)
  {
    for (uint8_t p = 0; p < pages; p++)
    {
      uint16_t rowOffset = (uint16_t)p * w;
      for (uint8_t k = 0; k < drawW; k++)
      {
        uint8_t b = pgm_read_byte(&bitmap[rowOffset + k]) ^ invMask;
        for (uint8_t bit = 0; bit < 8; bit++)
        {
          uint8_t py = row + (p << 3) + bit;
          if (py < 64 && ((p << 3) + bit) < h)
            _setPixelBuf(x + k, py, (b & (1U << bit)) != 0);
        }
      }
    }
    if (flgAutoUpd)
      update();
  }
  else
  {
    if (row >= 8)
      return;
    for (uint8_t p = 0; p < pages; p++)
    {
      uint8_t pageY = row + p;
      if (pageY > 7)
        break;
      _setWindow(x, x + drawW - 1, pageY, pageY);
      Wire.beginTransmission(dispAddr);
      Wire.write(SSD1306_DATA);
      uint16_t rowOffset = (uint16_t)p * w;
      for (uint8_t k = 0; k < drawW; k++)
      {
        Wire.write(pgm_read_byte(&bitmap[rowOffset + k]) ^ invMask);
      }
      Wire.endTransmission();
    }
  }
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::setFont(const uint8_t *font)
{
  if (!font)
    return;

  insFont.font = font;
  insFont.width = pgm_read_byte(font);
  insFont.height = pgm_read_byte(font + 1);
  insFont.firstSymbol = pgm_read_byte(font + 2);
  insFont.sumSymbol = pgm_read_byte(font + 3);
  insFont.isFontSet = true;

  uint16_t offset = (((uint16_t)insFont.sumSymbol * insFont.width * insFont.height) >> 3) + 4;
  uint8_t intervals = pgm_read_byte(&font[offset]);
  if (intervals > 3)
    intervals = 3;

  for (uint8_t k = 0; k < 3; k++)
  {
    if (k < intervals)
    {
      insFont.startSpace[k] = pgm_read_byte(&font[offset + 1 + (k << 1)]);
      insFont.sumSpace[k] = pgm_read_byte(&font[offset + 2 + (k << 1)]);
    }
    else
    {
      insFont.startSpace[k] = 0xFF;
      insFont.sumSpace[k] = 0;
    }
  }
}

static inline void _danichCopyRam(char *dst, const char *src, uint8_t maxLen)
{
  uint8_t i = 0;
  while (i < maxLen && src[i] != '\0')
  {
    dst[i] = src[i];
    i++;
  }
  if (i == maxLen && ((uint8_t)dst[i - 1] == 0xD0 || (uint8_t)dst[i - 1] == 0xD1))
    i--;
  dst[i] = '\0';
}

static inline void _danichCopyProgmem(char *dst, const __FlashStringHelper *src, uint8_t maxLen)
{
  const char *p = (const char *)src;
  uint8_t i = 0;
  while (i < maxLen)
  {
    char c = pgm_read_byte(p++);
    if (c == '\0')
      break;
    dst[i++] = c;
  }
  if (i == maxLen && ((uint8_t)dst[i - 1] == 0xD0 || (uint8_t)dst[i - 1] == 0xD1))
    i--;
  dst[i] = '\0';
}

template <uint8_t _BUFF>
uint16_t Danich_SSD130x_Fast<_BUFF>::_getGlyphIndex(uint8_t ch) const
{
  uint16_t num = ch;
  for (uint8_t k = 0; k < 3; k++)
  {
    if (insFont.startSpace[k] < num)
      num -= insFont.sumSpace[k];
  }
  return num - insFont.firstSymbol;
}
//*********************************************************************
#ifdef RENESAS_CORTEX_M4
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(int num, uint8_t col, uint8_t str, uint8_t sys) { print((int32_t)num, col, str, sys); }
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(unsigned int num, uint8_t col, uint8_t str, uint8_t sys) { print((uint32_t)num, col, str, sys); }
#else
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(int8_t num, uint8_t col, uint8_t str, uint8_t sys) { print((int32_t)num, col, str, sys); }
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(uint8_t num, uint8_t col, uint8_t str, uint8_t sys) { print((uint32_t)num, col, str, sys); }
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(int16_t num, uint8_t col, uint8_t str, uint8_t sys) { print((int32_t)num, col, str, sys); }
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(uint16_t num, uint8_t col, uint8_t str, uint8_t sys) { print((uint32_t)num, col, str, sys); }
#endif

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(int32_t num, uint8_t col, uint8_t str, uint8_t sys)
{
  if (sys < 2 || sys > 36)
    sys = 10;
  char buf[34];
  char *ptr = &buf[33];
  *ptr = '\0';

  bool isNegative = (num < 0);
  uint32_t n = isNegative ? (0UL - (uint32_t)num) : (uint32_t)num;

  if (n == 0)
    *(--ptr) = '0';
  else
  {
    while (n > 0)
    {
      uint8_t rem = n % sys;
      *(--ptr) = (rem < 10) ? ('0' + rem) : ('A' + (rem - 10));
      n /= sys;
    }
  }
  if (isNegative)
    *(--ptr) = '-';
  _print(ptr, col, str);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(uint32_t num, uint8_t col, uint8_t str, uint8_t sys)
{
  if (sys < 2 || sys > 36)
    sys = 10;
  char buf[33];
  ultoa(num, buf, sys);
  _print(buf, col, str);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(double num, uint8_t col, uint8_t str, int sum)
{
  char buf[32];
  char *ptr = buf;
  if (sum > 9)
    sum = 9;
  if (sum < 0)
    sum = 0;

  if (num < 0.0)
  {
    *ptr++ = '-';
    num = -num;
  }

  double rounding = 0.5;
  for (int i = 0; i < sum; ++i)
    rounding /= 10.0;
  num += rounding;
  if (num > 4294967040.0)
    num = 4294967040.0;

  uint32_t intPart = (uint32_t)num;
  double fracPart = num - (double)intPart;

  char *intPtr = &buf[31];
  *intPtr = '\0';
  if (intPart == 0)
    *(--intPtr) = '0';
  else
  {
    while (intPart > 0)
    {
      *(--intPtr) = '0' + (intPart % 10);
      intPart /= 10;
    }
  }
  while (*intPtr)
    *ptr++ = *intPtr++;

  if (sum > 0)
  {
    *ptr++ = '.';
    while (sum > 0)
    {
      fracPart *= 10.0;
      uint8_t digit = (uint8_t)fracPart;
      *ptr++ = '0' + digit;
      fracPart -= digit;
      sum--;
    }
  }
  *ptr = '\0';
  _print(buf, col, str);
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(char *txt, uint8_t col, uint8_t str) { print((const char *)txt, col, str); }

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(const char *txt, uint8_t col, uint8_t str)
{
  if (!txt)
    return;
  char buf[50];
  _danichCopyRam(buf, txt, 49);
  _print(_codingCP866(buf), col, str);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(const String &str, uint8_t col, uint8_t row) { print(str.c_str(), col, row); }

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(char c, uint8_t x, uint8_t y)
{
  char buf[2] = {c, '\0'};
  _print(_codingCP866(buf), x, y);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::print(const __FlashStringHelper *txt, uint8_t x, uint8_t y)
{
  if (!txt)
    return;
  char buf[50];
  _danichCopyProgmem(buf, txt, 49);
  _print(_codingCP866(buf), x, y);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::printPadZero(uint16_t val, uint8_t width, uint8_t col, uint8_t str)
{
  if (width == 0 || width > 5)
    return;
  char buf[6];
  buf[width] = '\0';
  for (int8_t i = width - 1; i >= 0; i--)
  {
    buf[i] = '0' + (val % 10);
    val /= 10;
  }
  _print(buf, col, str);
}
//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_print(char *txt, uint8_t col, uint8_t str)
{
  if (!insFont.isFontSet || (insFont.height & 7) > 0)
    return;

  uint8_t fontWidth = insFont.width;
  uint8_t fontPages = insFont.height >> 3;
  uint8_t scale = txtScale;
  uint8_t charStepX = fontWidth * scale;
  uint8_t totalPages = fontPages * scale;
  uint8_t totalH = insFont.height * scale;

  uint16_t len = (uint16_t)strlen(txt) * charStepX;
  if (len > 128)
    len = (128 / charStepX) * charStepX;

  switch (col)
  {
  case OLED_L:
    numX = 0;
    break;
  case OLED_C:
    numX = (128 - len) >> 1;
    break;
  case OLED_R:
    numX = 128 - len;
    break;
  case OLED_N:
    break;
  default:
    numX = col;
    break;
  }

  if (_BUFF)
  {
    switch (str)
    {
    case OLED_T:
      numY = 0;
      break;
    case OLED_C:
      numY = (64 - totalH) >> 1;
      break;
    case OLED_B:
      numY = 64 - totalH;
      break;
    case OLED_N:
      break;
    default:
      numY = _resolveY(str);
      break;
    }
  }
  else
  {
    switch (str)
    {
    case OLED_T:
      numY = totalPages - 1;
      break;
    case OLED_C:
      numY = ((8 + totalPages) >> 1) - 1;
      break;
    case OLED_B:
      numY = 7;
      break;
    case OLED_N:
      break;
    default:
      numY = _resolveY(str);
      break;
    }
  }

  if (numX >= 128)
    return;
  if (numX + len > 128)
    len = ((128 - numX) / charStepX) * charStepX;
  if (len == 0)
    return;

  uint8_t maxChars = len / charStepX;
  uint8_t invertMask = insFont.inverted ? 0xFF : 0x00;

  if (_BUFF)
  {
    for (uint8_t n = 0; n < maxChars; n++)
    {
      uint16_t num = _getGlyphIndex((uint8_t)txt[n]);
      uint16_t charBase = num * fontWidth * fontPages + 4;
      uint8_t baseX = numX + n * charStepX;

      for (uint8_t p = 0; p < fontPages; p++)
      {
        uint16_t pageAddr = charBase + (uint16_t)p * fontWidth;
        for (uint8_t k = 0; k < fontWidth; k++)
        {
          uint8_t fontByte = pgm_read_byte(&insFont.font[pageAddr + k]) ^ invertMask;
          for (uint8_t b = 0; b < 8; b++)
          {
            bool pix = (fontByte & (1U << b)) != 0;
            uint8_t py0 = numY + ((p << 3) + b) * scale;
            uint8_t px0 = baseX + k * scale;
            for (uint8_t sy = 0; sy < scale; sy++)
            {
              for (uint8_t sx = 0; sx < scale; sx++)
              {
                _setPixelBuf(px0 + sx, py0 + sy, pix);
              }
            }
          }
        }
      }
    }
    if (flgAutoUpd)
      update();
  }
  else
  {
    if (scale == 1)
    {
      for (uint8_t p = 0; p < fontPages; p++)
      {
        int8_t pageY = (int8_t)numY + (int8_t)p + 1 - (int8_t)fontPages;
        if (pageY < 0 || pageY > 7)
          continue;

        _setWindow(numX, numX + len - 1, (uint8_t)pageY, (uint8_t)pageY);
        Wire.beginTransmission(dispAddr);
        Wire.write(SSD1306_DATA);

        for (uint8_t n = 0; n < maxChars; n++)
        {
          uint16_t num = _getGlyphIndex((uint8_t)txt[n]);
          uint16_t glyphAddr = num * fontWidth * fontPages + (uint16_t)p * fontWidth + 4;
          for (uint8_t k = 0; k < fontWidth; k++)
          {
            Wire.write(pgm_read_byte(&insFont.font[glyphAddr + k]) ^ invertMask);
          }
        }
        Wire.endTransmission();
      }
    }
    else
    {
      for (uint8_t tp = 0; tp < totalPages; tp++)
      {
        int8_t pageY = (int8_t)numY + (int8_t)tp + 1 - (int8_t)totalPages;
        if (pageY < 0 || pageY > 7)
          continue;

        _setWindow(numX, numX + len - 1, (uint8_t)pageY, (uint8_t)pageY);
        Wire.beginTransmission(dispAddr);
        Wire.write(SSD1306_DATA);

        uint8_t srcPage = tp / scale;
        uint8_t subSlice = tp % scale;
        uint8_t bitStart = (subSlice * 8) / scale;

        for (uint8_t n = 0; n < maxChars; n++)
        {
          uint16_t num = _getGlyphIndex((uint8_t)txt[n]);
          uint16_t glyphAddr = num * fontWidth * fontPages + (uint16_t)srcPage * fontWidth + 4;

          for (uint8_t k = 0; k < fontWidth; k++)
          {
            uint8_t fontByte = pgm_read_byte(&insFont.font[glyphAddr + k]);
            uint8_t outByte = 0;
            for (uint8_t outBit = 0; outBit < 8; outBit++)
            {
              uint8_t srcBit = bitStart + (outBit / scale);
              if (fontByte & (1U << srcBit))
                outByte |= (1U << outBit);
            }
            outByte ^= invertMask;
            for (uint8_t s = 0; s < scale; s++)
              Wire.write(outByte);
          }
        }
        Wire.endTransmission();
      }
    }
  }
  numX += len;
}
//*********************************************************************
template <uint8_t _BUFF>
uint16_t Danich_SSD130x_Fast<_BUFF>::printScroll(char *str, uint8_t row, int16_t offsetX, uint8_t winX, uint8_t winW)
{
  return printScroll((const char *)str, row, offsetX, winX, winW);
}

template <uint8_t _BUFF>
uint16_t Danich_SSD130x_Fast<_BUFF>::printScroll(const char *str, uint8_t row, int16_t offsetX, uint8_t winX, uint8_t winW)
{
  if (!str)
    return 0;
  char buf[80];
  _danichCopyRam(buf, str, 79);
  return _printScroll(_codingCP866(buf), row, offsetX, winX, winW);
}

template <uint8_t _BUFF>
uint16_t Danich_SSD130x_Fast<_BUFF>::printScroll(const __FlashStringHelper *txt, uint8_t row, int16_t offsetX, uint8_t winX, uint8_t winW)
{
  if (!txt)
    return 0;
  char buf[80];
  _danichCopyProgmem(buf, txt, 79);
  return _printScroll(_codingCP866(buf), row, offsetX, winX, winW);
}

template <uint8_t _BUFF>
uint16_t Danich_SSD130x_Fast<_BUFF>::_printScroll(char *txt, uint8_t row, int16_t offsetX, uint8_t winX, uint8_t winW)
{
  if (!insFont.isFontSet || (insFont.height & 7) > 0)
    return 0;
  if (winX >= 128 || winW == 0)
    return 0;
  if (winX + winW > 128)
    winW = 128 - winX;

  uint8_t fontWidth = insFont.width;
  uint8_t fontPages = insFont.height >> 3;
  uint8_t scale = txtScale;
  uint8_t charStepX = fontWidth * scale;
  uint8_t totalPages = fontPages * scale;
  uint16_t numChars = (uint16_t)strlen(txt);
  uint16_t totalW = numChars * charStepX;

  uint8_t winEnd = winX + winW;
  uint8_t invertMask = insFont.inverted ? 0xFF : 0x00;

  int16_t startRelX = (int16_t)winX - offsetX;
  uint8_t initChar = 0;
  uint8_t initCol = 0;
  uint8_t initSub = 0;

  if (startRelX > 0 && startRelX < (int16_t)totalW)
  {
    initChar = (uint8_t)((uint16_t)startRelX / charStepX);
    uint8_t rem = (uint8_t)((uint16_t)startRelX % charStepX);
    initCol = rem / scale;
    initSub = rem % scale;
  }

  for (uint8_t tp = 0; tp < totalPages; tp++)
  {
    int8_t pageY = (int8_t)row + (int8_t)tp + 1 - (int8_t)totalPages;
    if (pageY < 0 || pageY > 7)
      continue;

    if (!_BUFF)
    {
      _setWindow(winX, winEnd - 1, (uint8_t)pageY, (uint8_t)pageY);
      Wire.beginTransmission(dispAddr);
      Wire.write(SSD1306_DATA);
    }

    uint8_t srcPage = tp / scale;
    uint8_t subSlice = tp % scale;
    uint8_t bitStart = (subSlice * 8) / scale;

    int16_t relX = startRelX;
    uint8_t charIdx = initChar;
    uint8_t colIdx = initCol;
    uint8_t subIdx = initSub;

    uint16_t glyphBase = 0;
    uint8_t cachedOut = invertMask;
    bool needGlyph = true;
    bool needCol = true;
    uint16_t bufRowBase = (uint16_t)pageY << 7;

    for (uint8_t sx = winX; sx < winEnd; sx++, relX++)
    {
      if (relX < 0 || relX >= (int16_t)totalW)
      {
        if (_BUFF)
          arrBuffer[bufRowBase + sx] = invertMask;
        else
          Wire.write(invertMask);
        continue;
      }

      if (needGlyph)
      {
        uint16_t num = _getGlyphIndex((uint8_t)txt[charIdx]);
        glyphBase = num * fontWidth * fontPages + (uint16_t)srcPage * fontWidth + 4;
        needGlyph = false;
        needCol = true;
      }

      if (needCol)
      {
        uint8_t fontByte = pgm_read_byte(&insFont.font[glyphBase + colIdx]);
        if (scale == 1)
        {
          cachedOut = fontByte ^ invertMask;
        }
        else
        {
          uint8_t outByte = 0;
          for (uint8_t outBit = 0; outBit < 8; outBit++)
          {
            uint8_t srcBit = bitStart + (outBit / scale);
            if (fontByte & (1U << srcBit))
              outByte |= (1U << outBit);
          }
          cachedOut = outByte ^ invertMask;
        }
        needCol = false;
      }

      if (_BUFF)
        arrBuffer[bufRowBase + sx] = cachedOut;
      else
        Wire.write(cachedOut);

      if (++subIdx >= scale)
      {
        subIdx = 0;
        needCol = true;
        if (++colIdx >= fontWidth)
        {
          colIdx = 0;
          charIdx++;
          needGlyph = true;
        }
      }
    }

    if (_BUFF)
      _markDirty(winX, winEnd - 1, (uint8_t)pageY, (uint8_t)pageY);
    else
      Wire.endTransmission();
  }

  if (_BUFF && flgAutoUpd)
    update();

  return totalW;
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::printTicker(char *str, uint8_t row, uint16_t speedMs, uint8_t winX, uint8_t winW)
{
  printTicker((const char *)str, row, speedMs, winX, winW);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::printTicker(const char *str, uint8_t row, uint16_t speedMs, uint8_t winX, uint8_t winW)
{
  if (!str)
    return;
  uint32_t now = millis();
  if (tickerX != 32767 && (now - tTicker < speedMs))
    return;
  tTicker = now;

  char buf[80];
  _danichCopyRam(buf, str, 79);
  _printTicker(_codingCP866(buf), row, winX, winW);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::printTicker(const __FlashStringHelper *txt, uint8_t row, uint16_t speedMs, uint8_t winX, uint8_t winW)
{
  if (!txt)
    return;
  uint32_t now = millis();
  if (tickerX != 32767 && (now - tTicker < speedMs))
    return;
  tTicker = now;

  char buf[80];
  _danichCopyProgmem(buf, txt, 79);
  _printTicker(_codingCP866(buf), row, winX, winW);
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_printTicker(char *txt, uint8_t row, uint8_t winX, uint8_t winW)
{
  if (winX + winW > 128)
    winW = (winX < 128) ? (128 - winX) : 0;

  int16_t rightEdge = (int16_t)(winX + winW - 1);
  if (tickerX == 32767)
    tickerX = rightEdge;

  uint16_t totalW = _printScroll(txt, row, tickerX, winX, winW);
  if (--tickerX <= (int16_t)winX - (int16_t)totalW)
  {
    tickerX = rightEdge;
  }
}

//*********************************************************************
template <uint8_t _BUFF>
char *Danich_SSD130x_Fast<_BUFF>::_codingCP866(char *StrIn)
{
  uint8_t *in = (uint8_t *)StrIn;
  uint8_t *out = (uint8_t *)StrIn;

  switch (codingName)
  {
  case TXT_UTF8:
    while (*in != '\0')
    {
      uint8_t c1 = *in;
      uint8_t c2 = *(in + 1);
      if (c1 == 0xD0 && c2 >= 0x90 && c2 <= 0xBF)
      {
        *out++ = c2 - 0x10;
        in += 2;
      }
      else if (c1 == 0xD0 && c2 == 0x81)
      {
        *out++ = 0xF0;
        in += 2;
      }
      else if (c1 == 0xD1 && c2 >= 0x80 && c2 <= 0x8F)
      {
        *out++ = c2 + 0x60;
        in += 2;
      }
      else if (c1 == 0xD1 && c2 == 0x91)
      {
        *out++ = 0xF1;
        in += 2;
      }
      else
      {
        *out++ = *in++;
      }
    }
    *out = '\0';
    break;

  case TXT_WIN1251:
    while (*in != '\0')
    {
      if (*in >= 0xC0 && *in <= 0xEF)
        *out++ = *in++ - 0x40;
      else if (*in >= 0xF0 && *in <= 0xFF)
        *out++ = *in++ - 0x10;
      else if (*in == 0xA8)
      {
        *out++ = 0xF0;
        in++;
      }
      else if (*in == 0xB8)
      {
        *out++ = 0xF1;
        in++;
      }
      else
        *out++ = *in++;
    }
    *out = '\0';
    break;
  }

  return StrIn;
}

//*********************************************************************
template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_setWindow(uint8_t col0, uint8_t col1, uint8_t page0, uint8_t page1)
{
  Wire.beginTransmission(dispAddr);
  Wire.write(SSD1306_COMMAND);
  Wire.write(SSD1306_ADDR_COLUMN);
  Wire.write(col0);
  Wire.write(col1);
  Wire.write(SSD1306_ADDR_PAGE);
  Wire.write(page0);
  Wire.write(page1);
  Wire.endTransmission();
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_sendCommand(uint8_t command)
{
  Wire.beginTransmission(dispAddr);
  Wire.write(SSD1306_COMMAND);
  Wire.write(command);
  Wire.endTransmission();
}

template <uint8_t _BUFF>
void Danich_SSD130x_Fast<_BUFF>::_sendCommand2(uint8_t command, uint8_t arg)
{
  Wire.beginTransmission(dispAddr);
  Wire.write(SSD1306_COMMAND);
  Wire.write(command);
  Wire.write(arg);
  Wire.endTransmission();
}
