#include "mnode_callback.h"
#include "string.h"

struct js_callback *_js_callback = NULL;
static js_mq_func _js_mq_func = NULL;

static void append_callback(struct js_callback *callback) {
    struct js_callback *_callback = _js_callback;

    if (_js_callback == NULL) {
        _js_callback = callback;
        return;
    }
    
    while (_callback->next != NULL) {
        _callback = _callback->next;
    }

    _callback->next = callback;
}

static void remove_callback(struct js_callback *callback) {
    struct js_callback *_callback = _js_callback;

    if (_js_callback == NULL)
        return;
    
    if (_js_callback == callback) {
        _js_callback = _js_callback->next;
        free(callback);
        return;
    }

    while (_callback->next != NULL){
        if (_callback->next == callback) {
            _callback->next = callback->next;
            free(callback);
            break;
        }
        _callback = _callback->next;
    }
}

static BaseType_t has_callback(struct js_callback *callback)
{
    struct js_callback *_callback = _js_callback;

    if (callback == NULL || _js_callback == NULL) {
        return pdFALSE;
    }

    do {
        if (_callback == callback) {
            return pdTRUE;
        }
        _callback = _callback->next;
    }
    while (_callback != NULL);

    return pdFALSE;
}

struct js_callback* js_add_callback(js_callback_func callback)
{
    struct js_callback *cb = (struct js_callback *)malloc(sizeof(struct js_callback));
    if (!cb) {
        return NULL;
    }

    cb->function = (js_callback_func)callback;
    cb->next = NULL;

    append_callback(cb);

    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Add js callback\n");

    return cb;
}

void js_remove_callback(struct js_callback *callback)
{
    remove_callback(callback);
}

void js_remove_all_callbacks(void) {
    struct js_callback *_callback_free;

    while (_js_callback != NULL) {
        _callback_free = _js_callback;
        _js_callback = _js_callback->next;
        free(_callback_free);
    }
    _js_callback = NULL;
}

void js_call_callback(struct js_callback *callback, const void *data, uint32_t size) {
    if (has_callback(callback)) {
        if (callback->function) {
            callback->function(data, size);
        }
    }
}

BaseType_t js_send_callback(struct js_callback *callback, const void *args, uint32_t size) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"mnode_callback, Js send callbakc, %d\n",(int)callback->function);
    BaseType_t ret = pdFAIL;
    struct js_mq_callback *jmc = NULL;

    jmc = (struct js_mq_callback *)malloc(sizeof(struct js_mq_callback));
    if (jmc) {
        jmc->callback = callback;
        jmc->args = malloc(size);
        if (jmc->args && args) {
            memcpy(jmc->args, args, size);
        }
        jmc->size = size;

        if (_js_mq_func) {
            ret = _js_mq_func(jmc);
        }

        if (ret == pdFAIL) {
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js_end_callback fail\n");
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

void js_mq_func_deinit(void)
{
    _js_mq_func = NULL;
    js_remove_all_callbacks();
}
