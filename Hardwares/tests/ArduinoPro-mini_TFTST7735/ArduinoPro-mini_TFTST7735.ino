/**********************************************************************************
 * پروژه تست و راه‌اندازی نمایشگر رنگی 1.44 اینچ TFT با درایور ST7735 و برد Arduino Pro Mini 3.3V
 * 
 * سخت‌افزارهای مورد استفاده (از انبار سخت‌افزار):
 * 1. ماژول نمایشگر تمام رنگ 1.44 اینچ SPI با چیپ درایور ST7735 (کد کالا: LCD-01-155)
 *    - درایور کنترلر: Sitronix ST7735R
 *    - رزولوشن: 128x128 پیکسل
 *    - تعداد رنگ: 65,536 رنگ (16-bit RGB565)
 *    - رابط ارتباطی: گذرگاه سخت‌افزاری 4-Wire SPI
 * 
 * 2. برد پردازنده آردوینو پرومینی مدل 3.3 ولت 8 مگاهرتز (کد کالا: ARD-01-049)
 *    - میکروکنترلر: Microchip ATmega328P
 *    - سطح ولتاژ کاری: 3.3V (کاملاً منطبق بر سطح منطقی ST7735 بدون نیاز به Level Shifter)
 *    - فرکانس کاری: 8 MHz
 *    - حافظه رم: 2048 بایت (2KB SRAM)
 * 
 * کتابخانه‌های مورد نیاز (نصب از طریق Arduino Library Manager):
 * - Adafruit GFX Library (توسط Adafruit)
 * - Adafruit ST7735 and ST7789 Library (توسط Adafruit)
 * 
 * =================================================================================
 * جدول اتصالات و پین‌اوت کابل فلت ۱۴ پین نمایشگر (Z144SN005 14-Pin FPC Pinout):
 * روی فلت یا آداپتور این پنل نام پین‌ها چاپ نشده و فقط شماره ۱ تا ۱۴ مشخص است.
 * اتصالات بر اساس مپ رسمی سخت‌افزار (Z144SN005) به صورت زیر است:
 * =================================================================================
 * شماره پین فلت | نام سیگنال | پین Arduino Pro Mini (3.3V) | توضیحات اتصال
 * ---------------------------------------------------------------------------------
 *  1            | NC         | نامتصل (No Connect)         | پین آزاد
 *  2            | GND        | GND                         | زمین مدار (مشترک)
 *  3            | LED-       | GND                         | کاتد ال‌ای‌دی بک‌لایت
 *  4            | LED+       | VCC (3.3V) با مقاومت 22Ω    | آند بک‌لایت (یا پین D6 آردوینو با PWM)
 *  5            | GND        | GND                         | زمین مدار
 *  6            | /RESET     | D8                          | ریست سخت‌افزاری نمایشگر (Active Low)
 *  7            | A0 / DC    | D9                          | انتخاب داده / فرمان (Data / Command)
 *  8            | SDA (MOSI) | D11 (MOSI)                  | خط ارسال داده سخت‌افزاری SPI
 *  9            | SCK (CLK)  | D13 (SCK)                   | خط کلاک سخت‌افزاری SPI
 * 10            | VCC        | VCC (3.3V)                  | تغذیه اصلی درایور کنترلر ST7735
 * 11            | IOVCC      | VCC (3.3V)                  | تغذیه خطوط منطقی دیجیتال (3.3 ولت)
 * 12            | CS         | D10 (SS)                    | انتخاب تراشه روی باس SPI (Active Low)
 * 13            | GND        | GND                         | زمین مدار
 * 14            | NC         | نامتصل (No Connect)         | پین آزاد
 * ---------------------------------------------------------------------------------
 * 
 * خلاصه اتصالات به تفکیک پایه‌های آردوینو Pro Mini:
 * - پایه‌های GND آردوینو: متصل به پین‌های 2 و 3 و 5 و 13 نمایشگر
 * - پایه‌های VCC (3.3V) آردوینو: متصل به پین‌های 10 و 11 (و پین 4 با مقاومت 22 اهم یا به D6)
 * - پین D8 آردوینو: متصل به پین 6 (/RESET)
 * - پین D9 آردوینو: متصل به پین 7 (A0/DC)
 * - پین D10 آردوینو: متصل به پین 12 (CS)
 * - پین D11 آردوینو: متصل به پین 8 (SDA)
 * - پین D13 آردوینو: متصل به پین 9 (SCK)
 * - پین‌های 1 و 14 نمایشگر: آزاد و بدون اتصال (NC)
 * 
 * قابلیت‌های برنامه تست:
 * ۱. صفحه ۱: کارت هویت و اطلاعات سخت‌افزاری سیستم (مشخصات برد، نمایشگر و میزان رم آزاد)
 * ۲. صفحه ۲: تست وفاداری رنگ‌های اصلی ۱۶ بیتی RGB565 و شیب خاکستری (Grayscale)
 * ۳. صفحه ۳: بنچ‌مارک اشکال هندسی (دوایر، مستطیل‌ها، مثلث‌ها و خطوط قطری)
 * ۴. صفحه ۴: شبیه‌ساز داشبورد مانیتورینگ سنسور اینترنت اشیاء (گیج و نوار پیشرفت متحرک)
 * ۵. صفحه ۵: تست تایپوگرافی و مقیاس فونت‌ها (سایز ۱، ۲، ۳ و متن‌های معکوس)
 * ۶. صفحه ۶: انیمیشن روان توپ جهنده برای سنجش نرخ رفرش ریت صفحه (Bounce Demo)
 * ۷. منوی فرمان تعاملی از طریق Serial Monitor با باودریت 115200 (ارسال اعداد ۱ تا ۶ یا دستورات کنترلی)
 **********************************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// =================================================================================
// 1. تعاریف پین‌های سخت‌افزاری (Hardware Pin Mapping)
// =================================================================================
#define TFT_CS   10  // پایه Chip Select نمایشگر
#define TFT_RST   8  // پایه Reset نمایشگر
#define TFT_DC    9  // پایه Data/Command (A0)
#define TFT_LED   6  // پایه کنترل نور پس‌زمینه (اختیاری با PWM - اگر به VCC وصل نیست)

// ساخت شیء درایور نمایشگر با پورت سخت‌افزاری SPI (MOSI=11, SCK=13)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// =================================================================================
// 2. تعاریف رنگ‌های پرکاربرد ۱۶ بیتی (16-bit RGB565 Color Definitions)
// =================================================================================
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_YELLOW      0xFFE0
#define COLOR_ORANGE      0xFD20
#define COLOR_NAVY        0x000F
#define COLOR_DARKGREY    0x39C7
#define COLOR_LIGHTGREY   0xC618
#define COLOR_BACKGROUND  0x0842  // خاکستری بسیار تیره متمایل به لاجوردی

// =================================================================================
// 3. متغیرهای کنترل حالت تست و زمان‌بندی
// =================================================================================
uint8_t currentScreen = 1;
const uint8_t TOTAL_SCREENS = 6;
bool autoCycleMode = true;
unsigned long lastScreenSwitchTime = 0;
const unsigned long SCREEN_DURATION_MS = 5000; // توقف ۵ ثانیه روی هر صفحه در حالت خودکار

uint8_t currentRotation = 0;
bool isInverted = false;
uint8_t backlightBrightness = 255;

// متغیرهای انیمیشن توپ در صفحه ۶
int ballX = 64;
int ballY = 64;
int ballRadius = 6;
int ballSpeedX = 2;
int ballSpeedY = 3;

// متغیر شبیه‌ساز سنسور در صفحه ۴
int sensorValue = 25;
int sensorDir = 1;

// =================================================================================
// تابع سنجش حافظه رم آزاد (Free SRAM Measurement for ATmega328P)
// =================================================================================
int getFreeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// پیش‌اعلان توابع صفحات تست
void drawScreen1_Info();
void drawScreen2_Colors();
void drawScreen3_Geometry();
void drawScreen4_IoTDashboard();
void drawScreen5_Typography();
void initScreen6_Animation();
void updateScreen6_Animation();
void showHelpMenu();
void applyScreen(uint8_t screenNum);

// =================================================================================
// تنظیمات اولیه سیستم (Arduino Setup)
// =================================================================================
void setup() {
  // راه‌اندازی ارتباط سریال جهت دیباگ و ارسال دستورات تعاملی
  Serial.begin(115200);
  while (!Serial && millis() < 2000); // تاخیر کوتاه برای پایدار شدن ارتباط سریال

  Serial.println(F("\n========================================================"));
  Serial.println(F("  Arduino Pro Mini 3.3V + 1.44\" TFT ST7735 Test Suite  "));
  Serial.println(F("========================================================"));
  Serial.print(F("[INFO] Free SRAM at startup: "));
  Serial.print(getFreeRam());
  Serial.println(F(" bytes (out of 2048)"));

  // پیکربندی پین بک‌لایت در صورت تمایل به کنترل نور
  pinMode(TFT_LED, OUTPUT);
  analogWrite(TFT_LED, backlightBrightness);

  Serial.println(F("[INIT] Initializing ST7735 128x128 display (Green Tab)..."));
  
  // راه‌اندازی نمایشگر 1.44 اینچ 128x128
  // در اکثر ماژول‌های 1.44 اینچی موجود، INITR_144GREENTAB آفست دقیق 128x128 را اعمال می‌کند
  tft.initR(INITR_144GREENTAB);
  
  tft.setRotation(currentRotation);
  tft.fillScreen(COLOR_BLACK);

  Serial.println(F("[OK] Display initialized successfully."));
  showHelpMenu();

  // نمایش صفحه نخست
  applyScreen(1);
  lastScreenSwitchTime = millis();
}

// =================================================================================
// حلقه اصلی برنامه (Arduino Main Loop)
// =================================================================================
void loop() {
  // ۱. بررسی ورودی‌های مانیتور سریال جهت کنترل تعاملی
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // نادیده گرفتن کاراکترهای پایان خط
    if (cmd == '\r' || cmd == '\n') return;

    if (cmd >= '1' && cmd <= '6') {
      autoCycleMode = false;
      currentScreen = cmd - '0';
      Serial.print(F("[NAV] Manual Jump -> Screen "));
      Serial.print(currentScreen);
      Serial.println(F(" (Auto-cycle Paused)"));
      applyScreen(currentScreen);
    } 
    else if (cmd == 'a' || cmd == 'A') {
      autoCycleMode = true;
      lastScreenSwitchTime = millis();
      Serial.println(F("[NAV] Auto-cycle Resumed."));
    } 
    else if (cmd == 'r' || cmd == 'R') {
      currentRotation = (currentRotation + 1) % 4;
      tft.setRotation(currentRotation);
      Serial.print(F("[DISP] Rotation changed to: "));
      Serial.print(currentRotation * 90);
      Serial.println(F(" deg"));
      applyScreen(currentScreen);
    } 
    else if (cmd == 'i' || cmd == 'I') {
      isInverted = !isInverted;
      tft.invertDisplay(isInverted);
      Serial.print(F("[DISP] Invert mode: "));
      Serial.println(isInverted ? F("ON") : F("OFF"));
    } 
    else if (cmd == 'm' || cmd == 'M') {
      Serial.print(F("[MEM] Free SRAM: "));
      Serial.print(getFreeRam());
      Serial.println(F(" bytes / 2048 bytes"));
    } 
    else if (cmd == 'b' || cmd == 'B') {
      backlightBrightness = (backlightBrightness == 255) ? 100 : ((backlightBrightness == 100) ? 20 : 255);
      analogWrite(TFT_LED, backlightBrightness);
      Serial.print(F("[PWM] Backlight set to: "));
      Serial.print((backlightBrightness * 100) / 255);
      Serial.println(F("%"));
    } 
    else if (cmd == '?' || cmd == 'h' || cmd == 'H') {
      showHelpMenu();
    }
  }

  // ۲. مدیریت انیمیشن‌های زنده در صفحات فعال
  if (currentScreen == 6) {
    updateScreen6_Animation();
  } 
  else if (currentScreen == 4) {
    // به‌روزرسانی پویای سنسور هر 100 میلی‌ثانیه بدون تاخیر مسدودکننده
    static unsigned long lastSensorUpdate = 0;
    if (millis() - lastSensorUpdate > 100) {
      lastSensorUpdate = millis();
      sensorValue += sensorDir;
      if (sensorValue >= 85) sensorDir = -1;
      if (sensorValue <= 15) sensorDir = 1;
      drawScreen4_IoTDashboard();
    }
  }

  // ۳. تعویض خودکار صفحات در حالت Auto-cycle
  if (autoCycleMode && (currentScreen != 6)) {
    if (millis() - lastScreenSwitchTime >= SCREEN_DURATION_MS) {
      currentScreen++;
      if (currentScreen > TOTAL_SCREENS) currentScreen = 1;
      applyScreen(currentScreen);
      lastScreenSwitchTime = millis();
    }
  } 
  else if (autoCycleMode && (currentScreen == 6)) {
    // در صفحه انیمیشن به مدت 8 ثانیه انیمیشن اجرا شده سپس به صفحه 1 برمی‌گردد
    if (millis() - lastScreenSwitchTime >= 8000) {
      currentScreen = 1;
      applyScreen(currentScreen);
      lastScreenSwitchTime = millis();
    }
  }
}

// =================================================================================
// تابع اعمال صفحه انتخاب‌شده
// =================================================================================
void applyScreen(uint8_t screenNum) {
  tft.fillScreen(COLOR_BACKGROUND);

  switch (screenNum) {
    case 1: drawScreen1_Info(); break;
    case 2: drawScreen2_Colors(); break;
    case 3: drawScreen3_Geometry(); break;
    case 4: drawScreen4_IoTDashboard(); break;
    case 5: drawScreen5_Typography(); break;
    case 6: initScreen6_Animation(); break;
  }
}

// =================================================================================
// صفحه ۱: کارت مشخصات سخت‌افزاری و سیستم
// =================================================================================
void drawScreen1_Info() {
  // هدر بالا
  tft.fillRect(0, 0, 128, 20, 0x0015);
  tft.drawFastHLine(0, 20, 128, COLOR_CYAN);
  
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(1);
  tft.setCursor(6, 6);
  tft.print(F("HARDWARE PROFILE"));

  // کادرهای مشخصات
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 26);
  tft.print(F("MCU: "));
  tft.setTextColor(COLOR_WHITE);
  tft.println(F("ATmega328P"));

  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 38);
  tft.print(F("Freq: "));
  tft.setTextColor(COLOR_WHITE);
  tft.println(F("8 MHz (3.3V)"));

  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 50);
  tft.print(F("TFT: "));
  tft.setTextColor(COLOR_WHITE);
  tft.println(F("ST7735 (1.44\")"));

  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 62);
  tft.print(F("Res: "));
  tft.setTextColor(COLOR_WHITE);
  tft.println(F("128x128 SPI"));

  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(4, 74);
  tft.print(F("Color: "));
  tft.setTextColor(COLOR_WHITE);
  tft.println(F("16-Bit RGB565"));

  // نوار وضعیت حافظه رم
  int freeRam = getFreeRam();
  tft.drawRoundRect(4, 88, 120, 24, 4, COLOR_LIGHTGREY);
  tft.fillRect(6, 90, 116, 20, COLOR_BLACK);
  
  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(10, 96);
  tft.print(F("RAM Free: "));
  tft.print(freeRam);
  tft.print(F("B"));

  // فوتر
  tft.drawFastHLine(0, 116, 128, COLOR_DARKGREY);
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(20, 119);
  tft.print(F("[ Page 1 of 6 ]"));
}

// =================================================================================
// صفحه ۲: نوارهای رنگی و شیب خاکستری (Color Fidelity Benchmark)
// =================================================================================
void drawScreen2_Colors() {
  tft.fillRect(0, 0, 128, 16, COLOR_BLACK);
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(12, 4);
  tft.print(F("COLOR BARS TEST"));

  // ۸ نوار رنگی عمودی
  const uint16_t colors[8] = {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW,
    COLOR_CYAN, COLOR_MAGENTA, COLOR_WHITE, COLOR_ORANGE
  };

  const uint8_t barWidth = 16;
  for (uint8_t i = 0; i < 8; i++) {
    tft.fillRect(i * barWidth, 18, barWidth, 68, colors[i]);
  }

  // شیب خاکستری پله‌ای در پایین (Grayscale Ramp)
  for (uint8_t step = 0; step < 16; step++) {
    uint8_t c = step * 16 + (step > 0 ? 15 : 0);
    // ساخت رنگ RGB565 خاکستری متناظر: R(5bit)=c>>3, G(6bit)=c>>2, B(5bit)=c>>3
    uint16_t gray565 = tft.color565(c, c, c);
    tft.fillRect(step * 8, 90, 8, 22, gray565);
  }

  tft.drawRect(0, 90, 128, 22, COLOR_DARKGREY);

  // فوتر
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(20, 118);
  tft.print(F("[ Page 2 of 6 ]"));
}

// =================================================================================
// صفحه ۳: بنچ‌مارک اشکال هندسی (2D Geometry Benchmark)
// =================================================================================
void drawScreen3_Geometry() {
  tft.fillRect(0, 0, 128, 16, COLOR_NAVY);
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(1);
  tft.setCursor(8, 4);
  tft.print(F("GEOMETRY BENCH"));

  // مستطیل‌های تودرتو در بخش چپ
  for (int r = 0; r < 24; r += 4) {
    tft.drawRect(8 + r, 24 + r, 48 - (2 * r), 48 - (2 * r), COLOR_ORANGE);
  }

  // دوایر هم‌مرکز در بخش راست
  for (int rad = 4; rad <= 24; rad += 5) {
    tft.drawCircle(96, 48, rad, COLOR_CYAN);
  }

  // مثلث‌های توپر و خطوط ضربدری
  tft.fillTriangle(10, 80, 35, 110, 60, 80, COLOR_GREEN);
  tft.drawTriangle(10, 80, 35, 110, 60, 80, COLOR_WHITE);

  tft.drawLine(70, 80, 118, 110, COLOR_RED);
  tft.drawLine(118, 80, 70, 110, COLOR_YELLOW);
  tft.drawRoundRect(68, 78, 52, 34, 6, COLOR_MAGENTA);

  // فوتر
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(20, 119);
  tft.print(F("[ Page 3 of 6 ]"));
}

// =================================================================================
// صفحه ۴: شبیه‌ساز داشبورد اینترنت اشیاء (IoT Telemetry Dashboard)
// =================================================================================
void drawScreen4_IoTDashboard() {
  // این تابع هم برای ترسیم کامل و هم برای آپدیت فریم‌های سنسور فراخوانی می‌شود
  static bool layoutDrawn = false;

  // اگر اولین بار در این صفحه است، اسکلت داشبورد ترسیم می‌شود
  if (currentScreen != 4) {
    layoutDrawn = false;
    return;
  }

  // هدر داشبورد
  tft.fillRect(0, 0, 128, 18, 0x18C3);
  tft.setTextColor(COLOR_GREEN);
  tft.setTextSize(1);
  tft.setCursor(6, 5);
  tft.print(F("IOT TELEMETRY"));

  // وضعیت سیستم
  tft.fillCircle(118, 9, 3, COLOR_GREEN);

  // نمایش گیج بارگراف سنسور
  tft.setTextColor(COLOR_YELLOW);
  tft.setCursor(6, 26);
  tft.print(F("TEMP SIM: "));
  
  // پاکسازی عدد قبلی
  tft.fillRect(70, 24, 52, 12, COLOR_BACKGROUND);
  tft.setTextColor(COLOR_WHITE);
  tft.print(sensorValue);
  tft.print(F(" C"));

  // کادر بارگراف
  tft.drawRoundRect(6, 42, 116, 16, 3, COLOR_CYAN);
  int fillWidth = map(sensorValue, 0, 100, 0, 112);
  tft.fillRect(8, 44, fillWidth, 12, (sensorValue > 65) ? COLOR_RED : COLOR_GREEN);
  tft.fillRect(8 + fillWidth, 44, 112 - fillWidth, 12, COLOR_BLACK);

  // پارامتر دوم: رطوبت فرضی
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(6, 66);
  tft.print(F("HUMIDITY: "));
  tft.setTextColor(COLOR_WHITE);
  tft.print(100 - (sensorValue / 2));
  tft.print(F(" %"));

  // کارت زمان کارکرد (Uptime)
  tft.drawFastHLine(6, 84, 116, COLOR_DARKGREY);
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(6, 92);
  tft.print(F("Uptime: "));
  tft.setTextColor(COLOR_WHITE);
  tft.print(millis() / 1000);
  tft.print(F("s"));

  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(6, 104);
  tft.print(F("Bus: SPI 4MHz"));

  // فوتر
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(20, 119);
  tft.print(F("[ Page 4 of 6 ]"));
}

// =================================================================================
// صفحه ۵: آزمون تایپوگرافی و فونت‌ها (Typography & Font Scaling)
// =================================================================================
void drawScreen5_Typography() {
  tft.fillRect(0, 0, 128, 16, COLOR_BLACK);
  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(14, 4);
  tft.print(F("TYPOGRAPHY TEST"));

  // سایز ۱ فونت
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(6, 24);
  tft.println(F("Size 1: ABCabc 123"));

  // سایز ۲ فونت
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(2);
  tft.setCursor(6, 38);
  tft.println(F("Size 2"));

  // سایز ۳ فونت
  tft.setTextColor(COLOR_ORANGE);
  tft.setTextSize(3);
  tft.setCursor(6, 60);
  tft.println(F("ST7735"));

  // متن هایلایت شده با پس‌زمینه رنگی معکوس
  tft.setTextSize(1);
  tft.setTextColor(COLOR_BLACK, COLOR_GREEN); // نوشته مشکی با کادر سبز
  tft.setCursor(6, 92);
  tft.print(F(" INVERTED HIGHLIGHT "));

  tft.setTextColor(COLOR_WHITE, COLOR_RED);
  tft.setCursor(6, 104);
  tft.print(F(" STATUS: ALL OK     "));

  // فوتر
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(20, 119);
  tft.print(F("[ Page 5 of 6 ]"));
}

// =================================================================================
// صفحه ۶: انیمیشن روان توپ جهنده (Smooth Bouncing Animation & FPS)
// =================================================================================
void initScreen6_Animation() {
  tft.fillScreen(COLOR_BLACK);
  tft.drawRect(0, 0, 128, 128, COLOR_CYAN);

  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(16, 6);
  tft.print(F("BOUNCE ANIMATION"));

  ballX = 64;
  ballY = 64;
  ballSpeedX = 3;
  ballSpeedY = 2;
}

void updateScreen6_Animation() {
  static unsigned long lastFrameTime = 0;
  static int frameCount = 0;
  static int currentFps = 0;

  // محاسبه ساده نرخ فریم بر ثانیه (FPS)
  frameCount++;
  if (millis() - lastFrameTime >= 1000) {
    currentFps = frameCount;
    frameCount = 0;
    lastFrameTime = millis();
    
    // چاپ نرخ فریم در پایین کادر
    tft.fillRect(10, 112, 108, 10, COLOR_BLACK);
    tft.setTextColor(COLOR_GREEN);
    tft.setTextSize(1);
    tft.setCursor(12, 113);
    tft.print(F("FPS: ~"));
    tft.print(currentFps);
    tft.print(F(" | RAM: "));
    tft.print(getFreeRam());
  }

  // ۱. پاکسازی موقعیت قبلی توپ
  tft.fillCircle(ballX, ballY, ballRadius, COLOR_BLACK);

  // ۲. به‌روزرسانی مختصات جدید
  ballX += ballSpeedX;
  ballY += ballSpeedY;

  // برخورد به دیواره‌های چپ و راست
  if (ballX - ballRadius <= 2 || ballX + ballRadius >= 125) {
    ballSpeedX = -ballSpeedX;
  }

  // برخورد به سقف و کف
  if (ballY - ballRadius <= 18 || ballY + ballRadius >= 110) {
    ballSpeedY = -ballSpeedY;
  }

  // ۳. ترسیم توپ در مختصات جدید
  tft.fillCircle(ballX, ballY, ballRadius, COLOR_RED);
  tft.drawCircle(ballX, ballY, ballRadius, COLOR_WHITE);

  // تاخیر کوتاه برای کنترل سرعت و روانی انیمیشن
  delay(15);
}

// =================================================================================
// منوی راهنمای سریال
// =================================================================================
void showHelpMenu() {
  Serial.println(F("\n----------------- Interactive Commands -----------------"));
  Serial.println(F("  [1-6]  : Jump directly to Screen 1 to 6"));
  Serial.println(F("  [A]    : Resume Auto-cycle mode (5s per page)"));
  Serial.println(F("  [R]    : Rotate Display by 90 degrees (0, 90, 180, 270)"));
  Serial.println(F("  [I]    : Toggle Color Inversion"));
  Serial.println(F("  [B]    : Cycle Backlight Brightness (100% -> 40% -> 8%)"));
  Serial.println(F("  [M]    : Print Free SRAM memory"));
  Serial.println(F("  [?]    : Show this help menu"));
  Serial.println(F("--------------------------------------------------------\n"));
}
