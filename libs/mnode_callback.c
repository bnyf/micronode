#include "mnode_callback.h"

#include <stdint.h>
#include <string.h>

struct js_callback *_js_callback = NULL;
static js_mq_func _js_mq_func = NULL;
static js_mq_func _js_mq_func_from_isr = NULL;

void js_call_callback(js_callback_func callback, const void *data, uint32_t size) {
    if (callback) {
        callback(data, size);
    }
}

BaseType_t js_send_callback_from_isr(js_callback_func callback, const void *args, uint32_t size) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "mnode_callback, Js send callback from isr, %d\n", (int)callback);
    BaseType_t ret = pdFAIL;
    struct js_mq_callback *jmc = NULL;

    jmc = (struct js_mq_callback *)malloc(sizeof(struct js_mq_callback));
    if (jmc)
    {
        jmc->callback = callback;
        jmc->args = malloc(size);
        if (jmc->args && args)
        {
            memcpy(jmc->args, args, size);
        }
        jmc->size = size;

        if (_js_mq_func)
        {
            ret = _js_mq_func_from_isr(jmc);
        }

        if (ret == pdFAIL)
        {
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "Js_end_callback fail\n");
            free(jmc->args);
            free(jmc);
        }
    }

    return ret;
}

BaseType_t js_send_callback(js_callback_func callback, const void *args, uint32_t size)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "mnode_callback, Js send callback, %d\n", (int)callback);
    BaseType_t ret = pdFAIL;
    struct js_mq_callback *jmc = NULL;

    jmc = (struct js_mq_callback *)malloc(sizeof(struct js_mq_callback));
    if (jmc)
    {
        jmc->callback = callback;
        jmc->args = malloc(size);
        if (jmc->args && args)
        {
            memcpy(jmc->args, args, size);
        }
        jmc->size = size;

        if (_js_mq_func)
        {
            ret = _js_mq_func(jmc);
        }

        if (ret == pdFAIL)
        {
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "Js_end_callback fail\n");
            free(jmc->args);
            free(jmc);
        }
    }

    return ret;
}

void js_mq_func_init(js_mq_func signal)
{
    _js_mq_func = signal;
}

void js_mq_func_init_isr(js_mq_func signal) {
    _js_mq_func_from_isr = signal;
}
void js_mq_func_deinit(void)
{
    _js_mq_func = NULL;
    _js_mq_func_from_isr = NULL;
}
