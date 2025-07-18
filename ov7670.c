#include "ov7670.h"
#include "ov7670_init.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "image.pio.h"
#include <stdio.h>

static const uint8_t OV2640_ADDR = 0x42 >> 1;

void ov2640_init(struct ov2640_config *config) {
	// XCLK generation (~20.83 MHz)
	gpio_set_function(config->pin_xclk, GPIO_FUNC_PWM);
	uint slice_num = pwm_gpio_to_slice_num(config->pin_xclk);
	// 6 cycles (0 to 5), 125 MHz / 6 = ~20.83 MHz wrap rate
	pwm_set_wrap(slice_num, 4);
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

	// ov2640_regs_write(config, OV7670_init);//regsDefault
	// ov2640_regs_write(config, regsRGB565);
	// ov2640_regs_write(config, regsQQVGA);
	// ov2640_regs_write(config, setDisablePixelClockDuringBlankLines);
	// ov2640_regs_write(config, setDisableHREFDuringBlankLines);
	ov2640_regs_write(config, OV7670_Reg1);//OV_reg

	
	printf("Initialise the camera itself over SCCB.\n");

	// Enable image RX PIO
	uint offset = pio_add_program(config->pio, &image_program);
	image_program_init(config->pio, config->pio_sm, offset, config->pin_y2_pio_base);
	printf("Enable image RX PIO.\n");
}

void ov2640_capture_frame(struct ov2640_config *config) {
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

	// Wait for vsync rising edge to start frame
	while (gpio_get(config->pin_vsync) == true);
	while (gpio_get(config->pin_vsync) == false);
	// while (gpio_get(15) == true);
	// while (gpio_get(15) == false);
	// while (gpio_get(15) == true);
	// while (gpio_get(15) == false);

	dma_channel_start(config->dma_channel);
	dma_channel_wait_for_finish_blocking(config->dma_channel);
}

void ov2640_reg_write(struct ov2640_config *config, uint8_t reg, uint8_t value) {
	uint8_t data[] = {reg, value};
	i2c_write_blocking(config->sccb, OV2640_ADDR, data, sizeof(data), false);
}

uint8_t ov2640_reg_read(struct ov2640_config *config, uint8_t reg) {
	i2c_write_blocking(config->sccb, OV2640_ADDR, &reg, 1, false);

	uint8_t value;
	i2c_read_blocking(config->sccb, OV2640_ADDR, &value, 1, false);

	return value;
}

void ov2640_regs_write(struct ov2640_config *config, const uint8_t (*regs_list)[2]) {
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
		ov2640_reg_write(config, reg, value);

		regs_list++;
		count++;
		sleep_ms(1);
	}
}
