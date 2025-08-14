#include "st7789.h"
#include "pin_definitions.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

// SPI 实例
spi_inst_t *spi = spi0;

// 简单的延时宏
#define delay_ms(ms) sleep_ms(ms)

// 低层 SPI / GPIO 操作函数
static inline void cs_select()   { gpio_put(PIN_CS, 0); }
static inline void cs_deselect() { gpio_put(PIN_CS, 1); }

static inline void dc_command()  { gpio_put(PIN_DC, 0); }
static inline void dc_data()     { gpio_put(PIN_DC, 1); }

static void spi_write(uint8_t data)
{
    // 只写 1 字节（阻塞）
    spi_write_blocking(spi, &data, 1);
}

/* 发送命令（先拉低 DC，再写命令字节） */
static void st7789_write_cmd(uint8_t cmd)
{
    cs_select();
    dc_command();
    spi_write(cmd);
    cs_deselect();
}

/* 发送数据（先拉高 DC，再写数据） */
static void st7789_write_data(const uint8_t *buf, size_t len)
{
    if (len == 0) return;
    cs_select();
    dc_data();
    spi_write_blocking(spi, buf, len);
    cs_deselect();
}

/* 简化版：发送单个字节数据 */
static void st7789_write_data_byte(uint8_t data)
{
    cs_select();
    dc_data();
    spi_write(data);
    cs_deselect();
}

/* ---------------------- 复位 ---------------------- */
static void st7789_reset(void)
{
    gpio_put(PIN_RST, 0);
    delay_ms(100);
    gpio_put(PIN_RST, 1);
    delay_ms(100);
}

/* ----------------- 发送颜色数据（RGB565） ----------------- */
void st7789_write_color(uint16_t color, uint32_t count)
{
    // 颜色数据是 16bit（高字节在前），一次性发送 2*count 字节
    uint8_t buf[64];
    // 将 color 按 big‑endian 拆成 2 字节
    uint8_t hi = color >> 8;  // 先发送高字节
    uint8_t lo = color & 0xFF;    // 后发送低字节

    // 填充缓冲区
    for (int i = 0; i < sizeof(buf); i += 2) {
        buf[i]     = hi;
        buf[i + 1] = lo;
    }

    cs_select();
    dc_data();
    while (count) {
        uint32_t chunk = (count > (sizeof(buf) / 2)) ? (sizeof(buf) / 2) : count;
        spi_write_blocking(spi, buf, chunk * 2);
        count -= chunk;
    }
    cs_deselect();
}

/* ----------------- 设置显示窗口（列/行） ----------------- */
void st7789_set_window(uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1)
{
    // 加上偏移
    x0 += X_OFFSET;
    x1 += X_OFFSET;
    y0 += Y_OFFSET;
    y1 += Y_OFFSET;

    // Column address set (0x2A)
    st7789_write_cmd(0x2A);
    uint8_t col[4] = {
        (x0 >> 8) & 0xFF, x0 & 0xFF,
        (x1 >> 8) & 0xFF, x1 & 0xFF
    };
    st7789_write_data(col, 4);

    // Row address set (0x2B)
    st7789_write_cmd(0x2B);
    uint8_t row[4] = {
        (y0 >> 8) & 0xFF, y0 & 0xFF,
        (y1 >> 8) & 0xFF, y1 & 0xFF
    };
    st7789_write_data(row, 4);

    // Write to RAM
    st7789_write_cmd(0x2C);
}

/* ----------------- 设置旋转/颜色顺序 ----------------- */
// MADCTL 0x36
// bit7: MY (row address order)
// bit6: MX (column address order)
// bit5: MV (row/col exchange)
// bit3: BGR (0=RGB, 1=BGR)
static void st7789_set_madctl(void)
{
    // 常用方向设置：
    // 0x00: 正常方向
    // 0xC0: 180度旋转
    // 0xA0: 90度旋转
    // 0x60: 270度旋转
    uint8_t madctl = 0x60;           

    // 若需要镜像（比如屏幕被装反），可以 OR 上 0x80 (MY) 或 0x40 (MX)
    // 例如：madctl |= 0x80;   // 行反向
    st7789_write_cmd(0x36);
    st7789_write_data_byte(madctl);
}

/* ----------------- 初始化 ST7789 ----------------- */
void st7789_init(void)
{
    // 初始化ST7789显示屏控制引脚
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); // CS idle high

    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_put(PIN_DC, 0);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1); // 先不复位

    gpio_init(PIN_BL);
    gpio_set_dir(PIN_BL, GPIO_OUT);
    gpio_put(PIN_BL, 1); // 打开背光（高电平）

    // 初始化 SPI
    spi_init(spi, 50 * 1000 * 1000); // 50 MHz
    spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // 将 SCK、MOSI 复用为 SPI 功能
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    st7789_reset();

    // 1. Software reset
    st7789_write_cmd(0x01);
    delay_ms(150);

    // 2. Sleep out
    st7789_write_cmd(0x11);
    delay_ms(120);

    // 3. Set pixel format to 16‑bit (RGB565)
    st7789_write_cmd(0x3A);
    st7789_write_data_byte(0x55);
    delay_ms(10);

    // 4. Set MADCTL (旋转 + 颜色顺序)
    st7789_set_madctl();

    // 5. (可选) 颜色反转，如果需要 
    st7789_write_cmd(0x21); 
    st7789_write_cmd(0x13); // 标准伽马曲线以改善色彩质量

    // 6. 打开显示
    st7789_write_cmd(0x29);   // DISPON
    delay_ms(10);
}

/* ----------------- 填充颜色 ----------------- */
void st7789_fill_color(uint16_t color)
{
    // 全屏窗口
    st7789_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    // 写入 color，数量 = 像素数
    st7789_write_color(color, LCD_WIDTH * (uint32_t)LCD_HEIGHT);
}

/* ----------------- 绘制图像 ----------------- */
void st7789_draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *image)
{
    st7789_set_window(x, y, x + width - 1, y + height - 1);
    
    cs_select();
    dc_data();
    
    // 直接发送图像数据，使用uint16_t指针避免手动字节交换
    // RP2040是小端系统，而ST7789需要大端格式的RGB565数据
    // 通过逐个发送16位值并交换字节来实现正确格式
    const uint16_t *pixels = image;
    uint32_t total_pixels = (uint32_t)width * height;
    
    // 使用较小的缓冲区逐批处理，减少函数调用次数
    uint16_t buffer[64];  // 一次处理64个像素
    uint32_t pixels_sent = 0;
    
    while (pixels_sent < total_pixels) {
        uint32_t chunk_size = total_pixels - pixels_sent;
        if (chunk_size > 64) {
            chunk_size = 64;
        }
        
        // 复制并交换字节序
        for (uint32_t i = 0; i < chunk_size; i++) {
            uint16_t pixel = pixels[pixels_sent + i];
            // 交换字节序：将little-endian转换为big-endian
            buffer[i] = ((pixel & 0xFF) << 8) | (pixel >> 8);
        }
        
        // 发送这一批数据
        spi_write_blocking(spi, (const uint8_t*)buffer, chunk_size * 2);
        pixels_sent += chunk_size;
    }
    
    cs_deselect();
}