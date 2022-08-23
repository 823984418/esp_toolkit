//
// Created by q8239 on 2022/6/22.
//

#include "wifi_sta.h"

#include <esp_wifi.h>
#include <esp_log.h>

#define TAG "WIFI_STA"


esp_ip4_addr_t wifi_sta_ip = {0};

void wifi_event_handler_t(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START: {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        }
        case WIFI_EVENT_STA_CONNECTED: {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED: {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        }
        default:
            break;
    }
}

void ip_event_handler_t(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
            wifi_sta_ip = event->ip_info.ip;
            ESP_LOGI(TAG, "IP: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
            break;
        }
        case IP_EVENT_STA_LOST_IP: {
            wifi_sta_ip.addr = 0;
            break;
        }
        default:
            break;
    }
}

void wifi_sta_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler_t, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler_t, NULL, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifiConfig = {
        .sta = {
            .ssid = "SteamTicket",
            .password = "11113355",
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SECURITY,
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());
}
