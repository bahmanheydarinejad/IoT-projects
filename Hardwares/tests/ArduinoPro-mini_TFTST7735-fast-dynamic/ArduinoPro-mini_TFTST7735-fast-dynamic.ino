/**********************************************************************************
 * پروژه تست رفرش‌ریت: انیمیشن مکعب ۳ بعدی نئونی با شمارنده زنده FPS
 * برد پردازنده: Arduino Pro Mini 3.3V (8MHz)
 * ماژول نمایشگر: 1.44" TFT SPI با چیپ درایور ST7735 (128x128 Pixels)
 * کتابخانه‌های مورد نیاز: Adafruit_GFX و Adafruit_ST7735
 **********************************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ۱. تنظیمات پایه‌های اتصال به برد آردویینو پرومینی
#define TFT_CS    10  // Chip Select
#define TFT_DC     9  // Data / Command (A0)
#define TFT_RST    8  // Reset
#define TFT_LED    6  // نور پس‌زمینه (یا اتصال مستقیم با مقاومت به 3.3V)

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ۲. جدول رنگ‌های نئونی ۱۶ بیتی (RGB565)
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_YELLOW      0xFFE0
#define COLOR_ORANGE      0xFD20
#define COLOR_GREENYELLOW 0xAFE5
#define COLOR_PINK        0xFC18
#define COLOR_PURPLE      0x8010
#define COLOR_HUD_BG      0x10A2

// ۱۲ رنگ مجزا برای یال‌های مکعب
const uint16_t edgeColors[12] = {
  COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW,
  COLOR_CYAN, COLOR_MAGENTA, COLOR_ORANGE, COLOR_GREENYELLOW,
  COLOR_PINK, COLOR_WHITE, COLOR_CYAN, COLOR_YELLOW
};

// ۳. مدل هندسی مکعب سه‌بعدی (۸ رأس) - ابعاد تنظیم شده جهت عدم تداخل با نوار بالای صفحه
float vertices[8][3] = {
  {-20, -20, -20}, { 20, -20, -20}, { 20,  20, -20}, {-20,  20, -20},
  {-20, -20,  20}, { 20, -20,  20}, { 20,  20,  20}, {-20,  20,  20}
};

// ۱۲ یال اتصال‌دهنده رأس‌ها
const uint8_t edges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0}, // وجه جلو
  {4, 5}, {5, 6}, {6, 7}, {7, 4}, // وجه عقب
  {0, 4}, {1, 5}, {2, 6}, {3, 7}  // خطوط رابط
};

// مختصات دوبعدی رأس‌ها روی صفحه (برای فریم جاری و فریم قبلی جهت پاکسازی)
int current2D[8][2];
int prev2D[8][2];

// زوایای چرخش مکعب
float angleX = 0;
float angleY = 0;
float angleZ = 0;

// متغیرهای سنجش دقیق FPS
unsigned long frameCount = 0;
unsigned long lastFpsUpdate = 0;
int liveFps = 0;
unsigned long frameStartTime = 0;
int frameDurationMs = 0;

void setup() {
  Serial.begin(115200);

  // فعال‌سازی نور پس‌زمینه
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  // راه‌اندازی ال‌سی‌دی 1.44 اینچ (تب سبز)
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(1); // حالت افقی
  tft.fillScreen(COLOR_BLACK);

  // ترسیم قاب ثابت بالای صفحه (HUD برای نمایش FPS)
  tft.fillRect(0, 0, 128, 16, COLOR_HUD_BG);
  tft.drawFastHLine(0, 16, 128, COLOR_CYAN);
  
  tft.setTextSize(1);
  tft.setTextColor(COLOR_YELLOW, COLOR_HUD_BG);
  tft.setCursor(4, 4);
  tft.print("FPS: --");

  tft.setTextColor(COLOR_WHITE, COLOR_HUD_BG);
  tft.setCursor(68, 4);
  tft.print("PRO MINI");

  // محاسبه اولیه مختصات
  for (int i = 0; i < 8; i++) {
    prev2D[i][0] = 64;
    prev2D[i][1] = 72;
  }
}

void loop() {
  frameStartTime = millis();

  // الف) پاک کردن یال‌ها و رأس‌های فریم قبلی (رسم با رنگ مشکی - بدون نیاز به رفرش کل صفحه)
  for (int i = 0; i < 12; i++) {
    int p1 = edges[i][0];
    int p2 = edges[i][1];
    tft.drawLine(prev2D[p1][0], prev2D[p1][1], prev2D[p2][0], prev2D[p2][1], COLOR_BLACK);
  }
  for (int i = 0; i < 8; i++) {
    tft.drawPixel(prev2D[i][0], prev2D[i][1], COLOR_BLACK);
  }

  // ب) محاسبات ریاضی دوران سه‌بعدی و نگاشت روی صفحه نمایش
  float radX = angleX * 0.01745329;
  float radY = angleY * 0.01745329;
  float radZ = angleZ * 0.01745329;

  float sinX = sin(radX), cosX = cos(radX);
  float sinY = sin(radY), cosY = cos(radY);
  float sinZ = sin(radZ), cosZ = cos(radZ);

  for (int i = 0; i < 8; i++) {
    // دوران حول محور X
    float y1 = vertices[i][1] * cosX - vertices[i][2] * sinX;
    float z1 = vertices[i][1] * sinX + vertices[i][2] * cosX;

    // دوران حول محور Y
    float x2 = vertices[i][0] * cosY + z1 * sinY;
    float z2 = -vertices[i][0] * sinY + z1 * cosY;

    // دوران حول محور Z
    float x3 = x2 * cosZ - y1 * sinZ;
    float y3 = x2 * sinZ + y1 * cosZ;

    // پرسپکتیو و تبدیل به مختصات صفحه (مرکز صفحه: X=64, Y=72)
    float distance = 95.0;
    float scale = distance / (distance + z2);
    current2D[i][0] = 64 + (int)(x3 * scale);
    current2D[i][1] = 72 + (int)(y3 * scale);
  }

  // ج) رسم یال‌های فریم جدید با ۱۲ رنگ مختلف نئونی
  for (int i = 0; i < 12; i++) {
    int p1 = edges[i][0];
    int p2 = edges[i][1];
    tft.drawLine(current2D[p1][0], current2D[p1][1], current2D[p2][0], current2D[p2][1], edgeColors[i]);
  }

  // د) رسم رأس‌های درخشان مکعب
  for (int i = 0; i < 8; i++) {
    tft.drawPixel(current2D[i][0], current2D[i][1], COLOR_WHITE);
  }

  // ذخیره مختصات جاری برای فریم بعدی
  for (int i = 0; i < 8; i++) {
    prev2D[i][0] = current2D[i][0];
    prev2D[i][1] = current2D[i][1];
  }

  // افزایش زاویه دوران
  angleX += 3.5;
  angleY += 4.5;
  angleZ += 2.0;
  if (angleX >= 360) angleX -= 360;
  if (angleY >= 360) angleY -= 360;
  if (angleZ >= 360) angleZ -= 360;

  // محاسبه زمان رندر این فریم
  frameDurationMs = millis() - frameStartTime;

  // هـ) سنجش و به‌روزرسانی نرخ فریم (FPS) هر ۱ ثانیه
  frameCount++;
  if (millis() - lastFpsUpdate >= 1000) {
    liveFps = frameCount;
    frameCount = 0;
    lastFpsUpdate = millis();

    // چاپ نرخ فریم بدون پرش روی نوار بالا
    tft.setTextSize(1);
    tft.setTextColor(COLOR_YELLOW, COLOR_HUD_BG);
    tft.setCursor(4, 4);
    tft.print("FPS: ");
    if (liveFps < 10) tft.print(" ");
    tft.print(liveFps);

    // چاپ زمان رندر بر حسب میلی‌ثانیه
    tft.setTextColor(COLOR_CYAN, COLOR_HUD_BG);
    tft.setCursor(76, 4);
    if (frameDurationMs < 10) tft.print(" ");
    tft.print(frameDurationMs);
    tft.print("ms ");
  }
}
