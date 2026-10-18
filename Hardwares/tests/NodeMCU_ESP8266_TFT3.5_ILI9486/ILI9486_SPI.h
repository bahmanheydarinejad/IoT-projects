#ifndef _ILI9486_SPI_H_
#define _ILI9486_SPI_H_

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

// ابعاد پیش‌فرض نمایشگر ۳.۵ اینچ در حالت عمودی
#define ILI9486_TFTWIDTH   320
#define ILI9486_TFTHEIGHT  480

// دستورات اصلی درایور ILI9486
#define ILI9486_NOP        0x00
#define ILI9486_SWRESET    0x01
#define ILI9486_SLPIN      0x10
#define ILI9486_SLPOUT     0x11
#define ILI9486_NORON      0x13
#define ILI9486_INVOFF     0x20
#define ILI9486_INVON      0x21
#define ILI9486_DISPOFF    0x28
#define ILI9486_DISPON     0x29
#define ILI9486_CASET      0x2A
#define ILI9486_PASET      0x2B
#define ILI9486_RAMWR      0x2C
#define ILI9486_RAMRD      0x2E
#define ILI9486_MADCTL     0x36
#define ILI9486_PIXFMT     0x3A

// رنگ‌های پرکاربرد ۱۶ بیتی RGB565
#define COLOR_BLACK       0x0000
#define COLOR_NAVY        0x000F
#define COLOR_DARKGREEN   0x03E0
#define COLOR_DARKCYAN    0x03EF
#define COLOR_MAROON      0x7800
#define COLOR_PURPLE      0x780F
#define COLOR_OLIVE       0x7BE0
#define COLOR_LIGHTGREY   0xC618
#define COLOR_DARKGREY    0x7BEF
#define COLOR_BLUE        0x001F
#define COLOR_GREEN       0x07E0
#define COLOR_CYAN        0x07FF
#define COLOR_RED         0xF800
#define COLOR_MAGENTA     0xF81F
#define COLOR_YELLOW      0xFFE0
#define COLOR_WHITE       0xFFFF
#define COLOR_ORANGE      0xFD20
#define COLOR_GREENYELLOW 0xAFE5
#define COLOR_PINK        0xF81F

class ILI9486_SPI : public Adafruit_GFX {
public:
  ILI9486_SPI(int8_t cs, int8_t dc, int8_t rst = -1)
    : Adafruit_GFX(ILI9486_TFTWIDTH, ILI9486_TFTHEIGHT),
      _cs(cs), _dc(dc), _rst(rst) {}

