#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

// LED引脚
#define PIN_LED 25

// OV7670摄像头引脚定义
#define PIN_CAM_SIOC 21         // I2C0 SCL
#define PIN_CAM_SIOD 4          // I2C0 SDA
#define PIN_CAM_RESETB 17
#define PIN_CAM_XCLK 3
#define PIN_CAM_VSYNC 16
#define PIN_CAM_Y2_PIO_BASE 6   // Y2-Y9数据引脚基地址

// ST7789显示屏引脚定义
#define PIN_MISO -1             // 不使用 MISO，设为 -1
#define PIN_CS 5
#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_DC 2
#define PIN_RST 20
#define PIN_BL 22

#endif // PIN_DEFINITIONS_H