/**********************************************************************************
 * پروژه تست و راه‌اندازی ماژول نمایشگر 2.8 اینچ TFT لمسی SPI به همراه NodeMCU ESP8266
 * 
 * سخت‌افزارهای مورد استفاده (از انبار قطعات):
 * 1. ماژول نمایشگر 2.8 اینچ TFT لمسی تمام رنگ SPI ورژن 1.2 (کد کالا: LCD-01-280)
 *    - درایور تصویر: ILI9341 (رزولوشن 320x240 پیکسل، 65K رنگ RGB565)
 *    - کنترلر لمسی: XPT2046 (ارتباط SPI مجزا)
 * 2. برد توسعه NodeMcu دارای هسته وای‌فای ESP8266 و مبدل CH340 (کد کالا: COM-03-009)
 * 
 * کتابخانه‌های مورد نیاز (نصب از طریق Arduino Library Manager):
 * - Adafruit GFX Library (توسط Adafruit)
 * - Adafruit ILI9341 (توسط Adafruit)
 * - XPT2046_Touchscreen (توسط Paul Stoffregen)
 * 
 * اتصالات پین‌ها (Wiring Pinout - چیدمان منظم و پشت سر هم):
 * ---------------------------------------------------------------------------------
 * پایه نمایشگر (TFT & Touch)  | پایه NodeMCU (ESP8266) | عملکرد پین
 * ---------------------------------------------------------------------------------
 * 1. VCC                     | 3V3                     | تغذیه 3.3 ولت (کنار D4)
 * 2. GND                     | GND                     | زمین مدار (کنار 3V3)
 * 3. CS                      | D1 (GPIO5)              | انتخاب تراشه نمایشگر (متوالی با پین 3)
 * 4. RESET                   | D2 (GPIO4)              | ریست نمایشگر (متوالی با پین 4)
 * 5. DC (or RS)              | D3 (GPIO0)              | دستور / داده نمایشگر (متوالی با پین 5)
 * 6. SDI (MOSI)              | D7 (GPIO13)             | گذرگاه سخت‌افزاری SPI MOSI
 * 7. SCK (CLK)               | D5 (GPIO14)             | کلاک سخت‌افزاری SPI SCK
 * 8. LED (BLK)               | 3V3                     | نور پس‌زمینه (روشن دائم)
 * 9. SDO (MISO)              | آزاد (اختیاری)          | خروجی دیتای تصویر
 * 10. T_CLK                  | D5 (GPIO14)             | کلاک تاچ (مشترک با SCK نمایشگر)
 * 11. T_CS                   | D8 (GPIO15)             | انتخاب تراشه تاچ (کنار D7)
 * 12. T_DIN                  | D7 (GPIO13)             | دیتای ورودی تاچ (مشترک با MOSI)
 * 13. T_DO                   | D6 (GPIO12)             | دیتای خروجی تاچ (متصل به MISO)
 * 14. T_IRQ                  | D0 (GPIO16) یا نامتصل   | وقفه تاچ (اختیاری)
 * ---------------------------------------------------------------------------------
 * 
 * ویژگی‌های جدید این نسخه:
 * ۱. اصلاح تطبیق زاویه و جهت محورهای لمسی و رفع جابجایی افقی/عمودی (Landscape 320x240)
 * ۲. ابزار کالیبراسیون تعاملی ۲ نقطه‌ای روی صفحه نمایش (دکمه CALIB یا ارسال حرف c در سریال)
 * ۳. ذخیره دائمی ضرایب کالیبراسیون در حافظه فلش EEPROM برد ESP8266
 **********************************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <EEPROM.h>

// =================================================================================
// 1. تعریف پایه‌های اتصال متوالی و منظم (Sequential Orderly Pinout)
// =================================================================================
#define TFT_CS    D1    // GPIO5  -> متناظر مستقیم با پین 3 نمایشگر (CS)
#define TFT_RST   D2    // GPIO4  -> متناظر مستقیم با پین 4 نمایشگر (RESET)
#define TFT_DC    D3    // GPIO0  -> متناظر مستقیم با پین 5 نمایشگر (DC)

#define TOUCH_CS  D8    // GPIO15 -> متناظر با پین 11 نمایشگر (T_CS)
#define TOUCH_IRQ D0    // GPIO16 -> متناظر با پین 14 نمایشگر (T_IRQ - اختیاری)

// ساخت اشیاء نمایشگر و تاچ
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

// =================================================================================
// 2. تنظیمات کالیبراسیون تاچ و تطبیق محورها (Touch Calibration & Mapping)
// =================================================================================
#define EEPROM_MAGIC 0x4341  // کد شناسه اعتبارسنجی ذخیره‌سازی در EEPROM ('CA')

// مختصات دو نقطه استاندارد کالیبراسیون روی صفحه نمایش
const int CALIB_P1_X = 30;   // هدف نقطه ۱ (بالا-چپ)
const int CALIB_P1_Y = 30;
const int CALIB_P2_X = 290;  // هدف نقطه ۲ (پایین-راست)
const int CALIB_P2_Y = 210;

// مقادیر کالیبراسیون پیش‌فرض بهینه شده برای جهت لنداسکیپ (Landscape Rotation 1)
// در صورت تمایل می‌توانید با زدن دکمه CALIB یا تایپ c در سریال کالیبره مجدد انجام دهید
int cal_p1_rawX = 700;
int cal_p1_rawY = 3400;
int cal_p2_rawX = 3400;
int cal_p2_rawY = 600;
bool cal_swapXY = true;

// ساختار ذخیره اطلاعات در EEPROM
struct TouchCalibrationData {
  uint16_t magic;
  int16_t  p1_x;
  int16_t  p1_y;
  int16_t  p2_x;
  int16_t  p2_y;
  bool     swap;
};

// =================================================================================
// 3. پالت رنگ‌ها و تعریف اجزای رابط کاربری (UI)
// =================================================================================
#define COLOR_BG          0x0821  // خاکستری بسیار تیره متمایل به سورمه‌ای
#define COLOR_HEADER      0x18E3  // نوار سربرگ تیره
#define COLOR_PANEL       0x2124  // پنل ابزارها
#define COLOR_CANVAS      0x0000  // بوم نقاشی مشکی مطلق
#define COLOR_BORDER      0x4A69  // رنگ کادرهای بیرونی
#define COLOR_TEXT        0xFFFF  // سفید
#define COLOR_ACCENT      0x07E0  // سبز نئونی
#define COLOR_TOUCH_DOT   0xF800  // قرمز نشانگر لمس

// رنگ‌های قابل انتخاب برای براش قلم
const uint16_t PALETTE_COLORS[] = {
  ILI9341_WHITE,
  ILI9341_RED,
  ILI9341_GREEN,
  ILI9341_BLUE,
  ILI9341_YELLOW,
  ILI9341_CYAN,
  ILI9341_MAGENTA,
  ILI9341_ORANGE
};
const int NUM_COLORS = 8;
uint16_t currentColor = ILI9341_YELLOW;
int brushSize = 3;

// ابعاد ناحیه بوم نقاشی (Canvas Area)
const int CANVAS_X = 65;
const int CANVAS_Y = 38;
const int CANVAS_W = 250;
const int CANVAS_H = 196;

// متغیرهای ذخیره وضعیت آخرین لمس
int lastTouchX = -1;
int lastTouchY = -1;
unsigned long lastTouchTime = 0;
bool isTouching = false;

// پیش‌اعلان توابع
void initUserInterface();
void drawCanvasArea();
void drawToolPanel();
void drawHeader(const char* title, uint16_t titleColor = ILI9341_WHITE);
void updateTouchStatusIndicator(bool active, int sx, int sy);
void runGraphicsDemo();
void runTouchCalibration();
void drawCrosshair(int x, int y, uint16_t color);

// =================================================================================
// توابع تبدیل و نگاشت دقیق مختصات لمسی (Coordinate Mapping)
// =================================================================================
bool getTouchCoordinates(int &screenX, int &screenY, int &rawX, int &rawY, int &rawZ) {
  if (!ts.touched()) {
    return false;
  }

  TS_Point p = ts.getPoint();
  rawX = p.x;
  rawY = p.y;
  rawZ = p.z;

  // فیلتر کردن نویزهای با فشار بسیار کم
  if (rawZ < 300) {
    return false;
  }

  int raw_h = 0;
  int raw_v = 0;
  int h1 = 0, h2 = 0;
  int v1 = 0, v2 = 0;

  // تطبیق محورها بر اساس کالیبراسیون
  if (cal_swapXY) {
    raw_h = rawY;
    raw_v = rawX;
    h1 = cal_p1_rawY;
    h2 = cal_p2_rawY;
    v1 = cal_p1_rawX;
    v2 = cal_p2_rawX;
  } else {
    raw_h = rawX;
    raw_v = rawY;
    h1 = cal_p1_rawX;
    h2 = cal_p2_rawX;
    v1 = cal_p1_rawY;
    v2 = cal_p2_rawY;
  }

  // جلوگیری از خطای تقسیم بر صفر
  if (h2 == h1 || v2 == v1) {
    return false;
  }

  // درونیابی خطی دو نقطه‌ای مستقل از جهت و معکوس بودن
  long mappedX = (long)CALIB_P1_X + (long)(raw_h - h1) * (CALIB_P2_X - CALIB_P1_X) / (h2 - h1);
  long mappedY = (long)CALIB_P1_Y + (long)(raw_v - v1) * (CALIB_P2_Y - CALIB_P1_Y) / (v2 - v1);

  screenX = constrain((int)mappedX, 0, tft.width() - 1);
  screenY = constrain((int)mappedY, 0, tft.height() - 1);

  return true;
}

// =================================================================================
// ترسیم بوم نقاشی (Canvas Drawing Area)
// =================================================================================
void drawCanvasArea() {
  tft.fillRect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, COLOR_CANVAS);
  tft.drawRect(CANVAS_X - 1, CANVAS_Y - 1, CANVAS_W + 2, CANVAS_H + 2, COLOR_BORDER);
  
  tft.setTextSize(1);
  tft.setTextColor(0x52AA);
  tft.setCursor(CANVAS_X + 25, CANVAS_Y + CANVAS_H / 2 - 5);
  tft.println("TOUCH / DRAW ON THIS CANVAS");
}

// =================================================================================
// ترسیم سربرگ و نوار بالایی (Header Bar)
// =================================================================================
void drawHeader(const char* title, uint16_t titleColor) {
  tft.fillRect(0, 0, 320, 32, COLOR_HEADER);
  tft.drawFastHLine(0, 32, 320, COLOR_BORDER);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(6, 6);
  tft.print("NodeMCU ESP8266 + 2.8\" TFT SPI");

  tft.setTextColor(titleColor);
  tft.setCursor(6, 18);
  tft.print(title);

  updateTouchStatusIndicator(false, 0, 0);
}

// =================================================================================
// به‌روزرسانی زنده وضعیت و مختصات لمس در سربرگ
// =================================================================================
void updateTouchStatusIndicator(bool active, int sx, int sy) {
  tft.fillRect(210, 4, 105, 24, COLOR_HEADER);
  tft.drawRect(210, 4, 105, 24, active ? COLOR_ACCENT : COLOR_BORDER);

  tft.setTextSize(1);
  if (active) {
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(215, 7);
    tft.print("TOUCH: ON");
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(215, 17);
    tft.printf("X:%03d Y:%03d", sx, sy);
  } else {
    tft.setTextColor(0x8410);
    tft.setCursor(215, 7);
    tft.print("TOUCH: IDLE");
    tft.setCursor(215, 17);
    tft.print("Tap Screen...");
  }
}

// =================================================================================
// ترسیم نوار ابزار سمت چپ (Left Sidebar Tool Panel)
// =================================================================================
void drawToolPanel() {
  tft.fillRect(0, 33, 62, 207, COLOR_PANEL);
  tft.drawFastVLine(62, 33, 207, COLOR_BORDER);

  // عنوان بخش پالت
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(12, 36);
  tft.print("COLORS");

  // رسم پالت رنگ‌ها
  int startY = 47;
  int btnH = 12;
  int btnW = 46;

  for (int i = 0; i < NUM_COLORS; i++) {
    int by = startY + (i * (btnH + 2));
    tft.fillRect(8, by, btnW, btnH, PALETTE_COLORS[i]);
    
    if (PALETTE_COLORS[i] == currentColor) {
      tft.drawRect(6, by - 2, btnW + 4, btnH + 4, ILI9341_WHITE);
      tft.drawRect(7, by - 1, btnW + 2, btnH + 2, ILI9341_BLACK);
    } else {
      tft.drawRect(8, by, btnW, btnH, ILI9341_BLACK);
    }
  }

  // 1. دکمه CLEAR
  tft.fillRect(6, 164, 50, 18, 0xC800); // قرمز تیره
  tft.drawRect(6, 164, 50, 18, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(14, 169);
  tft.print("CLEAR");

  // 2. دکمه TEST
  tft.fillRect(6, 187, 50, 18, 0x03E0); // سبز تیره
  tft.drawRect(6, 187, 50, 18, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(17, 192);
  tft.print("TEST");

  // 3. دکمه CALIB (کالیبراسیون لمس)
  tft.fillRect(6, 210, 50, 18, 0x001F); // آبی تیره
  tft.drawRect(6, 210, 50, 18, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(14, 215);
  tft.print("CALIB");
}

// =================================================================================
// آزمون نمایشی بنچ‌مارک گرافیک (Graphics Benchmark Test)
// =================================================================================
void runGraphicsDemo() {
  Serial.println(F("[DEMO] Starting Graphics Demo Suite..."));
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 30);
  tft.println("GRAPHICS BENCHMARK");
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(40, 55);
  tft.println("Display Driver: ILI9341 (320x240 SPI)");
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(40, 70);
  tft.println("Controller: NodeMCU ESP8266 @ 80/160MHz");

  const uint16_t bars[] = {
    ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE, 
    ILI9341_YELLOW, ILI9341_CYAN, ILI9341_MAGENTA, ILI9341_WHITE
  };
  int barW = 300 / 7;
  for (int i = 0; i < 7; i++) {
    tft.fillRect(10 + (i * barW), 90, barW, 25, bars[i]);
  }
  tft.drawRect(9, 89, (barW * 7) + 2, 27, ILI9341_WHITE);

  int cx = 70;
  int cy = 175;
  for (int r = 10; r <= 45; r += 7) {
    tft.drawCircle(cx, cy, r, ILI9341_YELLOW);
  }
  tft.fillCircle(cx, cy, 6, ILI9341_RED);

  int rx = 160;
  int ry = 175;
  for (int s = 10; s <= 45; s += 7) {
    tft.drawRect(rx - s, ry - s, s * 2, s * 2, ILI9341_CYAN);
  }

  int tx = 265;
  int ty = 175;
  for (int i = 0; i < 360; i += 45) {
    float rad = i * 0.0174533;
    int x2 = tx + (int)(cos(rad) * 40.0);
    int y2 = ty + (int)(sin(rad) * 40.0);
    tft.drawLine(tx, ty, x2, y2, ILI9341_GREEN);
  }

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(55, 228);
  tft.println("TOUCH SCREEN TO RETURN TO CANVAS");

  unsigned long startWait = millis();
  while (millis() - startWait < 8000) {
    yield();
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      if (p.z > 300) {
        break;
      }
    }
    delay(50);
  }

  initUserInterface();
}

// =================================================================================
// ترسیم نشانگر هدف کالیبراسیون (Crosshair)
// =================================================================================
void drawCrosshair(int x, int y, uint16_t color) {
  tft.drawFastHLine(x - 12, y, 25, color);
  tft.drawFastVLine(x, y - 12, 25, color);
  tft.drawCircle(x, y, 8, color);
  tft.drawCircle(x, y, 4, color);
}

// =================================================================================
// نمونه‌برداری با دقت بالا از نقطه لمس شده حین کالیبراسیون
// =================================================================================
void getRawCalibrationPoint(int targetX, int targetY, int &avgRawX, int &avgRawY) {
  drawCrosshair(targetX, targetY, ILI9341_RED);

  while (true) {
    yield();
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      if (p.z > 350) {
        long sumX = 0;
        long sumY = 0;
        int count = 0;

        for (int i = 0; i < 20; i++) {
          yield();
          delay(12);
          if (ts.touched()) {
            TS_Point pt = ts.getPoint();
            if (pt.z > 250) {
              sumX += pt.x;
              sumY += pt.y;
              count++;
            }
          }
        }

        if (count >= 8) {
          avgRawX = sumX / count;
          avgRawY = sumY / count;
          drawCrosshair(targetX, targetY, ILI9341_GREEN);
          delay(300);
          break;
        }
      }
    }
    delay(20);
  }

  // انتظار برای برداشتن قلم از روی صفحه
  while (ts.touched()) {
    yield();
    delay(20);
  }
  delay(250);
}

// =================================================================================
// اجرای ابزار تعاملی کالیبراسیون ۲ نقطه‌ای (Interactive Calibration Wizard)
// =================================================================================
void runTouchCalibration() {
  Serial.println();
  Serial.println(F("========================================================="));
  Serial.println(F(">>> STARTING INTERACTIVE 2-POINT TOUCH CALIBRATION <<<   "));
  Serial.println(F("========================================================="));

  tft.fillScreen(ILI9341_BLACK);

  // نوار عنوان
  tft.fillRect(0, 0, 320, 26, 0x0010);
  tft.drawFastHLine(0, 26, 320, ILI9341_CYAN);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(65, 9);
  tft.println("TOUCH SCREEN CALIBRATION WIZARD");

  // ۱. نقطه اول: بالا - چپ
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(35, 95);
  tft.println("1/2: Touch the red target [+] at TOP-LEFT");
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(35, 115);
  tft.println("Use stylus or fingernail firmly...");

  int raw1X = 0, raw1Y = 0;
  getRawCalibrationPoint(CALIB_P1_X, CALIB_P1_Y, raw1X, raw1Y);
  Serial.printf("[CALIB] Target 1 (%d, %d) captured -> Raw: X=%d, Y=%d\n",
                CALIB_P1_X, CALIB_P1_Y, raw1X, raw1Y);

  delay(300);

  // پاک کردن نقطه اول و متن
  tft.fillRect(CALIB_P1_X - 14, CALIB_P1_Y - 14, 29, 29, ILI9341_BLACK);
  tft.fillRect(20, 80, 280, 70, ILI9341_BLACK);

  // ۲. نقطه دوم: پایین - راست
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(30, 95);
  tft.println("2/2: Touch red target [+] at BOTTOM-RIGHT");
  tft.setTextColor(COLOR_ACCENT);
  tft.setCursor(30, 115);
  tft.println("Use stylus or fingernail firmly...");

  int raw2X = 0, raw2Y = 0;
  getRawCalibrationPoint(CALIB_P2_X, CALIB_P2_Y, raw2X, raw2Y);
  Serial.printf("[CALIB] Target 2 (%d, %d) captured -> Raw: X=%d, Y=%d\n",
                CALIB_P2_X, CALIB_P2_Y, raw2X, raw2Y);

  // به‌روزرسانی مقادیر فعال
  cal_p1_rawX = raw1X;
  cal_p1_rawY = raw1Y;
  cal_p2_rawX = raw2X;
  cal_p2_rawY = raw2Y;

  // تشخیص خودکار جابجایی محورها
  int deltaRawX = abs(raw2X - raw1X);
  int deltaRawY = abs(raw2Y - raw1Y);
  cal_swapXY = (deltaRawY > deltaRawX);

  // ذخیره در حافظه فلش EEPROM
  TouchCalibrationData data;
  data.magic = EEPROM_MAGIC;
  data.p1_x  = (int16_t)cal_p1_rawX;
  data.p1_y  = (int16_t)cal_p1_rawY;
  data.p2_x  = (int16_t)cal_p2_rawX;
  data.p2_y  = (int16_t)cal_p2_rawY;
  data.swap  = cal_swapXY;
  EEPROM.put(0, data);
  EEPROM.commit();

  // چاپ گزارش جامع در مانیتور سریال
  Serial.println(F("---------------------------------------------------------"));
  Serial.println(F("CALIBRATION CALCULATIONS & RESULTS:"));
  Serial.printf("  Point 1 (Top-Left):     Screen(%d, %d)  -> Raw(X=%d, Y=%d)\n", CALIB_P1_X, CALIB_P1_Y, raw1X, raw1Y);
  Serial.printf("  Point 2 (Bottom-Right): Screen(%d, %d) -> Raw(X=%d, Y=%d)\n", CALIB_P2_X, CALIB_P2_Y, raw2X, raw2Y);
  Serial.printf("  Delta Raw X: %d | Delta Raw Y: %d\n", deltaRawX, deltaRawY);
  Serial.printf("  Detected Swap Axes: %s\n", cal_swapXY ? "YES (Horizontal=Raw Y, Vertical=Raw X)" : "NO");
  Serial.println(F("  Status: Successfully written to EEPROM flash!"));
  Serial.println(F("========================================================="));

  // نمایش تاییدیه روی صفحه نمایش
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(20, 65, 280, 110, 0x01E0);
  tft.drawRect(20, 65, 280, 110, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(45, 85);
  tft.println("CALIBRATION OK!");
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(45, 120);
  tft.println("Saved to EEPROM Flash memory.");
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(45, 140);
  tft.println("Loading canvas...");

  delay(1800);
  initUserInterface();
}

// =================================================================================
// راه‌اندازی و بازسازی رابط کاربری کامل (UI Initialization)
// =================================================================================
void initUserInterface() {
  tft.fillScreen(COLOR_BG);
  drawHeader("Interactive Paint & Touch Test");
  drawToolPanel();
  drawCanvasArea();
  isTouching = false;
  lastTouchX = -1;
  lastTouchY = -1;
}

// =================================================================================
// مدیریت کلیک روی دکمه‌های نوار ابزار سمت چپ
// =================================================================================
void handleToolPanelTouch(int sx, int sy) {
  // ۱. دکمه CLEAR
  if (sx >= 6 && sx <= 56 && sy >= 164 && sy <= 182) {
    Serial.println(F("[ACTION] Clear Canvas pressed."));
    drawCanvasArea();
    delay(150);
    return;
  }

  // ۲. دکمه TEST
  if (sx >= 6 && sx <= 56 && sy >= 187 && sy <= 205) {
    Serial.println(F("[ACTION] Test Benchmark pressed."));
    runGraphicsDemo();
    return;
  }

  // ۳. دکمه CALIB
  if (sx >= 6 && sx <= 56 && sy >= 210 && sy <= 228) {
    Serial.println(F("[ACTION] Calibrate Wizard pressed."));
    runTouchCalibration();
    return;
  }

  // بررسی انتخاب رنگ از پالت
  int startY = 47;
  int btnH = 12;
  int btnW = 46;

  for (int i = 0; i < NUM_COLORS; i++) {
    int by = startY + (i * (btnH + 2));
    if (sx >= 8 && sx <= (8 + btnW) && sy >= by && sy <= (by + btnH)) {
      currentColor = PALETTE_COLORS[i];
      Serial.print(F("[ACTION] Color selected: 0x"));
      Serial.println(currentColor, HEX);
      drawToolPanel();
      delay(100);
      return;
    }
  }
}

// =================================================================================
// تنظیمات اولیه (Setup)
// =================================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println(F("========================================================="));
  Serial.println(F("  NodeMCU ESP8266 + 2.8\" SPI TFT Touch Test Program     "));
  Serial.println(F("  Display: ILI9341 (320x240) | Touch: XPT2046 SPI       "));
  Serial.println(F("========================================================="));
  Serial.printf("ESP8266 Chip ID: %08X\n", ESP.getChipId());
  Serial.printf("CPU Frequency:   %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap:       %d bytes\n", ESP.getFreeHeap());
  Serial.println(F("Initializing SPI Display & Touch Controller..."));

  // راه‌اندازی نمایشگر TFT در حالت لنداسکیپ (Landscape 320x240)
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  // راه‌اندازی کنترلر تاچ XPT2046 در حالت Raw (بدون اعمال چرخش نرم‌افزاری کتابخانه)
  ts.begin();
  ts.setRotation(0);

  // خواندن ضرایب ذخیره شده در EEPROM
  EEPROM.begin(64);
  TouchCalibrationData calData;
  EEPROM.get(0, calData);
  if (calData.magic == EEPROM_MAGIC) {
    cal_p1_rawX = calData.p1_x;
    cal_p1_rawY = calData.p1_y;
    cal_p2_rawX = calData.p2_x;
    cal_p2_rawY = calData.p2_y;
    cal_swapXY  = calData.swap;
    Serial.println(F("[CALIB] Successfully loaded calibrated data from EEPROM!"));
  } else {
    Serial.println(F("[CALIB] Using pre-calibrated default Landscape parameters."));
  }

  Serial.println(F("Hardware initialized successfully!"));
  Serial.println(F("NOTE: To calibrate touch screen at any time:"));
  Serial.println(F("      - Type 'c' in Serial Monitor, OR"));
  Serial.println(F("      - Press the blue [CALIB] button on the screen panel."));

  // بارگذاری صفحه رابط کاربری
  initUserInterface();
}

// =================================================================================
// حلقه اصلی برنامه (Main Loop)
// =================================================================================
void loop() {
  // دریافت دستور کالیبراسیون از مانیتور سریال
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'c' || ch == 'C') {
      runTouchCalibration();
      return;
    }
  }

  int screenX = 0;
  int screenY = 0;
  int rawX = 0;
  int rawY = 0;
  int rawZ = 0;

  bool touched = getTouchCoordinates(screenX, screenY, rawX, rawY, rawZ);

  if (touched) {
    lastTouchTime = millis();

    // چاپ مشخصات لمس در مانیتور سریال
    Serial.printf("[TOUCH] Raw: X=%04d, Y=%04d, Z=%04d  -->  Screen: X=%03d, Y=%03d\n",
                  rawX, rawY, rawZ, screenX, screenY);

    // ۱. اگر لمس در محدوده بوم نقاشی باشد (Drawing Canvas)
    if (screenX >= (CANVAS_X + brushSize) && screenX <= (CANVAS_X + CANVAS_W - brushSize) &&
        screenY >= (CANVAS_Y + brushSize) && screenY <= (CANVAS_Y + CANVAS_H - brushSize)) {
      
      if (isTouching && lastTouchX != -1 && lastTouchY != -1) {
        tft.drawLine(lastTouchX, lastTouchY, screenX, screenY, currentColor);
        tft.fillCircle(screenX, screenY, brushSize, currentColor);
      } else {
        tft.fillCircle(screenX, screenY, brushSize, currentColor);
      }

      lastTouchX = screenX;
      lastTouchY = screenY;
    }
    // ۲. اگر لمس در نوار ابزار سمت چپ باشد
    else if (screenX < CANVAS_X) {
      handleToolPanelTouch(screenX, screenY);
      lastTouchX = -1;
      lastTouchY = -1;
    }

    if (!isTouching) {
      isTouching = true;
      updateTouchStatusIndicator(true, screenX, screenY);
    } else {
      updateTouchStatusIndicator(true, screenX, screenY);
    }

  } else {
    // قطع لمس
    if (isTouching && (millis() - lastTouchTime > 150)) {
      isTouching = false;
      lastTouchX = -1;
      lastTouchY = -1;
      updateTouchStatusIndicator(false, 0, 0);
    }
  }

  delay(10);
}
