/**
 * ============================================================
 *  VIETNAM_ESP32S3_BOARD.CC
 *  Board chính - Xiaozhi AI Tiếng Việt
 *  Chip: ESP32-S3-N16R8
 * ============================================================
 *
 *  Cấu trúc mở rộng:
 *    - InitializeDisplayI2c()   → Khởi tạo bus I2C cho màn hình
 *    - InitializeSsd1306Display() → Khởi tạo màn OLED
 *    - InitializeButtons()      → Cấu hình nút nhấn
 *    - InitializeTools()        → Thêm thiết bị IoT / MCP Tools
 *    - GetAudioCodec()          → Trả về codec âm thanh I2S
 *
 *  ĐỂ THÊM TÍCH HỢP MỚI: xem phần InitializeTools() bên dưới
 * ============================================================
 */

#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/oled_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "lamp_controller.h"
#include "led/single_led.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

// Nếu dùng màn hình SH1106 thay vì SSD1306
#ifdef SH1106
#include <esp_lcd_panel_sh1106.h>
#endif

#define TAG "VietnamESP32S3Board"

// ============================================================
//  LỚP BOARD CHÍNH
// ============================================================

class VietnamESP32S3Board : public WifiBoard {
private:
    // --- Tài nguyên màn hình ---
    i2c_master_bus_handle_t display_i2c_bus_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;

    // --- Nút nhấn ---
    Button boot_button_;
    Button touch_button_;
    Button volume_up_button_;
    Button volume_down_button_;

    // ============================================================
    //  KHỞI TẠO BUS I2C CHO MÀN HÌNH
    // ============================================================
    void InitializeDisplayI2c() {
        i2c_master_bus_config_t bus_config = {
            .i2c_port          = (i2c_port_t)0,
            .sda_io_num        = DISPLAY_SDA_PIN,
            .scl_io_num        = DISPLAY_SCL_PIN,
            .clk_source        = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority     = 0,
            .trans_queue_depth = 0,
            .flags             = { .enable_internal_pullup = 1 },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &display_i2c_bus_));
        ESP_LOGI(TAG, "I2C bus khởi tạo thành công (SDA=%d, SCL=%d)",
                 DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);
    }

    // ============================================================
    //  KHỞI TẠO MÀN HÌNH OLED SSD1306 / SH1106
    // ============================================================
    void InitializeSsd1306Display() {
        esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr             = 0x3C,   // Địa chỉ I2C mặc định SSD1306
            .on_color_trans_done  = nullptr,
            .user_ctx             = nullptr,
            .control_phase_bytes  = 1,
            .dc_bit_offset        = 6,
            .lcd_cmd_bits         = 8,
            .lcd_param_bits       = 8,
            .flags                = { .dc_low_on_data = 0, .disable_control_phase = 0 },
            .scl_speed_hz         = 400 * 1000,  // 400kHz Fast Mode
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v2(display_i2c_bus_, &io_config, &panel_io_));

        ESP_LOGI(TAG, "Đang cài driver SSD1306 (%dx%d)...", DISPLAY_WIDTH, DISPLAY_HEIGHT);

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = -1;
        panel_config.bits_per_pixel = 1;

        esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
        };
        panel_config.vendor_config = &ssd1306_config;

#ifdef SH1106
        ESP_ERROR_CHECK(esp_lcd_new_panel_sh1106(panel_io_, &panel_config, &panel_));
        ESP_LOGI(TAG, "Dùng driver SH1106");
#else
        ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(panel_io_, &panel_config, &panel_));
        ESP_LOGI(TAG, "Dùng driver SSD1306");