  // تبدیل مؤلفه‌های 8 بیتی قرمز، سبز و آبی به فرمت رنگ 16 بیتی RGB565
  static inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }

  void begin(uint32_t freq = 40000000) {
    _spiFreq = freq;
    pinMode(_dc, OUTPUT);
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
    digitalWrite(_dc, HIGH);

    if (_rst >= 0) {
      pinMode(_rst, OUTPUT);
      digitalWrite(_rst, HIGH);
      delay(10);
      digitalWrite(_rst, LOW);
      delay(20);
      digitalWrite(_rst, HIGH);
      delay(120);
    }

    SPI.begin();

    // دنباله رجیسترهای راه‌اندازی ILI9486L
    writeCommand(ILI9486_SWRESET);
    delay(120);

    writeCommand(ILI9486_SLPOUT);
    delay(120);

    // تنظیم فرمت رنگ پیکسل روی حالت ۱۸ بیت در گذرگاه SPI
    // چیپ ILI9486 در ارتباط SPI داده‌های رنگی را به فرمت ۳ بایت (RGB666) دریافت می‌کند
    writeCommand(ILI9486_PIXFMT);
    writeData(0x66);

    // تنظیمات توان و ولتاژ درایور
    writeCommand(0xC2); // Power Control 3
    writeData(0x44);

    writeCommand(0xC5); // VCOM Control
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);

    // تنظیمات گاما مثبت و منفی (PGAMCTRL / NGAMCTRL)
    writeCommand(0xE0);
    writeData(0x0F); writeData(0x1F); writeData(0x1C); writeData(0x0C);
    writeData(0x0F); writeData(0x08); writeData(0x48); writeData(0x98);
    writeData(0x37); writeData(0x0A); writeData(0x13); writeData(0x04);
    writeData(0x11); writeData(0x0D); writeData(0x00);

    writeCommand(0xE1);
    writeData(0x0F); writeData(0x32); writeData(0x2E); writeData(0x0B);
    writeData(0x0D); writeData(0x05); writeData(0x47); writeData(0x75);
    writeData(0x37); writeData(0x06); writeData(0x10); writeData(0x03);
    writeData(0x24); writeData(0x20); writeData(0x00);

    // چرخش اولیه (Portrait)
    setRotation(0);

    writeCommand(ILI9486_INVOFF);
    writeCommand(ILI9486_NORON);
    delay(10);

    writeCommand(ILI9486_DISPON);
    delay(100);
  }

  void setRotation(uint8_t m) override {
    rotation = m % 4;
    writeCommand(ILI9486_MADCTL);
    switch (rotation) {
      case 0: // Portrait
        writeData(0x48);
        _width  = ILI9486_TFTWIDTH;
        _height = ILI9486_TFTHEIGHT;
        break;
      case 1: // Landscape
        writeData(0x28);
        _width  = ILI9486_TFTHEIGHT;
        _height = ILI9486_TFTWIDTH;
        break;
      case 2: // Inverted Portrait
        writeData(0x88);
        _width  = ILI9486_TFTWIDTH;
        _height = ILI9486_TFTHEIGHT;
        break;
      case 3: // Inverted Landscape
        writeData(0xE8);
        _width  = ILI9486_TFTHEIGHT;
        _height = ILI9486_TFTWIDTH;
        break;
    }
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

    setAddrWindow(x, y, x, y);

    SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);

    pushColor(color);

    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
  }

  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
    if ((x < 0) || (x >= _width) || (h <= 0)) return;
    if (y < 0) { h += y; y = 0; }
    if ((y + h - 1) >= _height) h = _height - y;
    if (h <= 0) return;

    fillRect(x, y, 1, h, color);
  }

  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
    if ((y < 0) || (y >= _height) || (w <= 0)) return;
    if (x < 0) { w += x; x = 0; }
    if ((x + w - 1) >= _width) w = _width - x;
    if (w <= 0) return;

    fillRect(x, y, w, 1, color);
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    if ((x >= _width) || (y >= _height)) return;
    if ((x + w - 1) >= _width)  w = _width  - x;
    if ((y + h - 1) >= _height) h = _height - y;
    if ((w <= 0) || (h <= 0)) return;

    setAddrWindow(x, y, x + w - 1, y + h - 1);

    uint8_t r = (color >> 8) & 0xF8;
    uint8_t g = (color >> 3) & 0xFC;
    uint8_t b = (color << 3) & 0xF8;

    SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);

    uint32_t totalPixels = (uint32_t)w * h;
    for (uint32_t i = 0; i < totalPixels; i++) {
      SPI.write(r);
      SPI.write(g);
      SPI.write(b);
    }

    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
  }

  void fillScreen(uint16_t color) override {
    fillRect(0, 0, _width, _height, color);
  }

  void invertDisplay(bool i) override {
    writeCommand(i ? ILI9486_INVON : ILI9486_INVOFF);
  }

private:
  int8_t _cs, _dc, _rst;
  uint32_t _spiFreq = 40000000;

  void writeCommand(uint8_t cmd) {
    SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
    digitalWrite(_dc, LOW);
    digitalWrite(_cs, LOW);
    SPI.write(cmd);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
  }

  void writeData(uint8_t data) {
    SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    SPI.write(data);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
  }

  void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    writeCommand(ILI9486_CASET);
    writeData(x0 >> 8);
    writeData(x0 & 0xFF);
    writeData(x1 >> 8);
    writeData(x1 & 0xFF);

    writeCommand(ILI9486_PASET);
    writeData(y0 >> 8);
    writeData(y0 & 0xFF);
    writeData(y1 >> 8);
    writeData(y1 & 0xFF);

    writeCommand(ILI9486_RAMWR);
  }

  inline void pushColor(uint16_t color) {
    // تبدیل ۱۶ بیت RGB565 به فرمت ۱۸ بیت RGB666 برای گذرگاه SPI
    uint8_t r = (color >> 8) & 0xF8;
    uint8_t g = (color >> 3) & 0xFC;
    uint8_t b = (color << 3) & 0xF8;
    SPI.write(r);
    SPI.write(g);
    SPI.write(b);
  }
};

#endif // _ILI9486_SPI_H_
