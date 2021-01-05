#ifndef ESP32_INIT_H
#define ESP32_INIT_H

#define MQTT_HOST "8.131.73.206"
#define MQTT_PORT 1883
#define WIFI_SSID "haiway"
#define WIFI_PASS "Seaway2019"

#define MAX_CODE_LENGTH 1024

void net_init();
void start_mqtt();

void vfs_init();
void load_js_entry();

void start_jerry();
void end_jerry();

void init();

#endif
