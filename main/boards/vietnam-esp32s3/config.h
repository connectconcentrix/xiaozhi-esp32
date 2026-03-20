/**
 * ============================================================
 *  CONFIG.H - Board Configuration cho ESP32-S3-N16R8
 *  Xiaozhi AI - Phiên bản Tiếng Việt
 * ============================================================
 * 
 * Phần cứng mặc định:
 *   - Chip:   ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM Octal)
 *   - Màn:   OLED SSD1306 0.96" (I2C)
 *   - Mic:   INMP441 (I2S)
 *   - Loa:   MAX98357A (I2S)
 * 
 * HƯỚNG DẪN ĐỔI CHÂN:
 *   Chỉ cần sửa các #define bên dưới để phù hợp với mạch của bạn.
 * ============================================================
 */

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// ============================================================
//  CẤU HÌNH ÂM THANH
// ============================================================

// Tốc độ mẫu âm thanh (không đổi)
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Dùng chế độ I2S Simplex (Mic và Loa dùng I2S riêng biệt)
// Bình luận dòng này nếu muốn dùng chế độ Duplex (chung 1 I2S)
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX

// --- CHÂN MICROPHONE I2S (INMP441 hoặc tương đương) ---
// WS  = Word Select / LRCK
// SCK = Bit Clock
// DIN = Data In (từ mic vào ESP)
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_6

// --- CHÂN LOA KHUẾCH ĐẠI I2S (MAX98357A hoặc tương đương) ---
// DOUT = Data Out (từ ESP ra loa)
// BCLK = Bit Clock
// LRCK = Left/Right Clock
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16

#else

// --- CHẾ ĐỘ DUPLEX (Mic và Loa chung I2S) ---
#define AUDIO_I2S_GPIO_WS    GPIO_NUM_4
#define AUDIO_I2S_GPIO_BCLK  GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN   GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT  GPIO_NUM_7

#endif // AUDIO_I2S_METHOD_SIMPLEX


// ============================================================
//  CHÂN NÚT NHẤN
// ============================================================

// Nút BOOT (GPIO0) - có sẵn trên mọi ESP32-S3 DevKit
// - Nhấn 1 lần khi đang standby: Bắt đầu/dừng hội thoại
// - Nhấn khi khởi động: Vào chế độ cấu hình WiFi
#define BOOT_BUTTON_GPIO        GPIO_NUM_0

// Nút cảm ứng (Touch) - giữ để nói, nhả để dừng
// Đổi chân này thành GPIO bạn nối nút của mình
#define TOUCH_BUTTON_GPIO       GPIO_NUM_47

// Nút tăng/giảm âm lượng (tùy chọn - có thể bỏ nếu không dùng)
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39


// ============================================================
//  ĐÈN LED TÍCH HỢP
// ============================================================

// GPIO của đèn LED tích hợp trên board
// ESP32-S3 DevKit thường dùng GPIO48
#define BUILTIN_LED_GPIO        GPIO_NUM_48


// ============================================================
//  CHÂN MÀN HÌNH OLED (I2C)
// ============================================================

// SDA = Data
// SCL = Clock
// Địa chỉ I2C mặc định của SSD1306 là 0x3C
#define DISPLAY_SDA_PIN  GPIO_NUM_41
#define DISPLAY_SCL_PIN  GPIO_NUM_42
#define DISPLAY_WIDTH    128

// Chọn loại OLED trong menuconfig:
//   CONFIG_OLED_SSD1306_128X32 → màn 0.96" (thường gặp)
//   CONFIG_OLED_SSD1306_128X64 → màn 1.3"
#if CONFIG_OLED_SSD1306_128X32
    #define DISPLAY_HEIGHT  32
#elif CONFIG_OLED_SSD1306_128X64
    #define DISPLAY_HEIGHT  64
#elif CONFIG_OLED_SH1106_128X64
    #define DISPLAY_HEIGHT  64
    #define SH1106
#else
    #error "Chưa chọn loại màn hình OLED! Vào menuconfig > Xiaozhi Assistant > OLED Type"
#endif

// Lật hình nếu màn hình của bạn bị ngược
#define DISPLAY_MIRROR_X  true
#define DISPLAY_MIRROR_Y  true


// ============================================================
//  TÍCH HỢP MỞ RỘNG (MCP Tools)
// ============================================================

// GPIO điều khiển relay/đèn (ví dụ: dùng lệnh "bật đèn")
// Đây là demo MCP Tool - có thể thêm nhiều thiết bị khác
#define LAMP_GPIO  GPIO_NUM_18

// ============================================================
//  THÊM CHÂN MỞ RỘNG CỦA BẠN Ở ĐÂY
// ============================================================

// Ví dụ: Cảm biến nhiệt độ DS18B20
// #define DS18B20_GPIO  GPIO_NUM_2

// Ví dụ: Buzzer
// #define BUZZER_GPIO  GPIO_NUM_3

// Ví dụ: Relay
// #define RELAY_GPIO   GPIO_NUM_8

// Ví dụ: LED RGB NeoPixel
// #define RGB_LED_GPIO  GPIO_NUM_21
// #define RGB_LED_COUNT 1


#endif // _BOARD_CONFIG_H_
