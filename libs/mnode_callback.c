#include "mnode_callback.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

QueueHandle_t xQueue;

void init_callback_queue() {
    xQueue = xQueueCreate(MAX_QUEUE_LENGTH, sizeof(jerry_value_t));
}

// intr
void add_callback(jerry_value_t cb_fn) {
    xQueueSend(xQueue, &cb_fn, portMAX_DELAY);
}

void add_callback_from_isr(void* args) {
    jerry_value_t cb_fn = (jerry_value_t)args;
    BaseType_t xHigherPriorityTaskWokenByPost = pdFALSE;
    xQueueSendFromISR(xQueue, &cb_fn, &xHigherPriorityTaskWokenByPost);
}

void call_callback(jerry_value_t cb_fn) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "call cb\n");

    jerry_value_t target_function = cb_fn;

    if (jerry_value_is_function (target_function)) {
        jerry_value_t this_val = jerry_create_undefined ();
        jerry_value_t ret_val = jerry_call_function (target_function, this_val, NULL, 0);

        if (jerry_value_is_error (ret_val)){
            jerry_port_log(JERRY_LOG_LEVEL_ERROR, "call function error\n");
        }

        jerry_release_value (ret_val);
        jerry_release_value (this_val);
    }

    jerry_release_value (target_function);

}