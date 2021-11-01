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

BaseType_t js_mq_send_from_isr(void* parameter) {
    BaseType_t ret = pdFAIL;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xQueue) {
        ret = xQueueSendFromISR(xQueue, (void *)&parameter, &xHigherPriorityTaskWoken);
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

static void jerry_callback_task_entry(void* para) {
    while (pdTRUE) {
        struct js_mq_callback *jmc = NULL;
        if (xQueueReceive(xQueue, &jmc, portMAX_DELAY)) {
            if (jmc) {
                js_call_callback(jmc->callback, jmc->args, jmc->size);
                jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "Js call callback done in %d, %s\n",(int)jmc->callback, (char*)para );
                free(jmc);
            }
        }
    }
}

static void jerry_exec_task_entry(void* parameter) {
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
            js_mq_func_init_isr(js_mq_send_from_isr);
        }

        // TaskHandle_t xHandle_cb[CORENUM];
        // for(int i=0;i<CORENUM - 1;++i) {
        //     xTaskCreate( jerry_callback_task_entry, "jerry_callback", 1024 * 16, (void *)filename, 1, &xHandle_cb[i] );
        //     configASSERT( xHandle_cb[i] );
        // }

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
                        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js call callback done, %d\n",(int)jmc->callback);
                        free(jmc);
                    }
                }
            }
        }
        // for(int i=0;i<CORENUM - 1;++i) {
        //     vTaskDelete( xHandle_cb[i] );
        // }
        vQueueDelete(xQueue);
        js_mq_func_deinit();

        /* Returned value must be freed */
        jerry_release_value(ret);
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

#ifdef DEBUG

void test() {
    const jerry_char_t script[] = "print('Hello, World!');";
    const jerry_length_t script_size = sizeof (script) - 1;

    /* Note: sizeof can be used here only because the compiler knows the static character arrays's size.
    * If this is not the case, strlen should be used instead.
    */

    /* JERRY_ENABLE_EXTERNAL_CONTEXT */
    jerry_port_default_set_current_context(jerry_create_context(MNODE_JMEM_HEAP_SIZE * 1024, context_alloc_fn, NULL));

    /* Initialize engine */
    jerry_init (JERRY_INIT_EMPTY);
    jerryx_handler_register_global((const jerry_char_t *)"print", jerryx_handler_print);

    /* Run the demo script with 'eval' */
    jerry_value_t eval_ret = jerry_eval (script,
                                        script_size,
                                        JERRY_PARSE_NO_OPTS);

    /* Parsed source code must be freed */
    jerry_release_value (eval_ret);

    /* Cleanup engine */
    jerry_cleanup ();
}

void recv(void* args) {
    printf("enter recv: %s\n",(char *)args);
    int x;
    while (xQueueReceive(xQueue, &x, portMAX_DELAY)) { 
        printf("%s: %d\n", (char *)args, x);
        vTaskDelay(1000);
    }
}

void send(void* args) {
    printf("enter send: %s\n",(char *)args);
    int x = 1;
    while(xQueueSend(xQueue, &x, portMAX_DELAY)) {
        printf("send: %d\n",x);
        x++;
        vTaskDelay(500);
    }
}
#endif

int jerry_exec(const char* filename)
{
#ifdef DEBUG
    xQueue = xQueueCreate(128, sizeof(int));
    printf("jerry_exec\n");
    TaskHandle_t xHandle_exec = NULL;
    xTaskCreate( recv, "recv1", 1024 * 16, (void *)"recv1", 1, &xHandle_exec );
    xTaskCreate( recv, "recv2", 1024 * 16, (void *)"recv2", 1, &xHandle_exec );
    xTaskCreate( send, "send", 1024 * 16, (void *)"send", 1, &xHandle_exec );  
#else

    TaskHandle_t xHandle_exec = NULL;
    xTaskCreate( jerry_exec_task_entry, "jerry_exec", 1024 * 16, (void *)filename, 1, &xHandle_exec );
    configASSERT( xHandle_exec );

#endif
    return 0;
}

