#include <stdio.h>
#include <stdlib.h>

#include "jerryscript.h"

#include "mnode_utils.h"
#include "mnode_config.h"
#include "mnode_callback.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static QueueHandle_t xQueue; // queue is thread safe

BaseType_t js_mq_send(void* parameter) {
    BaseType_t ret = pdFAIL;
    if (xQueue) {
        ret = xQueueSend(xQueue, (void *)&parameter, portMAX_DELAY);
    }
    return ret;
}

// #define JERRY_EXIT  1

/* Allocate JerryScript heap for each thread. */
static void *
context_alloc_fn (size_t size, void *cb_data) {
  (void) cb_data;
  return malloc (size);
}

// static void _jerry_exit(void)
// {
//     void *exit = (void *)JERRY_EXIT;

//     js_mq_send(exit);
// }

extern void jerry_port_default_set_current_context (jerry_context_t*);

static void jerry_task_entry(void* parameter) {
    static const char *TAG = "jerry_task_entry";
    char* filename = (char* )parameter;
    ESP_LOGI(TAG, "%s\n",filename);
    uint8_t* script;
    size_t length;

    if (filename == NULL) {
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"jerry_thread_entry (filename == NULL)\n");
        return;
    }

    /* JERRY_ENABLE_EXTERNAL_CONTEXT */
    // jerry_port_default_set_current_context(jerry_create_context(MNODE_JMEM_HEAP_SIZE * 1024, context_alloc_fn, NULL));

    /* Initialize engine */
    jerry_init(JERRY_INIT_EMPTY);
    /* Register 'print' function from the extensions */
    jerryx_handler_register_global((const jerry_char_t *)"print", jerryx_handler_print);
    js_util_init();

    /* add __filename, __dirname */
    jerry_value_t global_obj = jerry_get_global_object();
    char *full_path = malloc(1024);
    // char *full_dir = NULL;

    uint32_t sz = jerry_port_normalize_path((char*)filename, full_path, 1024, "/framework");
    if(sz == 0) {
        jerry_port_log(JERRY_LOG_LEVEL_ERROR,"normalize_path fail\n");
    }
    // full_dir = js_module_dirname(full_path);

    // js_set_string_property(global_obj, "__dirname", full_dir);
    js_set_string_property(global_obj, "__filename", full_path);
    jerry_release_value(global_obj);

    script = jerry_port_read_source((const char*)full_path, &length);
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"jerry read file : %s\n", (const char*)filename);
    free(full_path);
    if (length == 0) 
        return;

    /* Setup Global scope code */
    jerry_value_t parsed_code = jerry_parse(NULL, 0, (jerry_char_t*)script, length, JERRY_PARSE_NO_OPTS);
    if (jerry_value_is_error(parsed_code)) {
        jerry_port_log(JERRY_LOG_LEVEL_ERROR, "jerry parse failed!\n");
    }
    else {
        xQueue = xQueueCreate(128, sizeof(void *));
        if (xQueue) {
            js_mq_func_init(js_mq_send);

            /* Execute the parsed source code in the Global scope */
            jerry_value_t ret = jerry_run(parsed_code);
            if (jerry_value_is_error(ret)) {
                jerry_port_log(JERRY_LOG_LEVEL_ERROR, "jerry run err!!!\n");
            }
            else {
                while (pdTRUE) {
                    struct js_mq_callback *jmc = NULL;
                    if (xQueueReceive(xQueue, &jmc, portMAX_DELAY)) {
                        if (jmc) {
                            js_call_callback(jmc->callback, jmc->args, jmc->size);
                            jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js call callback done, %d\n",(int)jmc->callback->function);
                            free(jmc);
                        }
                    }
                }
            }

            vQueueDelete(xQueue);
            js_mq_func_deinit();
            /* Returned value must be freed */
            jerry_release_value(ret);
        }
    }

    /* Parsed source code must be freed */
    jerry_release_value(parsed_code);

    jerry_port_release_source(script);

    js_util_cleanup();
    /* Cleanup engine */
    jerry_cleanup();

    // free((void *)jerry_port_get_current_context());

    vTaskDelete(NULL);
}

int jerry_exec(const char* filename)
{
    TaskHandle_t xHandle = NULL;
    xTaskCreate( jerry_task_entry, "jerry_exec", 1024 * 32, (void *)filename, 1, &xHandle );
    configASSERT( xHandle );

    return 0;
}
