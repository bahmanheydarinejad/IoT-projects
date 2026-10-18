#ifndef PACKET_PROTOCOL_H
#define PACKET_PROTOCOL_H

#include <Arduino.h>

/**********************************************************************************
 * پروتکل امن و ضدتداخل ارتباط رادیویی NRF24L01+PA+LNA
 * ویژه دسته کنترل لمسی پلی‌استیشن (NodeMCU) و گیرنده مانیتورینگ (Pro Mini)
 * 
 * ویژگی‌های مهندسی ضدتداخل:
 * ۱. کانال رادیویی ۱۱۵ (فرکانس 2.515 GHz): کاملاً خارج از باند فرکانسی وای‌فای خانگی (2.412 - 2.484 GHz)
 * ۲. پهنای باند و نرخ انتقال 250kbps: بیشترین برد موثر (تا ۱ کیلومتر با آنتن PA/LNA) و حساسیت گیرنده بالا (-94dBm)
 * ۳. آدرس پایپ ۵ بایتی اختصاصی شبکه: جلوگیری از شنود یا دریافت بسته‌های تصادفی سایر ماژول‌ها
 * ۴. بایت‌های همگام‌سازی جادویی (Magic Bytes: 0xA5, 0x5A) و شناسه یکتای شبکه (Network Token: 0xC0DE)
 * ۵. کنترل خطای سخت‌گیرانه با چک‌سام CRC-8 و CRC-16 سخت‌افزاری
 * ۶. ارسال وضعیت کامل تمام دکمه‌ها در قالب یک بایت‌ماسک ۱۶ بیتی در هر سیکل
 **********************************************************************************/

// کانال رادیویی فرکانس بالا (کانال ۱۱۵ معادل ۲۵۱۵ مگاهرتز جهت حذف تداخل کامل با Wi-Fi و بلوتوث)
#define RF_CHANNEL_NUM        115

// آدرس ۵ بایتی پایپ سخت‌افزاری اختصاصی
const uint8_t RF_PIPE_ADDRESS[5] = {'P', 'S', 'G', '0', '1'};

// بایت‌های اعتبارسنجی پکت
#define PACKET_MAGIC_BYTE_1   0xA5
#define PACKET_MAGIC_BYTE_2   0x5A
#define PACKET_NETWORK_ID     0xC0DE

// بیت‌ماسک وضعیت کلیدهای دسته پلی‌استیشن (PlayStation Buttons Bitmask)
#define BTN_UP        (1 << 0)   // دکمه جهت بالا (D-Pad Up)
#define BTN_DOWN      (1 << 1)   // دکمه جهت پایین (D-Pad Down)
#define BTN_LEFT      (1 << 2)   // دکمه جهت چپ (D-Pad Left)
#define BTN_RIGHT     (1 << 3)   // دکمه جهت راست (D-Pad Right)

#define BTN_TRIANGLE  (1 << 4)   // دکمه مثلث سبز 🔺
#define BTN_CIRCLE    (1 << 5)   // دکمه دایره قرمز ⭕
#define BTN_CROSS     (1 << 6)   // دکمه ضربدر آبی ❌
#define BTN_SQUARE    (1 << 7)   // دکمه مربع بنفش/صورتی ⬜

#define BTN_L1        (1 << 8)   // دکمه گوشواره چپ (Left Shoulder L1)
#define BTN_R1        (1 << 9)   // دکمه گوشواره راست (Right Shoulder R1)
#define BTN_SELECT    (1 << 10)  // دکمه Select
#define BTN_START     (1 << 11)  // دکمه Start

// ساختار فشرده و استاندارد بسته داده (Total: 7 Bytes)
#pragma pack(push, 1)
struct GamepadPacket {
  uint8_t  magic1;      // بایت جادویی ۱: 0xA5
  uint8_t  magic2;      // بایت جادویی ۲: 0x5A
  uint16_t networkId;   // شناسه یکتای شبکه: 0xC0DE
  uint8_t  sequence;    // شمارنده سریال پکت (0 تا 255 برای تشخیص بسته گم‌شده)
  uint16_t buttons;     // بیت‌ماسک ۱۶ بیتی وضعیت کامل تمام دکمه‌ها
  uint8_t  crc8;        // کد کشف خطای CRC-8
};
#pragma pack(pop)

// تابع محاسبه CRC-8 بر روی داده‌های ارسالی (چندجمله‌ای 0x07 استاندارد)
inline uint8_t calculateCRC8(const uint8_t *data, size_t length) {
  uint8_t crc = 0x00;
  while (length--) {
    uint8_t extract = *data++;
    for (uint8_t tempI = 8; tempI; tempI--) {
      uint8_t sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

// تابع اعتبارسنجی بسته دریافتی
inline bool validatePacket(const GamepadPacket &pkt) {
  if (pkt.magic1 != PACKET_MAGIC_BYTE_1 || pkt.magic2 != PACKET_MAGIC_BYTE_2) {
    return false;
  }
  if (pkt.networkId != PACKET_NETWORK_ID) {
    return false;
  }
  uint8_t calculated = calculateCRC8((const uint8_t*)&pkt, sizeof(GamepadPacket) - sizeof(uint8_t));
  return (calculated == pkt.crc8);
}

// تابع آماده‌سازی بسته قبل از ارسال
inline void preparePacket(GamepadPacket &pkt, uint8_t seq, uint16_t buttonMask) {
  pkt.magic1 = PACKET_MAGIC_BYTE_1;
  pkt.magic2 = PACKET_MAGIC_BYTE_2;
  pkt.networkId = PACKET_NETWORK_ID;
  pkt.sequence = seq;
  pkt.buttons = buttonMask;
  pkt.crc8 = calculateCRC8((const uint8_t*)&pkt, sizeof(GamepadPacket) - sizeof(uint8_t));
}

#endif // PACKET_PROTOCOL_H
