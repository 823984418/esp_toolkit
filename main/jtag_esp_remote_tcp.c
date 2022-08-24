#include "jtag_esp_remote_tcp.h"

#include <stdint.h>
#include <sys/socket.h>
#include <esp_log.h>
#include <esp_task_wdt.h>

#include "jtag_spi_driver.h"

#define ESP_REMOTE_CMD_VER_1    1

#define ESP_REMOTE_CMD_RESET    1
#define ESP_REMOTE_CMD_SCAN     2
#define ESP_REMOTE_CMD_TMS_SEQ  3
#define ESP_REMOTE_CMD_SET_CLK  4

#define ESP_REMOTE_MAX_BUFF_SIZE     4095

#define ESP_REMOTE_PORT         5555

static const char *TAG = "jtag_esp_remote_tcp";

typedef union esp_remote_cmd {
    uint8_t command[4];
    struct {
        uint16_t reserved: 4;
        uint16_t ver: 4;
        uint16_t function: 8;
        union {
            uint16_t function_specific;
            struct {
                uint16_t srst: 1;
                uint16_t trst: 1;
                uint16_t reserved: 14;
            } reset;
            struct {
                uint16_t bits: 12;
                uint16_t read: 1;
                uint16_t flip_tms: 1;
                uint16_t reserved: 2;
            } scan;
            struct {
                uint16_t bits: 12;
                uint16_t reserved: 4;
            } tms_seq;
        };
    };
} esp_remote_cmd_t;

void jtag_esp_remote_tcp_server() {
    int ret;

    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_socket < 0) {
        ESP_LOGE(TAG, "failed to create socket: %d", errno);
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    // for ipv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ESP_REMOTE_PORT);

    ESP_LOGI(TAG, "jtag_esp_remote port:%d", ESP_REMOTE_PORT);

    ret = bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (ret != 0) {
        ESP_LOGE(TAG, "failed to bind: %d", errno);
        return;
    }

    ret = listen(server_socket, 1);
    if (ret != 0) {
        ESP_LOGE(TAG, "failed to listen: %d", errno);
        return;
    }

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    // accept

    socklen_t addrlen = sizeof(client_addr);
    int connect = accept(server_socket, (struct sockaddr *) &client_addr, &addrlen);
    if (connect < 0) {
        ESP_LOGE(TAG, "failed to accept: %d", errno);
        return;
    }

    jtag_esp_remote_process(connect);

    ret = closesocket(server_socket);
    if (ret != 0) {
        ESP_LOGE(TAG, "failed to close: %d", errno);
        return;
    }
    ESP_LOGI(TAG, "begin jtag_esp_remote in %d", socket);

}

void jtag_esp_remote_process(int connect) {
    int ret;
    uint8_t *buff = (uint8_t *) malloc(ESP_REMOTE_MAX_BUFF_SIZE);
    if (buff == NULL) {
        ESP_LOGE(TAG, "malloc %llu fail", (size_t) ESP_REMOTE_MAX_BUFF_SIZE);
    }
    esp_remote_cmd_t cmd;

    jtag_device_t jtag = {
        .config = {
            .tdi_io_num = GPIO_NUM_4,
            .tdo_io_num = GPIO_NUM_5,
            .tck_io_num = GPIO_NUM_6,
            .tms_io_num = GPIO_NUM_7,
            .spi_host = SPI2_HOST,
            .clock_speed_hz = 100000,
        },
        .spi_device_handle = NULL,
    };
    jtag_initialize(&jtag);

    for (;;) {
        esp_task_wdt_reset();
        ssize_t len;
        len = recv(connect, cmd.command, 4, 0);
        if (len != 4) {
            ESP_LOGE(TAG, "failed to recv %d: %d\n", len, errno);
            break;
        }
        ESP_LOGI(TAG, "cmd %02X", cmd.function & 0xFF);
        switch (cmd.function) {
            case ESP_REMOTE_CMD_RESET: {
                ESP_LOGI(TAG, "CMD_RESET");
                jtag_free(&jtag);
                jtag_initialize(&jtag);
                break;
            }
            case ESP_REMOTE_CMD_SCAN: {
                ESP_LOGI(TAG, "CMD_SCAN bits:%d, read:%d, flip_tms:%d", cmd.scan.bits, cmd.scan.read, cmd.scan.flip_tms);
                size_t bits = cmd.scan.bits;
                if (bits == 0) {
                    continue;
                }
                size_t bytes = (bits + 7) / 8;
                len = recv(connect, buff, bytes, 0);
                if (len != bytes) {
                    ESP_LOGE(TAG, "failed to recv %d: %d\n", len, errno);
                    break;
                }
                for (size_t i = 0; i < bytes; i++) {
                    printf("%02X ", buff[i]);
                }
                printf("\n");

                bool read = cmd.scan.read;

                if (cmd.scan.flip_tms) {
                    jtag_transport_last_tms(&jtag, bits, buff, read ? buff : NULL);
                } else {
                    jtag_transport(&jtag, bits, buff, read ? buff : NULL);
                }

                if (read) {
                    for (size_t i = 0; i < bytes; i++) {
                        printf("%02X ", buff[i]);
                    }
                    printf("\n");
                    len = send(connect, buff, bytes, 0);
                    if (len != bytes) {
                        ESP_LOGE(TAG, "failed to send %d: %d\n", len, errno);
                        break;
                    }
                }
                break;
            }
            case ESP_REMOTE_CMD_TMS_SEQ: {
                ESP_LOGI(TAG, "CMD_TMS_SEQ bits:%d", cmd.tms_seq.bits);
                size_t bits = cmd.scan.bits;
                if (bits == 0) {
                    continue;
                }
                size_t bytes = (bits + 7) / 8;
                len = recv(connect, buff, bytes, 0);
                if (len != bytes) {
                    ESP_LOGE(TAG, "failed to recv %d: %d\n", len, errno);
                    break;
                }
                for (size_t i = 0; i < bytes; i++) {
                    printf("%02X ", buff[i]);
                }
                printf("\n");
                jtag_state_move(&jtag, bits, buff);
                break;
            }
            case ESP_REMOTE_CMD_SET_CLK: {
                ESP_LOGI(TAG, "CMD_SET_CLK");
                len = recv(connect, buff, 4, 0);
                if (len != 4) {
                    ESP_LOGE(TAG, "failed to recv %d: %d\n", len, errno);
                    break;
                }
                int speed = ((int) buff[0] << 24) | ((int) buff[1] << 16) | ((int) buff[2] << 8) | ((int) buff[3]);
                ESP_LOGI(TAG, "speed:%d", speed);
                jtag.config.clock_speed_hz = speed;
                break;
            }
            default: {
                ESP_LOGE(TAG, "Unknown cmd");
            }
        }
    }
    ret = closesocket(connect);
    if (ret < 0) {
        ESP_LOGE(TAG, "failed to close: %d", errno);
    }
    jtag_free(&jtag);
}

