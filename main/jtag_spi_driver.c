#include "jtag_spi_driver.h"


void jtag_initialize(jtag_device_t *jtag) {
    ESP_ERROR_CHECK(gpio_set_direction(jtag->config.tms_io_num, GPIO_MODE_OUTPUT));
    spi_bus_config_t spiBusConfig = {
        .mosi_io_num = jtag->config.tdi_io_num,
        .miso_io_num = jtag->config.tdo_io_num,
        .sclk_io_num = jtag->config.tck_io_num,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(jtag->config.spi_host, &spiBusConfig, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t spiDeviceInterfaceConfig = {
        .flags = SPI_DEVICE_BIT_LSBFIRST,
        .queue_size = 1,
        .mode = 0, // todo
        .clock_speed_hz = jtag->config.clock_speed_hz,
        .spics_io_num = GPIO_NUM_NC,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(jtag->config.spi_host, &spiDeviceInterfaceConfig, &jtag->spi_device_handle));

    ESP_ERROR_CHECK(spi_device_acquire_bus(jtag->spi_device_handle, portMAX_DELAY));

    jtag_tms_set(jtag, 0);
}

void jtag_tms_set(jtag_device_t *jtag, uint32_t level) {
    ESP_ERROR_CHECK(gpio_set_level(jtag->config.tms_io_num, level));
}

void jtag_transport(jtag_device_t *jtag, size_t bits, const uint8_t *tdo_buff, uint8_t *tdi_buff) {
    spi_transaction_t spiTransaction = {
        .length = bits,
        .tx_buffer = tdo_buff,
        .rx_buffer = tdi_buff,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(jtag->spi_device_handle, &spiTransaction));
}

void jtag_transport_last_tms(jtag_device_t *jtag, size_t bits, const uint8_t *tdo_buff, uint8_t *tdi_buff) {
    size_t bytes = (bits + 7) / 8;
    uint8_t last_tdo_buff = tdo_buff[bytes - 1];
    uint8_t last_bit = 1 << ((bits - 1) & 7);
    jtag_transport(jtag, bits - 1, tdo_buff, tdi_buff);
    jtag_tms_set(jtag, 1);
    if (jtag_clk_with(jtag, (last_tdo_buff & last_bit) != 0)) {
        if (tdi_buff) {
            tdi_buff[bytes - 1] |= last_bit;
        }
    } else {
        if (tdi_buff) {
            tdi_buff[bytes - 1] &= ~last_bit;
        }
    };
    jtag_tms_set(jtag, 0);
}


uint8_t jtag_clk_with(jtag_device_t *jtag, uint32_t tdo_level) {
    uint8_t data;
    if (tdo_level) {
        data = 1;
    } else {
        data = 0;
    }
    jtag_transport(jtag, 1, &data, &data);
    return data & 1;
}

void jtag_state_move(jtag_device_t *jtag, size_t bits, const uint8_t *tms_buff) {
    if (bits != 0) {
        size_t bytes = (bits + 7) / 8;
        for (size_t i = 0; i < bytes - 1; i++) {
            uint8_t b = tms_buff[i];
            for (size_t j = 0; j < 8; j++) {
                if (b & BIT(j)) {
                    jtag_tms_set(jtag, 1);
                } else {
                    jtag_tms_set(jtag, 0);
                }
                jtag_clk_with(jtag, 0);
            }
        }
        uint8_t b = tms_buff[bytes - 1];
        for (size_t j = 0; j < (bits & 7); j++) {
            if (b & (1 << (j))) {
                jtag_tms_set(jtag, 1);
            } else {
                jtag_tms_set(jtag, 0);
            }
            jtag_clk_with(jtag, 0);
        }
    }
    jtag_tms_set(jtag, 0);
}

void jtag_free(jtag_device_t *jtag) {
    spi_device_release_bus(jtag->spi_device_handle);
    ESP_ERROR_CHECK(spi_bus_remove_device(jtag->spi_device_handle));
    ESP_ERROR_CHECK(spi_bus_free(jtag->config.spi_host));
}


