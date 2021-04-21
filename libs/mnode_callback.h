#ifndef MNODE_CALLBACK_H
#define MNODE_CALLBACK_H

#include "mnode_utils.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*js_callback_func)(const void *args, uint32_t size);
typedef BaseType_t(*js_mq_func)(void *args); // send to mq

struct js_mq_callback
{
    js_callback_func callback;
    void *args;
    uint32_t size;
};

void js_call_callback(js_callback_func callback, const void *data, uint32_t size);
BaseType_t js_send_callback_from_isr(js_callback_func callback, const void *args, uint32_t size);
BaseType_t js_send_callback(js_callback_func callback, const void *args, uint32_t size);
void js_mq_func_init(js_mq_func signal);
void js_mq_func_init_isr(js_mq_func signal);
void js_mq_func_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
