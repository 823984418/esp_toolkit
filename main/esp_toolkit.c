#include <stdio.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include "jtag_esp_remote_tcp.h"
#include "wifi_sta.h"

void jtag_server_task(void *args) {
    for (;;) {
        jtag_esp_remote_tcp_server();
    }
}

void app_main() {
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_sta_init();

    xTaskCreate(jtag_server_task, "jtag_server_task", 4096, NULL, 3, NULL);
}
