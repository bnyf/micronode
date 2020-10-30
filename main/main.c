/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "jerryscript-port.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

static const char *TAG = "example";

void jerry_ext_handler_init() {
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);
}

void app_main(void)
{
  jerry_char_t script[1024] = "print ('hello world');";

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



  // Check if destination file exists before renaming
  struct stat st;
  if (stat("/spiffs/hello.txt", &st) != 0) {
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/spiffs/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "print ('Hello JS!');\n");
    fclose(f);
    ESP_LOGI(TAG, "File written");
    esp_restart();
  }
  else {
    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");
    FILE* f = fopen("/spiffs/hello.txt", "r");
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


  /* Initialize engine */
  jerry_init(JERRY_INIT_EMPTY);
  jerry_ext_handler_init();

  while (true)
  {
      jerry_value_t parsed_code = jerry_parse (NULL, 0, script, strlen((char*)script), JERRY_PARSE_NO_OPTS);
      if (jerry_value_is_error(parsed_code)) {
        printf("Unexpected error\n");
      } else {
        jerry_value_t ret_value = jerry_run(parsed_code);
        jerry_release_value(ret_value);
      }
      jerry_release_value(parsed_code);

      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

  /* Cleanup engine */
  jerry_cleanup();
}