#endif

        // Reset và khởi tạo panel
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        if (esp_lcd_panel_init(panel_) != ESP_OK) {
            ESP_LOGE(TAG, "Khởi tạo màn hình thất bại! Kiểm tra dây nối SDA/SCL.");
            display_ = new NoDisplay();
            return;
        }

        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, false));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        display_ = new OledDisplay(panel_io_, panel_,
                                   DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                   DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        ESP_LOGI(TAG, "Màn hình OLED đã sẵn sàng!");
    }

    // ============================================================
    //  CẤU HÌNH NÚT NHẤN
    // ============================================================
    void InitializeButtons() {
        // --- Nút BOOT (GPIO0) ---
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                // Nhấn khi đang khởi động → Vào chế độ cấu hình WiFi
                EnterWifiConfigMode();
                return;
            }
            // Nhấn khi đang hoạt động → Bắt đầu/Dừng hội thoại
            app.ToggleChatState();
        });

        // --- Nút Cảm Ứng (Touch) ---
        // Giữ để nói, nhả để dừng (Push-to-Talk)
        touch_button_.OnPressDown([this]() {
            Application::GetInstance().StartListening();
        });
        touch_button_.OnPressUp([this]() {
            Application::GetInstance().StopListening();
        });

        // --- Nút Tăng Âm Lượng ---
        volume_up_button_.OnClick([this]() {
            auto codec  = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) volume = 100;
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(
                Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_up_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(100);
            GetDisplay()->ShowNotification(Lang::Strings::MAX_VOLUME);
        });

        // --- Nút Giảm Âm Lượng ---
        volume_down_button_.OnClick([this]() {
            auto codec  = GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) volume = 0;
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(
                Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_down_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(0);
            GetDisplay()->ShowNotification(Lang::Strings::MUTED);
        });
    }

    // ============================================================
    //  KHỞI TẠO CÔNG CỤ IoT / MCP TOOLS
    //
    //  ĐÂY LÀ NƠI BẠN THÊM TÍCH HỢP CỦA MÌNH:
    //
    //  Ví dụ 1 - Đèn/Relay (đã có sẵn):
    //    static LampController lamp(LAMP_GPIO);
    //
    //  Ví dụ 2 - Thêm sensor nhiệt độ:
    //    static TemperatureSensor sensor(DS18B20_GPIO);
    //
    //  Ví dụ 3 - Thêm relay điều khiển quạt:
    //    static RelayController fan(RELAY_GPIO, "fan", "quạt");
    //
    //  Ví dụ 4 - Thêm servo:
    //    static ServoController servo(SERVO_GPIO);
    //
    //  Xem thêm mcp_server.h để biết cách tạo MCP Tool tùy chỉnh.
    // ============================================================
    void InitializeTools() {
        // --- Đèn / Relay demo (bật/tắt bằng lệnh giọng nói) ---
        static LampController lamp(LAMP_GPIO);

        // =========================================================
        //  THÊM THIẾT BỊ CỦA BẠN Ở ĐÂY
        //  (bỏ comment và sửa theo phần cứng của bạn)
        // =========================================================

        // static LampController relay2(GPIO_NUM_8);      // Relay 2
        // static LampController fan_ctrl(GPIO_NUM_3);    // Quạt
    }

public:
    // ============================================================
    //  HÀM KHỞI TẠO BOARD
    // ============================================================
    VietnamESP32S3Board() :
        boot_button_(BOOT_BUTTON_GPIO),
        touch_button_(TOUCH_BUTTON_GPIO),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO)
    {
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, " Xiaozhi AI - Phiên bản Tiếng Việt");
        ESP_LOGI(TAG, " Chip: ESP32-S3-N16R8");
        ESP_LOGI(TAG, "========================================");

        InitializeDisplayI2c();
        InitializeSsd1306Display();
        InitializeButtons();
        InitializeTools();
    }

    // ============================================================
    //  TRẢ VỀ ĐỐI TƯỢNG ĐÈN LED
    // ============================================================
    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    // ============================================================
    //  TRẢ VỀ CODEC ÂM THANH I2S
    // ============================================================
    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        // Simplex: Mic và Loa dùng I2S riêng biệt (chất lượng tốt hơn)
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS,
            AUDIO_I2S_MIC_GPIO_DIN
        );
#else
        // Duplex: Mic và Loa chung I2S
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN
        );
#endif
        return &audio_codec;
    }

    // ============================================================
    //  TRẢ VỀ ĐỐI TƯỢNG MÀN HÌNH
    // ============================================================
    virtual Display* GetDisplay() override {
        return display_;
    }
};

// Đăng ký board với hệ thống Xiaozhi
DECLARE_BOARD(VietnamESP32S3Board);
