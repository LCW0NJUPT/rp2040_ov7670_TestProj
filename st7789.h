#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// 显示屏参数
#define LCD_WIDTH   240     // 水平像素数
#define LCD_HEIGHT  320     // 垂直像素数

// 列/行起始偏移
#define X_OFFSET    0       // 列起始偏移
#define Y_OFFSET    0       // 行起始偏移

// 引脚定义
#define PIN_MISO    -1      // 不使用 MISO，设为 -1
#define PIN_CS      5
#define PIN_SCK     18
#define PIN_MOSI    19
#define PIN_DC      2
#define PIN_RST     20
#define PIN_BL      22

// SPI 实例
extern spi_inst_t *spi;

// 函数声明
void st7789_init(void);
void st7789_write_color(uint16_t color, uint32_t count);
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void st7789_fill_color(uint16_t color);
void st7789_draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *image);

#endif // ST7789_H