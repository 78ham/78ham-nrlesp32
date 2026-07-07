#include "display.h"

#include "board_pins.h"
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <string.h>

static const char *TAG = "LCD";
static esp_lcd_panel_handle_t s_panel = nullptr;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_BLUE = 0x001F;
static constexpr uint16_t COLOR_YELLOW = 0xFFE0;

void display_init(void)
{
    spi_bus_config_t bus = {};
    bus.sclk_io_num = PIN_TFT_SCLK;
    bus.mosi_io_num = PIN_TFT_MOSI;
    bus.miso_io_num = -1;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 240 * 40 * 2;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io = nullptr;
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.dc_gpio_num = static_cast<gpio_num_t>(PIN_TFT_DC);
    io_cfg.cs_gpio_num = static_cast<gpio_num_t>(PIN_TFT_CS);
    io_cfg.pclk_hz = 40 * 1000 * 1000;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.lcd_param_bits = 8;
    io_cfg.spi_mode = 0;
    io_cfg.trans_queue_depth = 10;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io));

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = static_cast<gpio_num_t>(PIN_TFT_RST);
    panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_cfg.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
    ESP_LOGI(TAG, "ST7789V init ok");
}

static void fill(uint16_t color)
{
    static uint16_t line[240 * 20];
    for (size_t i = 0; i < sizeof(line) / sizeof(line[0]); ++i) {
        line[i] = color;
    }
    for (int y = 0; y < 320; y += 20) {
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, 240, y + 20, line);
    }
}

static const uint8_t *glyph(char c)
{
    static const uint8_t space[7] = {0, 0, 0, 0, 0, 0, 0};
    static const uint8_t unknown[7] = {0x1F, 0x11, 0x04, 0x04, 0, 0x04, 0};
    static const uint8_t dash[7] = {0, 0, 0, 0x1F, 0, 0, 0};
    static const uint8_t dot[7] = {0, 0, 0, 0, 0, 0x0C, 0x0C};
    static const uint8_t colon[7] = {0, 0x0C, 0x0C, 0, 0x0C, 0x0C, 0};
    static const uint8_t zero[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
    static const uint8_t one[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
    static const uint8_t two[7] = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
    static const uint8_t three[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
    static const uint8_t four[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
    static const uint8_t five[7] = {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
    static const uint8_t six[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
    static const uint8_t seven[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
    static const uint8_t eight[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
    static const uint8_t nine[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};
    static const uint8_t A[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    static const uint8_t B[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
    static const uint8_t C[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
    static const uint8_t D[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
    static const uint8_t E[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
    static const uint8_t F[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
    static const uint8_t G[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E};
    static const uint8_t H[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    static const uint8_t I[7] = {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
    static const uint8_t J[7] = {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
    static const uint8_t K[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    static const uint8_t L[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
    static const uint8_t M[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
    static const uint8_t N[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
    static const uint8_t O[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    static const uint8_t P[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
    static const uint8_t Q[7] = {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
    static const uint8_t R[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
    static const uint8_t S[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
    static const uint8_t T[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    static const uint8_t U[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    static const uint8_t V[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
    static const uint8_t W[7] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
    static const uint8_t X[7] = {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
    static const uint8_t Y[7] = {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
    static const uint8_t Z[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};

    if (c >= 'a' && c <= 'z') {
        c -= 32;
    }
    switch (c) {
    case ' ': return space;
    case '-': return dash;
    case '.': return dot;
    case ':': return colon;
    case '0': return zero;
    case '1': return one;
    case '2': return two;
    case '3': return three;
    case '4': return four;
    case '5': return five;
    case '6': return six;
    case '7': return seven;
    case '8': return eight;
    case '9': return nine;
    case 'A': return A;
    case 'B': return B;
    case 'C': return C;
    case 'D': return D;
    case 'E': return E;
    case 'F': return F;
    case 'G': return G;
    case 'H': return H;
    case 'I': return I;
    case 'J': return J;
    case 'K': return K;
    case 'L': return L;
    case 'M': return M;
    case 'N': return N;
    case 'O': return O;
    case 'P': return P;
    case 'Q': return Q;
    case 'R': return R;
    case 'S': return S;
    case 'T': return T;
    case 'U': return U;
    case 'V': return V;
    case 'W': return W;
    case 'X': return X;
    case 'Y': return Y;
    case 'Z': return Z;
    default: return unknown;
    }
}

static void draw_char(int x, int y, char c, uint16_t color, int scale)
{
    const uint8_t *rows = glyph(c);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if ((rows[row] & (1 << (4 - col))) != 0) {
                uint16_t block[4 * 4];
                for (size_t i = 0; i < sizeof(block) / sizeof(block[0]); ++i) {
                    block[i] = color;
                }
                esp_lcd_panel_draw_bitmap(s_panel, x + col * scale, y + row * scale, x + (col + 1) * scale, y + (row + 1) * scale, block);
            }
        }
    }
}

static void draw_text(int x, int y, const char *text, uint16_t color, int scale)
{
    while (*text != '\0') {
        draw_char(x, y, *text, color, scale);
        x += 6 * scale;
        ++text;
    }
}

void display_show_boot(const AppConfig *config)
{
    (void)config;
    if (s_panel) {
        fill(COLOR_BLACK);
        draw_text(12, 24, "78HAM NRL", COLOR_CYAN, 3);
        draw_text(12, 70, "BOOT", COLOR_WHITE, 2);
    }
}

void display_show_config_ap(const AppConfig *config)
{
    if (s_panel) {
        fill(COLOR_BLUE);
        draw_text(12, 16, "78HAM ESP32", COLOR_WHITE, 2);
        draw_text(12, 46, "CONFIG AP", COLOR_YELLOW, 2);
        draw_text(12, 88, "SSID", COLOR_CYAN, 2);
        draw_text(12, 112, config->ap_ssid, COLOR_WHITE, 2);
        draw_text(12, 154, "PASS", COLOR_CYAN, 2);
        draw_text(12, 178, config->ap_password, COLOR_WHITE, 2);
        draw_text(12, 234, "OPEN", COLOR_CYAN, 2);
        draw_text(12, 258, "192.168.4.1", COLOR_WHITE, 2);
    }
}
