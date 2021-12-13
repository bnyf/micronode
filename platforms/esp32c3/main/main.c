#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"
#include "mnode_event.h"
#include "driver/uart.h"

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

// #define MY_DEBUG
#define UART

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define CHIP_NAME "ESP32-C3"
#endif

static const char *TAG = "main";

// vfs
void esp32_spiffs_vfs_init() {
    /* vfs */
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/framework",
        .partition_label = NULL,
        .max_files = 3,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

static void esp32_nvs_flash_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void handler_print()
{
    jerryx_handler_register_global((const jerry_char_t *) "print", jerryx_handler_print);
    js_util_init();
}

extern int jerry_exec(const char*);
extern void handle_uart_input();
extern void Uart0Recv();

void app_main(void) {
    printf("Entered in app_main\n");
    esp32_nvs_flash_init();
    printf("Finished esp32_nvsflash_init()\n");    
    esp32_spiffs_vfs_init();
    printf("Finished spiffs_init()");
#ifdef UART
    jerry_init(JERRY_INIT_EMPTY);
    //xTaskCreate(Uart0Recv, "uart_echo_task", 1024*96, NULL, 10, NULL);
    handler_print();
    handle_uart_input();
    printf("Enter in while loop\n");
    while(true){
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    jerry_cleanup();
#else
    jerry_exec("index.js");
#endif
}
