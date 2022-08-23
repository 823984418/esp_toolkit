//
// Created by q8239 on 2022/6/22.
//

#ifndef ESP32_WEB_CAMERA_WIFI_STA_H
#define ESP32_WEB_CAMERA_WIFI_STA_H

#include <esp_wifi.h>

#ifdef __cplusplus
extern "C" {
#endif

extern esp_ip4_addr_t wifi_sta_ip;

void wifi_sta_init();

#ifdef __cplusplus
}
#endif

#endif //ESP32_WEB_CAMERA_WIFI_STA_H
