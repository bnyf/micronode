#ifndef MNODE_CALLBACK_H
#define MNODE_CALLBACK_H

#include "jerryscript.h"

// typedef struct {
//     jerry_value_t cb_fn;
//     mnode_callback *next;
// } mnode_callback;

#define MAX_QUEUE_LENGTH 20

void init_callback_queue();

void add_callback(jerry_value_t cb_fn);

void call_callback(jerry_value_t cb_fn);

void add_callback_from_isr(void* args);

#endif
