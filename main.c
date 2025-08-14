#include <stdio.h>
#include "ov7670.h"
#include "st7789.h"
#include "pin_definitions.h"
#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

uint8_t image_buf[320*240*2];

// 处理并显示捕获的图像帧
void process_and_display_frame(struct ov7670_config *config) {
    // 检查图像是否几乎全黑（镜头盖合上时）
    // 计算图像亮度平均值
    uint16_t *pixels = (uint16_t*)config->image_buf;
    uint32_t black_pixels = 0;
    const uint32_t total_pixels = 320 * 240;
    
    // 采样部分像素来判断是否为黑屏
    for (uint32_t i = 0; i < total_pixels; i += 200) { // 每200个像素采样一次以提高性能
        uint16_t pixel = pixels[i];
        // 提取RGB分量计算亮度 (RGB565)
        uint8_t r = (pixel >> 11) & 0x1F;
        uint8_t g = (pixel >> 5) & 0x3F;
        uint8_t b = pixel & 0x1F;
        
        // 简单亮度计算
        uint16_t brightness = r + g + b;
        
        if (brightness < 10) { // 如果亮度非常低，则计为黑像素
            black_pixels++;
        }
    }
    
    // 如果超过90%的采样像素是黑色的，则认为是黑屏情况
    if ((black_pixels * 100) / (total_pixels / 100) > 90) {
        // 显示纯黑色屏幕
        st7789_fill_color(0x0000);
    } else {
        // 在显示屏上显示捕获的图像，使用正确的尺寸参数（320x240）
        st7789_draw_image(0, 0, 320, 240, (uint16_t*)config->image_buf);
    }
}

int main() {
    vreg_set_voltage(VREG_VOLTAGE_MAX);
    set_sys_clock_khz(120*1000, true);
    stdio_uart_init();
    // 提高波特率以加快数据传输速度
    uart_set_baudrate(uart0, 115200);
    printf("\n\nBooted!\n");
    
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    printf("PIN_LED init.\n");
    
    // 初始化显示屏
    st7789_init();
    printf("ST7789 init.\n");
    
    // 填充蓝色背景
    st7789_fill_color(0x001F);
    
    struct ov7670_config config;
    ov7670_configure(&config);
    printf("OV7670_init.\n");
    
    // 读取设备ID
    uint8_t midh = ov7670_reg_read(&config, 0x1C);
    uint8_t midl = ov7670_reg_read(&config, 0x1D);
    printf("MIDH = 0x%02x, MIDL = 0x%02x\n", midh, midl);

    // 自动启动流模式
    printf("Starting video stream mode automatically\n");
    int frame_count = 0;
    while (true) {
        ov7670_capture_frame(&config);
        process_and_display_frame(&config);
        
        
        // LED指示灯闪烁表示程序运行中（降低频率）
        static uint32_t led_toggle_counter = 0;
        led_toggle_counter++;
        if (led_toggle_counter % 5 == 0) { // 每5帧闪烁一次LED（降低LED切换频率）
            gpio_put(PIN_LED, !gpio_get(PIN_LED));
        }
    }
    

    return 0;
}