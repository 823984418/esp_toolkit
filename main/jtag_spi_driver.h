#ifndef JTAG_SPI_DRIVER_H
#define JTAG_SPI_DRIVER_H

#include <driver/gpio.h>
#include <driver/spi_master.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jtag_config {
    gpio_num_t tdi_io_num;
    gpio_num_t tdo_io_num;
    gpio_num_t tck_io_num;
    gpio_num_t tms_io_num;
    int clock_speed_hz;
    spi_host_device_t spi_host;
} jtag_config_t;

typedef struct jtag_device {
    jtag_config_t config;
    spi_device_handle_t spi_device_handle;
} jtag_device_t;


void jtag_initialize(jtag_device_t *jtag);

void jtag_tms_set(jtag_device_t *jtag, uint32_t level);

void jtag_transport(jtag_device_t *jtag, size_t bits, const uint8_t *transmit, uint8_t *receive);

uint8_t jtag_clk_with(jtag_device_t *jtag, uint32_t tdo_level);

void jtag_state_move(jtag_device_t *jtag, size_t bits, const uint8_t *tms_buff);

void jtag_free(jtag_device_t *jtag);

#ifdef __cplusplus
}
#endif

#endif // JTAG_SPI_DRIVER_H
