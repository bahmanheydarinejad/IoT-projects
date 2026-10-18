/**********************************************************************************
 * پروژه گیرنده و مانیتورینگ دسته بازی پلی‌استیشن (PlayStation Gamepad Receiver)
 * بستر سخت‌افزاری: Arduino Pro Mini 3.3V (8MHz) + نمایشگر 1.44 اینچ TFT + ماژول NRF24L01+PA+LNA
 * 
 * سخت‌افزارهای مورد استفاده:
 * 1. برد پردازنده Arduino Pro Mini مدل 3.3 ولت 8 مگاهرتز (کد کالا: ARD-01-049)
 * 2. نمایشگر تمام رنگ 1.44 اینچ TFT با درایور ST7735 و فلت ۱۴ پین (کد کالا: LCD-01-155)
 * 3. ماژول فرستنده/گیرنده رادیویی NRF24L01+PA+LNA با برد ۱ کیلومتر (کد کالا: COM-09-030)
 * 
 * اتصالات سیم‌بندی کامل (Wiring Pinout - بدون تداخل بر روی باس SPI):
 * ---------------------------------------------------------------------------------
 * پین قطعه                 | اتصال به Pro Mini | شرح عملکرد
 * ---------------------------------------------------------------------------------
 * TFT_CS (پین ۱۲ فلت)      | D10 (SS)          | انتخاب تراشه نمایشگر روی باس SPI
 * TFT_DC (پین ۷ فلت)       | D9                | انتخاب داده / فرمان نمایشگر (A0)
 * TFT_RST (پین ۶ فلت)      | D8                | ریست سخت‌افزاری نمایشگر
 * TFT_LED (پین ۴ فلت)      | VCC (3.3V) با ۲۲Ω | نور پس‌زمینه نمایشگر (یا D6 با PWM)
 * NRF_CE (رادیو)           | D5                | فعال‌ساز مود کاری رادیو NRF24
 * NRF_CSN (رادیو)          | D7                | انتخاب تراشه رادیو NRF24 روی باس SPI
 * SPI SCK (مشترک)          | D13 (SCK)         | کلاک سخت‌افزاری SPI (مشترک رادیو و نمایشگر)
 * SPI MOSI (مشترک)         | D11 (MOSI)        | دیتای خروجی SPI (مشترک رادیو و نمایشگر)
 * SPI MISO (رادیو)         | D12 (MISO)        | دیتای ورودی میکرو از رادیو NRF24
 * VCC قطعات                | VCC (3.3V)        | تغذیه ۳.۳ ولت (خازن ۱۰ الی ۱۰۰ میکروفاراد روی NRF)
 * GND قطعات                | GND               | زمین مشترک مدار
 * ---------------------------------------------------------------------------------
 * 
 * پین‌های فلت ۱۴ پین نمایشگر (Z144SN005):
 * Pin 1: NC | Pin 2: GND | Pin 3: GND | Pin 4: 3.3V (22Ω) | Pin 5: GND
 * Pin 6: D8 | Pin 7: D9  | Pin 8: D11 | Pin 9: D13        | Pin 10,11: 3.3V
 * Pin 12: D10 | Pin 13: GND | Pin 14: NC
 **********************************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <RF24.h>
#include "PacketProtocol.h"

// =================================================================================
// 1. تعاریف پین‌های سخت‌افزاری (Arduino Pro Mini 3.3V)
// =================================================================================
#define TFT_CS    10  // پایه Chip Select نمایشگر
#define TFT_DC     9  // پایه Data/Command (A0)
#define TFT_RST    8  // پایه Reset نمایشگر

#define NRF_CE     5  // پایه Chip Enable رادیو
#define NRF_CSN    7  // پایه Chip Select Not رادیو

// ساخت اشیاء ماژول‌ها
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
RF24 radio(NRF_CE, NRF_CSN);

// =================================================================================
// 2. پالت رنگ‌های نمایشگر ۱۲۸×۱۲۸
// =================================================================================
#define COLOR_BG          0x0842   // زمینه تیره سورمه‌ای
#define COLOR_HEADER      0x10A4   // نوار سربرگ
#define COLOR_WHITE       0xFFFF
#define COLOR_DARKGREY    0x39E7
#define COLOR_LIGHTGREY   0x9CD3
#define COLOR_PANEL       0x18E5

#define PS_TRIANGLE_COLOR 0x07E0   // سبز نئونی
#define PS_CIRCLE_COLOR   0xF800   // قرمز
#define PS_CROSS_COLOR    0x041F   // آبی
#define PS_SQUARE_COLOR   0xF81F   // صورتی/سرخابی
#define COLOR_ACTIVE_BTN  0x07FF   // فیروزه‌ای برای D-Pad و کلیدهای جانبی

#define COLOR_STATUS_OK   0x07E0   // سبز اتصال
#define COLOR_STATUS_ERR  0xF800   // قرمز قطعی
#define COLOR_RX_PULSE    0xFFE0   // زرد پالس دریافت

// =================================================================================
// 3. متغیرهای وضعیت اتصال و بسته‌های داده
// =================================================================================
uint16_t activeButtons = 0;
uint16_t previousButtons = 0xFFFF;
uint32_t totalPacketsReceived = 0;
uint8_t lastPacketSeq = 0;
unsigned long lastPacketTime = 0;
bool isConnected = false;
bool wasConnected = false;
const unsigned long LINK_TIMEOUT_MS = 350; // اگر تا ۳۵۰ میلی‌ثانیه پکت نرسد وضعیت قطع اعلام می‌شود

// پیش‌اعلان توابع
void updateHeaderStatus(bool rxPulse);
void updateGamepadVisualizer(uint16_t mask);
void drawTelemetryInfo();

// =================================================================================
// توابع گرافیکی و ترسیم رابط کاربری (128x128 Pixel Display Layout)
// =================================================================================
void drawHeader() {
  tft.fillRect(0, 0, 128, 16, COLOR_HEADER);
  tft.drawFastHLine(0, 16, 128, COLOR_DARKGREY);

  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  tft.setCursor(4, 4);
  tft.print(F("PS RX"));

  updateHeaderStatus(false);
}

void updateHeaderStatus(bool rxPulse) {
  // پاکسازی محدوده آیکون و وضعیت در هدر
  tft.fillRect(40, 2, 86, 12, COLOR_HEADER);

  // آیکون پالس دریافت
  if (rxPulse) {
    tft.fillCircle(46, 8, 3, COLOR_RX_PULSE);
  } else {
    tft.drawCircle(46, 8, 3, COLOR_DARKGREY);
  }

  // نشانگر وضعیت اتصال
  tft.setTextSize(1);
  if (isConnected) {
    tft.fillRect(56, 3, 68, 10, COLOR_STATUS_OK);
    tft.setTextColor(COLOR_BG);
    tft.setCursor(62, 4);
    tft.print(F("LINK: OK"));
  } else {
    tft.fillRect(56, 3, 68, 10, COLOR_STATUS_ERR);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(59, 4);
    tft.print(F("NO LINK"));
  }
}

// ترسیم ماکت مینیاتوری دسته بازی در مرکز صفحه (Y: 20 الی 82)
void drawControllerSkeleton() {
  // بدنه اصلی ماکت کنترلر
  tft.fillRoundRect(14, 28, 100, 52, 10, COLOR_PANEL);
  tft.drawRoundRect(14, 28, 100, 52, 10, COLOR_DARKGREY);

  // دسته‌های چپ و راست (Grips)
  tft.fillRoundRect(10, 48, 24, 34, 6, COLOR_PANEL);
  tft.drawRoundRect(10, 48, 24, 34, 6, COLOR_DARKGREY);
  tft.fillRoundRect(94, 48, 24, 34, 6, COLOR_PANEL);
  tft.drawRoundRect(94, 48, 24, 34, 6, COLOR_DARKGREY);

  // خط میانی تزئینی
  tft.drawFastHLine(50, 45, 28, COLOR_DARKGREY);
}

// به‌روزرسانی تک‌تک کلیدها روی ماکت مینیاتوری
void updateGamepadVisualizer(uint16_t mask) {
  // ۱. دکمه‌های D-Pad (سمت چپ ماکت: مرکز حدود X=34, Y=48)
  bool up = (mask & BTN_UP) != 0;
  tft.fillRect(31, 34, 7, 7, up ? COLOR_ACTIVE_BTN : COLOR_BG);
  tft.drawRect(31, 34, 7, 7, up ? COLOR_WHITE : COLOR_DARKGREY);

  bool down = (mask & BTN_DOWN) != 0;
  tft.fillRect(31, 56, 7, 7, down ? COLOR_ACTIVE_BTN : COLOR_BG);
  tft.drawRect(31, 56, 7, 7, down ? COLOR_WHITE : COLOR_DARKGREY);

  bool left = (mask & BTN_LEFT) != 0;
  tft.fillRect(20, 45, 7, 7, left ? COLOR_ACTIVE_BTN : COLOR_BG);
  tft.drawRect(20, 45, 7, 7, left ? COLOR_WHITE : COLOR_DARKGREY);

  bool right = (mask & BTN_RIGHT) != 0;
  tft.fillRect(42, 45, 7, 7, right ? COLOR_ACTIVE_BTN : COLOR_BG);
  tft.drawRect(42, 45, 7, 7, right ? COLOR_WHITE : COLOR_DARKGREY);

  // ۲. کلیدهای اکشن پلی‌استیشن (سمت راست ماکت: مرکز حدود X=94, Y=48)
  bool tri = (mask & BTN_TRIANGLE) != 0;
  tft.fillRect(91, 34, 7, 7, tri ? PS_TRIANGLE_COLOR : COLOR_BG);
  tft.drawRect(91, 34, 7, 7, tri ? COLOR_WHITE : COLOR_DARKGREY);

  bool cross = (mask & BTN_CROSS) != 0;
  tft.fillRect(91, 56, 7, 7, cross ? PS_CROSS_COLOR : COLOR_BG);
  tft.drawRect(91, 56, 7, 7, cross ? COLOR_WHITE : COLOR_DARKGREY);

  bool sqr = (mask & BTN_SQUARE) != 0;
  tft.fillRect(80, 45, 7, 7, sqr ? PS_SQUARE_COLOR : COLOR_BG);
  tft.drawRect(80, 45, 7, 7, sqr ? COLOR_WHITE : COLOR_DARKGREY);

  bool cir = (mask & BTN_CIRCLE) != 0;
  tft.fillRect(102, 45, 7, 7, cir ? PS_CIRCLE_COLOR : COLOR_BG);
  tft.drawRect(102, 45, 7, 7, cir ? COLOR_WHITE : COLOR_DARKGREY);

  // ۳. کلیدهای گوشواره L1 و R1 (بالای کنترلر)
  bool l1 = (mask & BTN_L1) != 0;
  tft.fillRect(20, 24, 20, 4, l1 ? COLOR_WHITE : COLOR_DARKGREY);

  bool r1 = (mask & BTN_R1) != 0;
  tft.fillRect(88, 24, 20, 4, r1 ? COLOR_WHITE : COLOR_DARKGREY);

  // ۴. کلیدهای Select و Start (مرکز)
  bool sel = (mask & BTN_SELECT) != 0;
  tft.fillRect(52, 54, 8, 4, sel ? COLOR_ACTIVE_BTN : COLOR_DARKGREY);

  bool str = (mask & BTN_START) != 0;
  tft.fillRect(68, 54, 8, 4, str ? COLOR_ACTIVE_BTN : COLOR_DARKGREY);
}

// چاپ اطلاعات تله‌متری و نام کلیدهای فشرده شده در پایین صفحه
void drawTelemetryInfo() {
  tft.fillRect(0, 85, 128, 43, COLOR_BG);
  tft.drawFastHLine(0, 85, 128, COLOR_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_LIGHTGREY);

  // خط اول: فهرست متنی کلیدهای فعال
  tft.setCursor(4, 90);
  tft.print(F("KEYS: "));
  tft.setTextColor(COLOR_WHITE);

  if (activeButtons == 0) {
    tft.setTextColor(COLOR_LIGHTGREY);
    tft.print(F("[IDLE]"));
  } else {
    if (activeButtons & BTN_UP)       tft.print(F("^ "));
    if (activeButtons & BTN_DOWN)     tft.print(F("v "));
    if (activeButtons & BTN_LEFT)     tft.print(F("< "));
    if (activeButtons & BTN_RIGHT)    tft.print(F("> "));
    if (activeButtons & BTN_TRIANGLE) tft.print(F("T "));
    if (activeButtons & BTN_CIRCLE)   tft.print(F("O "));
    if (activeButtons & BTN_CROSS)    tft.print(F("X "));
    if (activeButtons & BTN_SQUARE)   tft.print(F("Sq "));
    if (activeButtons & BTN_L1)       tft.print(F("L1 "));
    if (activeButtons & BTN_R1)       tft.print(F("R1 "));
  }

  // خط دوم: شمارنده بسته‌ها
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(4, 103);
  tft.print(F("PACKETS: "));
  tft.setTextColor(COLOR_ACTIVE_BTN);
  tft.print(totalPacketsReceived);

  // خط سوم: فرکانس و کانال امن رادیو
  tft.setTextColor(COLOR_LIGHTGREY);
  tft.setCursor(4, 116);
  tft.print(F("CH115 | 2.515 GHz"));
}

// =================================================================================
// راه‌اندازی رادیو NRF24L01+ در مد گیرنده
// =================================================================================
void setupRadio() {
  Serial.println(F("[NRF24] Initializing Receiver on Pro Mini..."));

  if (!radio.begin()) {
    Serial.println(F("[ERROR] NRF24 Hardware failed to start!"));
    tft.fillRect(0, 50, 128, 30, COLOR_STATUS_ERR);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(8, 60);
    tft.print(F("NRF24 INIT FAIL!"));
    while (1) delay(1000);
  }

  radio.setChannel(RF_CHANNEL_NUM);                  // کانال 115 (2.515 GHz)
  radio.setDataRate(RF_250KBPS);                     // حداکثر برد در 250kbps
  radio.setPALevel(RF24_PA_HIGH);                    // توان بالا با آنتن خارجی
  radio.setCRCLength(RF24_CRC_16);                   // بررسی صحت سخت‌افزاری 16 بیتی
  radio.setAutoAck(true);                            // پاسخ‌دهی خودکار به فرستنده

  // باز کردن پایپ خواندن داده
  radio.openReadingPipe(1, RF_PIPE_ADDRESS);
  radio.startListening();                            // ورود به مد گوش‌به‌زنگ (RX Mode)

  Serial.println(F("[NRF24] Receiver ready on Pipe: PSG01"));
}

// =================================================================================
// تنظیمات اولیه سیستم (Setup)
// =================================================================================
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n========================================================"));
  Serial.println(F("  PlayStation Gamepad Receiver (Arduino Pro Mini 3.3V)  "));
  Serial.println(F("========================================================"));

  // راه‌اندازی نمایشگر ۱.۴۴ اینچ
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(0);
  tft.fillScreen(COLOR_BG);

  // راه‌اندازی رادیو
  setupRadio();

  // رسم ظاهر اولیه نمایشگر
  drawHeader();
  drawControllerSkeleton();
  updateGamepadVisualizer(0);
  drawTelemetryInfo();

  Serial.println(F("[OK] Listening for incoming gamepad packets..."));
}

// =================================================================================
// حلقه اصلی برنامه (Main Loop)
// =================================================================================
void loop() {
  bool newPacketReceived = false;

  // بررسی ورود داده روی خط رادیویی
  if (radio.available()) {
    GamepadPacket packet;
    radio.read(&packet, sizeof(packet));

    // اعتبارسنجی جادویی، شناسه شبکه و چک‌سام CRC-8
    if (validatePacket(packet)) {
      newPacketReceived = true;
      lastPacketTime = millis();
      isConnected = true;
      totalPacketsReceived++;
      lastPacketSeq = packet.sequence;
      activeButtons = packet.buttons;

      // چاپ لاگ سریال
      Serial.print(F("[RX #"));
      Serial.print(packet.sequence);
      Serial.print(F("] Mask: 0x"));
      Serial.println(packet.buttons, HEX);
    } else {
      Serial.println(F("[WARN] Corrupted packet rejected by CRC/Magic check."));
    }
  }

  // بررسی انقضای زمان دریافت (Timeout Fail-safe)
  if (millis() - lastPacketTime > LINK_TIMEOUT_MS) {
    if (isConnected) {
      isConnected = false;
      activeButtons = 0; // ریست ایمن دکمه‌ها در زمان قطعی اتصال
      Serial.println(F("[LINK] Connection lost (Timeout)."));
    }
  }

  // به‌روزرسانی رابط کاربری در صورت دریافت بسته یا تغییر وضعیت
  if (newPacketReceived) {
    updateHeaderStatus(true);
    updateGamepadVisualizer(activeButtons);
    drawTelemetryInfo();
    previousButtons = activeButtons;
  } else if (isConnected != wasConnected) {
    wasConnected = isConnected;
    updateHeaderStatus(false);
    updateGamepadVisualizer(activeButtons);
    drawTelemetryInfo();
  } else if (!isConnected && (activeButtons != previousButtons)) {
    updateGamepadVisualizer(0);
    drawTelemetryInfo();
    previousButtons = 0;
  }
}
