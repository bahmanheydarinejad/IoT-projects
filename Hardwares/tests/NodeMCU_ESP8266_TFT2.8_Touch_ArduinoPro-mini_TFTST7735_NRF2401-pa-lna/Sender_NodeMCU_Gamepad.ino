/**********************************************************************************
 * پروژه فرستنده دسته بازی پلی‌استیشن لمسی (PlayStation Touch Gamepad Transmitter)
 * بستر سخت‌افزاری: NodeMCU ESP8266 + نمایشگر 2.8 اینچ لمسی SPI + ماژول NRF24L01+PA+LNA
 * 
 * سخت‌افزارهای مورد استفاده:
 * 1. برد NodeMCU دارای پردازنده ESP8266 (کد کالا: COM-03-009)
 * 2. ماژول نمایشگر 2.8 اینچ TFT لمسی با درایور ILI9341 و تاچ XPT2046 (کد کالا: LCD-01-280)
 * 3. ماژول فرستنده/گیرنده رادیویی NRF24L01+PA+LNA با برد ۱ کیلومتر (کد کالا: COM-09-030)
 * 
 * اتصالات سیم‌بندی کامل (Wiring Pinout - بدون تداخل بر روی باس SPI):
 * ---------------------------------------------------------------------------------
 * پین قطعه             | اتصال به NodeMCU  | شرح عملکرد
 * ---------------------------------------------------------------------------------
 * TFT_CS (نمایشگر)     | D1 (GPIO5)        | انتخاب تراشه تصویر ILI9341
 * TFT_RST (نمایشگر)    | D2 (GPIO4)        | ریست سخت‌افزاری نمایشگر
 * TFT_DC (نمایشگر)     | D3 (GPIO0)        | انتخاب داده/فرمان تصویر
 * TOUCH_CS (تاچ)       | D8 (GPIO15)       | انتخاب تراشه کنترلر لمسی XPT2046
 * NRF_CE (رادیو)       | D0 (GPIO16)       | فعال‌ساز حالت ارسال/دریافت NRF24
 * NRF_CSN (رادیو)      | D4 (GPIO2)        | انتخاب تراشه رادیو NRF24 روی باس SPI
 * SPI SCK (مشترک)      | D5 (GPIO14)       | کلاک مشترک سخت‌افزاری SPI (نمایشگر + تاچ + رادیو)
 * SPI MISO (مشترک)     | D6 (GPIO12)       | خط دیتای ورودی مشترک (خروجی تاچ + خروجی رادیو)
 * SPI MOSI (مشترک)     | D7 (GPIO13)       | خط دیتای خروجی مشترک (ورودی تصویر + تاچ + رادیو)
 * VCC قطعات            | 3V3               | تغذیه ۳.۳ ولت (خازن ۱۰ الی ۱۰۰ میکروفاراد روی NRF الزامی است)
 * GND قطعات            | GND               | زمین مشترک مدار
 * ---------------------------------------------------------------------------------
 **********************************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <RF24.h>
#include "PacketProtocol.h"

// =================================================================================
// 1. تعاریف پین‌های سخت‌افزاری
// =================================================================================
#define TFT_CS    D1   // GPIO5
#define TFT_RST   D2   // GPIO4
#define TFT_DC    D3   // GPIO0

#define TOUCH_CS  D8   // GPIO15

#define NRF_CE    D0   // GPIO16
#define NRF_CSN   D4   // GPIO2

// ساخت اشیاء ماژول‌ها
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);
RF24 radio(NRF_CE, NRF_CSN);

// =================================================================================
// 2. پالت رنگ‌های تم پلی‌استیشن (PS5 / Cyber Dark Theme)
// =================================================================================
#define COLOR_BG          0x0842   // خاکستری بسیار تیره متمایل به سورمه‌ای
#define COLOR_BAR         0x10A4   // نوار ابزار بالا
#define COLOR_BTN_BG      0x2125   // پس‌زمینه پیش‌فرض دکمه‌ها
#define COLOR_BTN_BORDER  0x4A6B   // کادر بیرونی دکمه‌ها
#define COLOR_BTN_ACTIVE  0x05BF   // رنگ هایلایت هنگام لمس (آبی نئونی)
#define COLOR_WHITE       0xFFFF
#define COLOR_TEXT_MUTED  0x9CD3

#define PS_TRIANGLE_COLOR 0x07E0   // سبز
#define PS_CIRCLE_COLOR   0xF800   // قرمز
#define PS_CROSS_COLOR    0x041F   // آبی
#define PS_SQUARE_COLOR   0xF81F   // سرخابی/صورتی

#define COLOR_STATUS_OK   0x07E0   // سبز اتصال
#define COLOR_STATUS_ERR  0xF800   // قرمز قطعی
#define COLOR_TX_PULSE    0xFFE0   // زرد ارسال

// =================================================================================
// 3. ساختار و هندسه کلیدهای لمسی (Touch Button Layout)
// =================================================================================
struct TouchButton {
  int16_t  x, y, w, h;
  uint16_t mask;
  const char* label;
  uint16_t symbolColor;
  bool isPressed;
};

// تعریف ۱۲ کلید دسته پلی‌استیشن
const uint8_t NUM_BUTTONS = 12;
TouchButton buttons[NUM_BUTTONS] = {
  // D-Pad (سمت چپ صفحه)
  { 62,  75, 42, 42, BTN_UP,        "^",   COLOR_WHITE,       false },
  { 62, 169, 42, 42, BTN_DOWN,      "v",   COLOR_WHITE,       false },
  { 15, 122, 42, 42, BTN_LEFT,      "<",   COLOR_WHITE,       false },
  {109, 122, 42, 42, BTN_RIGHT,     ">",   COLOR_WHITE,       false },

  // کلیدهای اکشن پلی‌استیشن (سمت راست صفحه)
  {216,  75, 42, 42, BTN_TRIANGLE,  "/\\", PS_TRIANGLE_COLOR, false },
  {263, 122, 42, 42, BTN_CIRCLE,    "O",   PS_CIRCLE_COLOR,   false },
  {216, 169, 42, 42, BTN_CROSS,     "X",   PS_CROSS_COLOR,    false },
  {169, 122, 42, 42, BTN_SQUARE,    "[]",  PS_SQUARE_COLOR,   false },

  // کلیدهای گوشواره و میانی (بالای صفحه)
  { 15,  35, 65, 28, BTN_L1,        "L1",     COLOR_WHITE,       false },
  {240,  35, 65, 28, BTN_R1,        "R1",     COLOR_WHITE,       false },
  { 95,  35, 58, 28, BTN_SELECT,    "SELECT", COLOR_TEXT_MUTED,  false },
  {167,  35, 58, 28, BTN_START,     "START",  COLOR_TEXT_MUTED,  false }
};

// متغیرهای وضعیت سیستم و رادیو
uint8_t packetSequence = 0;
uint16_t currentButtonMask = 0;
uint16_t lastSentMask = 0xFFFF;
bool isReceiverConnected = false;
uint8_t consecutiveFailures = 0;
unsigned long lastTxTime = 0;
unsigned long lastStatusRefresh = 0;
uint32_t totalPacketsSent = 0;
uint32_t totalPacketsAcked = 0;
bool pulseTxIcon = false;

// ضرایب تبدیل کالیبراسیون تاچ (Landscape Rotation 1)
const int TS_MINX = 300, TS_MAXX = 3800;
const int TS_MINY = 300, TS_MAXY = 3800;

// پیش‌اعلان توابع
void updateStatusBar();
void drawSingleButton(int index);

// =================================================================================
// توابع گرافیکی و ترسیم رابط کاربری
// =================================================================================
void drawHeaderBar() {
  tft.fillRect(0, 0, 320, 28, COLOR_BAR);
  tft.drawFastHLine(0, 28, 320, COLOR_BTN_BORDER);

  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.print(F("PS-CONTROLLER TX"));

  // چاپ کانال رادیویی
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setCursor(110, 10);
  tft.print(F("CH:115 (2.515G)"));

  updateStatusBar();
}

void updateStatusBar() {
  // پاکسازی بخش وضعیت در سربرگ
  tft.fillRect(205, 4, 112, 20, COLOR_BAR);

  // آیکون پالس رادیویی
  if (pulseTxIcon) {
    tft.fillCircle(212, 14, 5, COLOR_TX_PULSE);
  } else {
    tft.drawCircle(212, 14, 5, COLOR_TEXT_MUTED);
  }

  // برچسب وضعیت اتصال
  tft.setTextSize(1);
  if (isReceiverConnected) {
    tft.fillRect(222, 6, 92, 16, COLOR_STATUS_OK);
    tft.setTextColor(COLOR_BG);
    tft.setCursor(227, 10);
    tft.print(F("LINK: ONLINE"));
  } else {
    tft.fillRect(222, 6, 92, 16, COLOR_STATUS_ERR);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(225, 10);
    tft.print(F("LINK: OFFLINE"));
  }
}

void drawSingleButton(int index) {
  TouchButton &b = buttons[index];
  uint16_t bgColor = b.isPressed ? COLOR_BTN_ACTIVE : COLOR_BTN_BG;
  uint16_t borderCol = b.isPressed ? COLOR_WHITE : COLOR_BTN_BORDER;

  tft.fillRoundRect(b.x, b.y, b.w, b.h, 6, bgColor);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 6, borderCol);

  // ترسیم متن یا نماد
  tft.setTextSize(2);
  tft.setTextColor(b.symbolColor);

  // محاسبه تقریبی موقعیت متن در مرکز دکمه
  int16_t tx = b.x + (b.w / 2) - (strlen(b.label) * 6);
  int16_t ty = b.y + (b.h / 2) - 7;
  
  // تنظیم سایز فونت برای دکمه‌های کوچک‌تر Select/Start
  if (b.mask == BTN_SELECT || b.mask == BTN_START) {
    tft.setTextSize(1);
    tx = b.x + (b.w / 2) - (strlen(b.label) * 3);
    ty = b.y + (b.h / 2) - 4;
  }

  tft.setCursor(tx, ty);
  tft.print(b.label);
}

void drawAllButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    drawSingleButton(i);
  }

  // بخش تزئینی مرکز کنترلر
  tft.drawRoundRect(140, 75, 40, 36, 4, COLOR_BAR);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.setTextSize(1);
  tft.setCursor(144, 85);
  tft.print(F("WIRE"));
  tft.setCursor(148, 97);
  tft.print(F("LESS"));

  // نوار وضعیت پایین
  tft.drawFastHLine(0, 222, 320, COLOR_BAR);
  tft.setCursor(10, 228);
  tft.setTextColor(COLOR_TEXT_MUTED);
  tft.print(F("Antigravity RF Protocol | 250kbps Ultra-Range"));
}

// =================================================================================
// راه‌اندازی ماژول رادیویی NRF24L01+
// =================================================================================
void setupRadio() {
  Serial.println(F("[NRF24] Initializing NRF24L01+PA+LNA Transmitter..."));

  if (!radio.begin()) {
    Serial.println(F("[ERROR] NRF24 Hardware not responding! Check wiring and 3.3V bypass capacitor."));
    tft.setCursor(20, 110);
    tft.setTextColor(COLOR_STATUS_ERR);
    tft.setTextSize(2);
    tft.println(F("NRF24 INIT FAILED!"));
    while (1) delay(1000);
  }

  // پیکربندی پارامترهای ضدتداخل
  radio.setChannel(RF_CHANNEL_NUM);                  // کانال 115 (2.515 GHz - خارج از وای‌فای)
  radio.setDataRate(RF_250KBPS);                     // حداکثر برد موثر و حساسیت در 250kbps
  radio.setPALevel(RF24_PA_HIGH);                    // توان بالای ماژول PA+LNA
  radio.setCRCLength(RF24_CRC_16);                   // کشف خطای سخت‌افزاری ۱۶ بیتی
  radio.setAutoAck(true);                            // فعال‌سازی Auto-Acknowledgment دوطرفه
  radio.setRetries(3, 5);                            // تاخیر ۱۰۰۰ میکروثانیه بین تلاش‌ها، حداکثر ۵ بار تلاش

  // باز کردن لوله ارسال به آدرس اختصاصی
  radio.openWritingPipe(RF_PIPE_ADDRESS);
  radio.stopListening();                             // مد فرستنده (TX Mode)

  Serial.println(F("[NRF24] Radio configured successfully on Channel 115."));
}

// =================================================================================
// تابع تنظیمات اولیه آردوینو (Setup)
// =================================================================================
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n========================================================"));
  Serial.println(F("  PlayStation Touch Gamepad Controller (NodeMCU ESP8266)  "));
  Serial.println(F("========================================================"));

  // راه‌اندازی نمایشگر ۲.۸ اینچ
  tft.begin();
  tft.setRotation(1); // حالت افقی لنداسکیپ 320x240
  tft.fillScreen(COLOR_BG);

  // راه‌اندازی تاچ اسکرین
  ts.begin();
  ts.setRotation(1);

  // راه‌اندازی رادیو NRF24
  setupRadio();

  // ترسیم قالب کامل گرافیکی دسته بازی
  drawHeaderBar();
  drawAllButtons();

  Serial.println(F("[OK] System ready for touch inputs and RF transmission."));
}

// =================================================================================
// خواندن نقاط لمس و تبدیل به وضعیت دکمه‌ها
// =================================================================================
uint16_t scanTouchButtons() {
  uint16_t pressedMask = 0;

  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    // تبدیل و نگاشت مختصات ADC تاچ به پیکسل‌های نمایشگر 320x240
    int screenX = map(p.x, TS_MINX, TS_MAXX, 0, 320);
    int screenY = map(p.y, TS_MINY, TS_MAXY, 0, 240);

    // بررسی برخورد نقطه لمس با دکمه‌های تعریف شده
    for (int i = 0; i < NUM_BUTTONS; i++) {
      TouchButton &b = buttons[i];
      if (screenX >= b.x && screenX <= (b.x + b.w) &&
          screenY >= b.y && screenY <= (b.y + b.h)) {
        pressedMask |= b.mask;
      }
    }
  }

  return pressedMask;
}

// =================================================================================
// ارسال بسته داده از طریق رادیو NRF24
// =================================================================================
void transmitGamepadState(uint16_t mask) {
  GamepadPacket packet;
  preparePacket(packet, packetSequence++, mask);

  pulseTxIcon = true;
  updateStatusBar();

  // ارسال داده با انتظار برای پاسخ ACK سخت‌افزاری
  bool ackReceived = radio.write(&packet, sizeof(packet));

  totalPacketsSent++;
  if (ackReceived) {
    totalPacketsAcked++;
    consecutiveFailures = 0;
    isReceiverConnected = true;
  } else {
    consecutiveFailures++;
    // اگر بیش از ۵ ارسال متوالی بدون پاسخ بماند، وضعیت قطع اعلام می‌شود
    if (consecutiveFailures >= 5) {
      isReceiverConnected = false;
    }
  }

  pulseTxIcon = false;
  lastTxTime = millis();
}

// =================================================================================
// حلقه اصلی برنامه (Main Loop)
// =================================================================================
void loop() {
  // ۱. اسکن وضعیت دکمه‌ها از طریق صفحه لمسی
  currentButtonMask = scanTouchButtons();

  // ۲. به‌روزرسانی جلوه گرافیکی دکمه‌ها در صورت تغییر وضعیت
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool isNowPressed = (currentButtonMask & buttons[i].mask) != 0;
    if (isNowPressed != buttons[i].isPressed) {
      buttons[i].isPressed = isNowPressed;
      drawSingleButton(i);
    }
  }

  // ۳. ارسال بسته داده:
  // - هر زمان که وضعیت دکمه‌ها تغییر کند (ارسال فوری Low-latency)
  // - یا حداقل هر ۴۰ میلی‌ثانیه یک‌بار به عنوان Heartbeat مداوم برای حفظ پایداری لینک
  unsigned long now = millis();
  if ((currentButtonMask != lastSentMask) || (now - lastTxTime >= 40)) {
    transmitGamepadState(currentButtonMask);
    lastSentMask = currentButtonMask;
  }

  // ۴. به‌روزرسانی نوار وضعیت اتصال هر ۵۰۰ میلی‌ثانیه
  if (now - lastStatusRefresh >= 500) {
    lastStatusRefresh = now;
    updateStatusBar();
  }

  delay(10);
}
