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

void display_show_boot(const AppConfig *config)
{
    (void)config;
    if (s_panel) {
        fill(0x0000);
    }
}

void display_show_config_ap(const AppConfig *config)
{
    (void)config;
    if (s_panel) {
        fill(0x001F);
    }
}
