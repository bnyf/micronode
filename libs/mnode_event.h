#ifndef MNODE_EVENT_H
#define MNODE_EVENT_H

#include "mnode_utils.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

struct js_listener
{
    jerry_value_t func;
    struct js_listener *next;
};

struct js_event
{
    char *name;
    struct js_listener *listeners;
    struct js_event *next;
};

struct js_emitter
{
    struct js_event *events;
};

void js_add_event_listener(jerry_value_t obj, const char *event_name, jerry_value_t func);
void js_remove_event_listener(jerry_value_t obj, const char *event_name);
BaseType_t js_emit_event(jerry_value_t obj, const char *event_name, const jerry_value_t argv[], const jerry_length_t argc);
void js_destroy_emitter(jerry_value_t obj);
void js_make_emitter(jerry_value_t obj);
int js_event_init(void);
int js_event_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
