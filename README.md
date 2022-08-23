# ESP32 调试工具箱

此工具箱旨在使用单个ESP32C3芯片完成对ESP32的无线下载调试功能，并且提供一些附属的扩展功能

## 使用示例
更改工具连接的WIFI信息，完成烧录后通过串口监视器获取其IP信息
更改 `esp32-remote.cfg` 中的 
`jtag_esp_remote_set_address` 为此无线调试器IP
`jtag_esp_remote_set_port` 为所使用的TCP端口(默认为5555)

导出 esp-idf 的环境变量
```
%IDF_PATH%/export.bat
```

运行为esp32定制的OpenOCD
下方 `esp_toolkit` 为项目的绝对路径
```
OpenOCD -f esp_toolkit/esp32-remote.cfg
```

## TODO
蓝牙串口
其他适用于普通OpenOCD的无线调试器
适用于sigrok(PulseView)的无线逻辑分析仪
