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

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

#include "init.h"

#include "mnode_require.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

static const char *TAG = "main";

typedef struct xSemaphoreGroup_s {
    SemaphoreHandle_t code;
    SemaphoreHandle_t file;
}xSemaphoreGroup_t;

xSemaphoreGroup_t semagroup;
jerry_char_t script[MAX_CODE_LENGTH];
size_t script_size;
// jerry_char_t default_script[] = "var str = 'Hello, World!';"; 
// jerry_char_t default_script[] = "print ('micronode');"; 

const char *entry = "/framework/index.js";

static void resource_init() {
    // mempcpy(script, default_script, sizeof(default_script));
    // script_size = sizeof(default_script) - 1;
    vSemaphoreCreateBinary( semagroup.code );
    vSemaphoreCreateBinary( semagroup.file );
}

// jerry
static void jerry_ext_handler_init() {
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);
}
static void store_js_code(const char* js_code, int length) {

    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen(entry, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    if(xSemaphoreTake(semagroup.file, ( TickType_t ) 10 ) == pdTRUE) {
        fprintf(f, "%.*s\n",length, js_code);
        xSemaphoreGive(semagroup.file);
    }
    fclose(f);
    ESP_LOGI(TAG, "File written");

    if( xSemaphoreTake( semagroup.code, ( TickType_t ) 10 ) == pdTRUE ) {
        mempcpy(script, js_code, length);
        script_size = length;
        xSemaphoreGive( semagroup.code);
    }
    
}

static esp_err_t esp_mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "code", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, "code", "print ('data')", 0, 0, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            store_js_code(event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}
static void esp_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handler_cb(event_data);
}

void start_jerry() {
    jerry_init(JERRY_INIT_EMPTY);
    jerry_ext_handler_init();
    mnode_init_require();
}
void end_jerry() {
    jerry_cleanup();
}

void net_init() {
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
}

void start_mqtt() {
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = MQTT_HOST,
        .port = MQTT_PORT
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, esp_mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

// vfs
void vfs_init() {
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

    // if file doesn't exists, create it.
    // struct stat st;
    // if (stat(entry, &st) != 0) { 
    //   ESP_LOGI(TAG, "Opening file");
    //   FILE* f = fopen(entry, "w");
    //   if (f == NULL) {
    //       ESP_LOGE(TAG, "Failed to open file for writing");
    //       return;
    //   }
    //   fprintf(f, "%s", default_script);
    //   fclose(f);
    //   ESP_LOGI(TAG, "File written");
    // }
}
void load_js_entry() {
    // Open renamed file for reading
    size_t size = 0;
    jerry_char_t *buffer = jerry_port_read_source(entry, &size);
    // ESP_LOGI(TAG, "load_js_entry, buffer: '%s'", (char*)buffer);
    // ESP_LOGI(TAG, "load_js_entry, buffer size: '%u'", size);
    if (buffer == NULL) {
        printf("No such file: %s\n",entry);
        return;
    }
    
    if( xSemaphoreTake( semagroup.code, ( TickType_t ) 10 ) == pdTRUE ) {
        mempcpy(script, buffer, size);
        script_size = size;
        xSemaphoreGive( semagroup.code );
    }
    // ESP_LOGI(TAG, "load_js_entry, script: '%s'", (char*)script);
    
    jerry_port_release_source(buffer);
}

void init() {
    resource_init();
    
    // net_init();
    // start_mqtt();

    vfs_init();
    load_js_entry();
}

void app_main(void) {
    init();

    start_jerry();
    while (true) {
        if( xSemaphoreTake( semagroup.code, ( TickType_t ) 10 ) == pdTRUE ) {
            ESP_LOGI(TAG, "script:\n%s", script);
            ESP_LOGI(TAG, "script lenth: %u", script_size);

            bool run_ok = false;

            run_ok = jerry_eval(script, script_size, JERRY_PARSE_NO_OPTS);
            if(run_ok) {
                ESP_LOGI(TAG, "success\n");
            } else {
                ESP_LOGI(TAG, "unsuccess\n");
            }

            xSemaphoreGive( semagroup.code );
        }

        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    end_jerry();
}
