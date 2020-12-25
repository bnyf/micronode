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
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"


#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

#define MAX_CODE_LENGTH 1024
static const char *TAG = "example";

jerry_char_t script[MAX_CODE_LENGTH] = { "var x = 7; \
var y = 4; \
function add(x, y) \
{ return x+y }; \
print(add(x,y));" };

// jerry_char_t script[MAX_CODE_LENGTH] = { "print ('hello micronode')" };

SemaphoreHandle_t xSemaphore;

void jerry_ext_handler_init() {
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);
}

void store_js_code(const char* js_code, int length) {
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/spiffs/js.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "%.*s\n",length, js_code);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    mempcpy(script, js_code, length);
    script[length] = 0;

}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // "/code/qos2"
            msg_id = esp_mqtt_client_subscribe(client, "/code/qos2", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "print ('data')", 0, 0, 0);
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

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = "192.168.3.22",
        .port = 1883
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
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void vfs_init() {
    /* vfs */
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
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

void mqtt_init() {
    /* mqtt */
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

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
}

void rw_file() {
    /* rw file */
    //Check if destination file exists before renaming
    struct stat st;
    if (stat("/spiffs/js.txt", &st) != 0) {
      // Use POSIX and C standard library functions to work with files.
      // First create a file.
      ESP_LOGI(TAG, "Opening file");
      FILE* f = fopen("/spiffs/js.txt", "w");
      if (f == NULL) {
          ESP_LOGE(TAG, "Failed to open file for writing");
          return;
      }
      fprintf(f, "%s", script);
      fclose(f);
      ESP_LOGI(TAG, "File written");
    }
    else {
      // Open renamed file for reading
      ESP_LOGI(TAG, "Reading file");
      FILE* f = fopen("/spiffs/js.txt", "r");
      if (f == NULL) {
          ESP_LOGE(TAG, "Failed to open file for reading");
          return;
      }
      char line[64];
      memset(line, 0 ,sizeof(line));
      fgets(line, sizeof(line), f);
      fclose(f);

      // strip newline
      char* pos = strchr(line, '\n');
      if (pos) {
          *pos = '\0';
      }
      ESP_LOGI(TAG, "Read from file: '%s'", line);

      if(strlen(line) > 0) {
        mempcpy(script,line,sizeof(line));
      }
      ESP_LOGI(TAG, "Script: '%s'", (char*)script);
    }
}

void app_main(void) {
    vfs_init();
    rw_file();

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    mqtt_init();
    mqtt_app_start();

    vSemaphoreCreateBinary( xSemaphore );

    /* Initialize jerryscript engine */
    jerry_init(JERRY_INIT_EMPTY);
    jerry_ext_handler_init();

    while (true) {
        if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ) {
            bool ret_value = jerry_run_simple (script, sizeof (script) - 1, JERRY_INIT_EMPTY);
            if(ret_value) {
                ESP_LOGI(TAG, "jerry run successfully");
            }
            else {
                ESP_LOGI(TAG, "jerry run error");
            }
           // We were able to obtain the semaphore and can now access the
           // shared resource.

           // ...

           // We have finished accessing the shared resource.  Release the
           // semaphore.
           xSemaphoreGive( xSemaphore );
        }   

        // jerry_value_t parsed_code = jerry_parse (NULL, 0, script, strlen((char*)script), JERRY_PARSE_NO_OPTS);
        // if (jerry_value_is_error(parsed_code)) {
        //   printf("Unexpected error\n");
        // } else {
        //   jerry_value_t ret_value = jerry_run(parsed_code);
        //   jerry_release_value(ret_value);
        // }
        // jerry_release_value(parsed_code);
        // printf("-------\n");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }

    /* Cleanup engine */
    jerry_cleanup();
}
