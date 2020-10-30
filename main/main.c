/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"
#include "jerryscript-port.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

void app_main(void)
{
    printf("Hello world!\n");
    // init jerryscript
    /* Initialize engine */
    jerry_init(JERRY_INIT_EMPTY);
    jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);
    
    const jerry_char_t script[] = "print ('Hello JS!');";
    while (true)
    {
        jerry_value_t parsed_code = jerry_parse (NULL, 0, script, sizeof (script) - 1, JERRY_PARSE_NO_OPTS);
        if (jerry_value_is_error(parsed_code)) {
          printf("Unexpected error\n");
        } else {
          jerry_value_t ret_value = jerry_run(parsed_code);
          jerry_release_value(ret_value);
        }
        jerry_release_value(parsed_code);

        // alive check here. but nothing to do now!
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    /* Cleanup engine */
    jerry_cleanup();
}
