
#ifndef JTAG_ESP_REMOTE_TCP_H
#define JTAG_ESP_REMOTE_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

void jtag_esp_remote_tcp_server();

void jtag_esp_remote_process(int socket);

#ifdef __cplusplus
}
#endif

#endif // JTAG_ESP_REMOTE_TCP_H
