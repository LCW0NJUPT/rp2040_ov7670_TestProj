#include "ov7670.h"
#include "ov7670_init.h"
#include "pin_definitions.h"
#include "image.pio.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"

extern uint8_t image_buf[320*240*2];

static const uint8_t OV7670_ADDR = 0x42 >> 1;

void ov7670_init(struct ov7670_config *config) {
	// XCLK generation (~24 MHz)
    // 将XCLK引脚（GPIO3）配置为PWM功能
	gpio_set_function(config->pin_xclk, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(config->pin_xclk);
	// 5 cycles (0 to 4), 120 MHz / 5 = 24 MHz wrap rate
    // PWM周期设置为4（即0-4，共5个计数周期）
	pwm_set_wrap(slice_num, 4);
    // PWM占空比设置为2（50%）
	pwm_set_gpio_level(config->pin_xclk, 2);
	pwm_set_enabled(slice_num, true);
	printf("XCLK generation (~20.83 MHz).\n");

	// SCCB I2C @ 100 kHz
	gpio_set_function(config->pin_sioc, GPIO_FUNC_I2C);
	gpio_set_function(config->pin_siod, GPIO_FUNC_I2C);
	i2c_init(config->sccb, 10 * 1000);
	gpio_pull_up(config->pin_sioc);
    gpio_pull_up(config->pin_siod);
	printf("SCCB I2C @ 100 kHz.\n");

	// Initialise reset pin
	gpio_init(config->pin_resetb);
	gpio_set_dir(config->pin_resetb, GPIO_OUT);
	printf("Initialise reset pin.\n");

	// Reset camera, and give it some time to wake back up
	gpio_put(config->pin_resetb, 0);
	sleep_ms(100);
	gpio_put(config->pin_resetb, 1);
	sleep_ms(100);
	printf("Reset camera via reset pin.\n");

	// ov7670_regs_write(config, OV7670_init);//regsDefault
	// ov7670_regs_write(config, regsRGB565);
	// ov7670_regs_write(config, regsQQVGA);
	// ov7670_regs_write(config, setDisablePixelClockDuringBlankLines);
	// ov7670_regs_write(config, setDisableHREFDuringBlankLines);
	ov7670_regs_write(config, OV7670_Reg1);//OV_reg

	
	printf("Initialise the camera itself over SCCB.\n");

	// Enable image RX PIO
	uint offset = pio_add_program(config->pio, &image_program);
	image_program_init(config->pio, config->pio_sm, offset, config->pin_y2_pio_base);
	printf("Enable image RX PIO.\n");
}

// 封装OV7670配置过程的函数
void ov7670_configure(struct ov7670_config *config) {
    config->pin_xclk = 3;
    config->pin_vsync = 16;
    config->pin_resetb = 17;
    config->pin_y2_pio_base = 6;
    config->pin_sioc = 21;
    config->pin_siod = 4;
    
    config->sccb = i2c0;
    config->pio = pio0;
    config->pio_sm = 0;

    config->dma_channel = 0;
    config->image_buf = image_buf;
    config->image_buf_size = sizeof(image_buf);
    
    ov7670_init(config);
}

void ov7670_capture_frame(struct ov7670_config *config) {
    // 只在第一次捕获时丢弃前几帧以解决图像撕裂问题，之后不再丢帧以提高帧率
    static bool is_first_capture = true;
    if (is_first_capture) {
        // 预热阶段 - 丢弃前几帧以确保图像稳定
        // 根据注释，从丢弃3帧减少到丢弃1帧
        const int warmup_frames = 1;
        for (int i = 0; i < warmup_frames; i++) {
            dma_channel_config c = dma_channel_get_default_config(config->dma_channel);
            channel_config_set_read_increment(&c, false);
            channel_config_set_write_increment(&c, true);
            channel_config_set_dreq(&c, pio_get_dreq(config->pio, config->pio_sm, false));
            channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
            
            dma_channel_configure(
                config->dma_channel, &c,
                config->image_buf,  // 临时使用主缓冲区
                &config->pio->rxf[config->pio_sm],
                config->image_buf_size,
                false
            );

            // 等待完整的VSYNC信号周期
            while (gpio_get(config->pin_vsync) == true) tight_loop_contents();
            while (gpio_get(config->pin_vsync) == false) tight_loop_contents();

            dma_channel_start(config->dma_channel);
            dma_channel_wait_for_finish_blocking(config->dma_channel);
        }
        is_first_capture = false;
    }
    
    // 捕获最终帧
    dma_channel_config c = dma_channel_get_default_config(config->dma_channel);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(config->pio, config->pio_sm, false));
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    
    dma_channel_configure(
        config->dma_channel, &c,
        config->image_buf,
        &config->pio->rxf[config->pio_sm],
        config->image_buf_size,
        false
    );

    // 等待完整的VSYNC信号周期以确保捕获完整帧
    while (gpio_get(config->pin_vsync) == true) tight_loop_contents();
    while (gpio_get(config->pin_vsync) == false) tight_loop_contents();

    dma_channel_start(config->dma_channel);
    dma_channel_wait_for_finish_blocking(config->dma_channel);
}

void ov7670_reg_write(struct ov7670_config *config, uint8_t reg, uint8_t value) {
	uint8_t data[] = {reg, value};
	i2c_write_blocking(config->sccb, OV7670_ADDR, data, sizeof(data), false);
}

uint8_t ov7670_reg_read(struct ov7670_config *config, uint8_t reg) {
	i2c_write_blocking(config->sccb, OV7670_ADDR, &reg, 1, false);

	uint8_t value;
	i2c_read_blocking(config->sccb, OV7670_ADDR, &value, 1, false);

	return value;
}

void ov7670_regs_write(struct ov7670_config *config, const uint8_t (*regs_list)[2]) {
	uint16_t count=0;
	printf("ov7670 regs write. \n");
	while (1) {
		uint8_t reg = (*regs_list)[0];
		uint8_t value = (*regs_list)[1];
		printf("Writing register: ");
		printf("%d,",count);
		printf("content: 0x%02x 0x%02x,",reg,value);
		printf("\r\n");
		if (reg == 0x00 && value == 0x00) {
			break;
		}
		ov7670_reg_write(config, reg, value);

		regs_list++;
		count++;
		sleep_ms(1);
	}
}