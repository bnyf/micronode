#include "mnode_event.h"
#include "string.h"

static jerry_value_t emitter = 0;
static jerry_value_t _js_emitter_prototype = 0;

static void remove_event(struct js_emitter *emitter, struct js_event *event);

static void free_listener(void *ptr)
{
    if (ptr)
    {
        struct js_listener *listener = (struct js_listener *)ptr;
        jerry_release_value(listener->func);
        free(listener);
    }
}

static void js_emitter_free_cb(void *native)
{
    struct js_emitter *emitter = (struct js_emitter *)native;
    struct js_event *_event,  *event = emitter->events;

    while (event)
    {
        _event = event;
        event = event->next;

        remove_event(emitter, _event);
    }

    free(emitter);
} 

static const jerry_object_native_info_t emitter_type_info =
{
    .free_cb = js_emitter_free_cb
};

static void js_event_proto_free_cb(void *native)
{
    _js_emitter_prototype = 0;
}

static const jerry_object_native_info_t event_proto_type_info =
{
    .free_cb = js_event_proto_free_cb
};

static struct js_listener *find_listener(struct js_event *event, jerry_value_t func)
{
    struct js_listener *_listener = event->listeners;

    while (_listener != NULL)
    {
        if (_listener->func == func)
        {
            break;
        }

        _listener = _listener->next;
    }

    return _listener;
}

static void append_listener(struct js_event *event, struct js_listener *listener)
{
    struct js_listener *_listener = event->listeners;

    if (event->listeners == NULL)
    {
        event->listeners = listener;
        return;
    }

    while (_listener->next != NULL)
    {
        _listener = _listener->next;
    }

    _listener->next = listener;
}

static void remove_listener(struct js_event *event, struct js_listener *listener)
{
    struct js_listener *_listener = event->listeners;

    if (event->listeners == listener || event->listeners == NULL)
    {
        event->listeners = NULL;
        return;
    }

    while (_listener->next != listener)
    {
        _listener = _listener->next;
    }

    _listener->next = listener->next;
}

static struct js_event *find_event(struct js_emitter *emitter, const char *event_name)
{
    struct js_event *event = emitter->events;

    while (event != NULL) {
        if (strcmp(event->name, event_name) == 0) {
            break;
        }

        event = event->next;
    }

    return event;
}

static void append_event(struct js_emitter *emitter, struct js_event *event)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Append event '%s' to emitter\n",event->name);
    struct js_event *_event = emitter->events;

    if (emitter->events == NULL)
    {
        emitter->events = event;
        return;
    }

    while (_event->next != NULL)
    {
        _event = _event->next;
    }

    _event->next = event;
}

static void remove_event(struct js_emitter *emitter, struct js_event *event)
{
    struct js_event *_event = emitter->events;
    struct js_listener *_listener, *listener = event->listeners;

    if (emitter->events == event)
    {
        emitter->events = event->next;
    }
    else
    {
        while (_event->next != event)
        {
            _event = _event->next;
        }

        _event->next = event->next;
    }

    while (listener != NULL)
    {
        _listener = listener;
        listener = listener->next;
        free_listener(_listener);
    }

    free(event->name);
    free(event);
}

void js_add_event_listener(jerry_value_t obj, const char *event_name, jerry_value_t func)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Enter js_add_event_listener func, %s\n", event_name);
    void *native_handle = NULL;

    if(jerry_get_object_native_pointer(obj, &native_handle, &emitter_type_info)) {
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Enter js_add_event_listener func, %s, true\n", event_name);
    }
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Enter js_add_event_listener func, %s, native_handle: %d\n", event_name, (int)native_handle);
    if (native_handle != NULL)
    {
        struct js_emitter *emitter = (struct js_emitter *)native_handle;
        struct js_event *event = NULL;
        struct js_listener *listener = NULL;
        
        event = find_event(emitter, event_name);
        if (!event)
        {
            event = (struct js_event *)malloc(sizeof(struct js_event));
            if (!event)
            {
                return;
            }

            event->next = NULL;
            event->listeners = NULL;
            event->name = strdup(event_name);
            append_event((struct js_emitter *)native_handle, event);
        }

        listener = (struct js_listener *)malloc(sizeof(struct js_listener));
        if (!listener)
        {
            return;
        }

        // listener->func = jerry_acquire_value(func);
        listener->func = func;
        listener->next = NULL;

        append_listener(event, listener);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Add event[%s] listener\n", event_name);
    }
}

void js_remove_event_listener(jerry_value_t obj, const char *event_name) {
    void *native_handle = NULL;
    
    jerry_get_object_native_pointer(obj, &native_handle, &emitter_type_info);
    if (native_handle) {
        struct js_emitter *emitter = (struct js_emitter *)native_handle;
        struct js_event *event = find_event(emitter, event_name);
        if (event) {
            struct js_listener *_listener, *listener = event->listeners;

            while (listener != NULL) {
                _listener = listener;
                listener = listener->next;
                free_listener(_listener);
            }

            event->listeners = NULL;
        }
    }
}

BaseType_t js_emit_event(jerry_value_t obj, const char *event_name, const jerry_value_t argv[], const jerry_length_t argc) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js emit event, %s\n",event_name);

    void *native_handle = NULL;

    jerry_get_object_native_pointer(obj, &native_handle, &emitter_type_info);
    if (native_handle) {
        struct js_emitter *emitter = (struct js_emitter *)native_handle;
        struct js_event *event = find_event(emitter, event_name);
        if (event) {
            struct js_listener *listener = event->listeners;

            while (listener) {
                jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Call 'js call funtion'\n");
                jerry_value_t ret = js_call_func_obj(listener->func, obj, argv, argc);
                if (jerry_value_is_error(ret)) {
                    js_value_dump(obj);
                    jerry_port_log(JERRY_LOG_LEVEL_ERROR, "event [%s] calling listener error!\n", event_name);
                }
                jerry_release_value(ret);
                listener = listener->next;
            }

            return pdPASS;
        }
    }
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js emit event, %s, can't find\n",event_name);
    return pdFAIL;
}

DECLARE_HANDLER(add_listener)
{
    if (args_cnt == 2)
    {
        char *name = js_value_to_string(args[0]);
        if (name)
        {
            js_add_event_listener(this_value, name, args[1]);
        }

        free(name);
    }

    return jerry_acquire_value(this_value);
}

DECLARE_HANDLER(remove_listener)
{
    if (args_cnt == 2)
    {
        char *name = js_value_to_string(args[0]);
        if (name)
        {
            void *native_handle = NULL;

            jerry_get_object_native_pointer(this_value, &native_handle, &emitter_type_info);
            if (native_handle)
            {
                struct js_emitter *emitter = (struct js_emitter *)native_handle;
                struct js_event *event = find_event(emitter, name);
                if (event)
                {
                    struct js_listener *listener = find_listener(event, args[1]);
                    if (listener)
                    {
                        remove_listener(event, listener);
                        free_listener(listener);
                    }
                }
            }
        }
        free(name);
    }

    return jerry_acquire_value(this_value);
}

DECLARE_HANDLER(remove_all_listeners)
{
    if (args_cnt == 1)
    {
        char *name = js_value_to_string(args[0]);
        if (name)
        {
            js_remove_event_listener(this_value, name);
        }
        free(name);
    }

    return jerry_acquire_value(this_value);
}

DECLARE_HANDLER(remove_event)
{
    if (args_cnt == 1)
    {
        char *name = js_value_to_string(args[0]);
        if (name)
        {
            void *native_handle = NULL;

            jerry_get_object_native_pointer(this_value, &native_handle, &emitter_type_info);
            if (native_handle)
            {
                struct js_emitter *emitter = (struct js_emitter *)native_handle;
                struct js_event *event = find_event(emitter, name);
                if (event)
                {
                    remove_event(emitter, event);
                }
            }
        }

        free(name);
    }

    return jerry_acquire_value(this_value);
}

DECLARE_HANDLER(emit_event)
{
    BaseType_t ret = pdFAIL;

    if (args_cnt >= 1)
    {
        char *name = js_value_to_string(args[0]);
        if (name)
        {
            ret = js_emit_event(this_value, name, args + 1, args_cnt - 1);
        }

        free(name);
    }

    return jerry_create_boolean(ret);
}

DECLARE_HANDLER(get_event_names)
{
    void *native_handle = NULL;

    jerry_get_object_native_pointer(this_value, &native_handle, &emitter_type_info);
    if (native_handle)
    {
        struct js_emitter *emitter = (struct js_emitter *)native_handle;
        struct js_event *event = emitter->events;
        uint32_t index = 0;
        jerry_value_t ret = 0;

        while (event)
        {
            index ++;
            event = event->next;
        }

        ret = jerry_create_array(index);
        if (ret)
        {
            event = emitter->events;
            index = 0;
            while (event)
            {
                jerry_set_property_by_index(ret, index++, jerry_create_string((jerry_char_t *)event->name));
                event = event->next;
            }

            return ret;
        }
    }

    return jerry_create_undefined();
}

void js_destroy_emitter(jerry_value_t obj)
{
    void *native_handle = NULL;

    jerry_get_object_native_pointer(obj, &native_handle, &emitter_type_info);
    if (native_handle)
    {
        struct js_emitter *emitter = (struct js_emitter *)native_handle;
        struct js_event *_event,  *event = emitter->events;

        while (event)
        {
            _event = event;
            event = event->next;

            remove_event(emitter, _event);
        }

        emitter->events = NULL;
    }
}

DECLARE_HANDLER(destroy)
{
    js_destroy_emitter(this_value);
 
    return jerry_create_undefined();
}

// 将成员函数挂载到 _js_emitter_prototype 全局对象
static void js_event_init_prototype(void)
{
    if (_js_emitter_prototype == 0)
    {
        _js_emitter_prototype = jerry_create_object();

        REGISTER_METHOD_NAME(_js_emitter_prototype, "on", add_listener);
        REGISTER_METHOD_NAME(_js_emitter_prototype, "addListener", add_listener);
        REGISTER_METHOD_NAME(_js_emitter_prototype, "emit", emit_event);
        REGISTER_METHOD_NAME(_js_emitter_prototype, "removeListener", remove_listener);
        REGISTER_METHOD_NAME(_js_emitter_prototype, "removeEvent", remove_event);
        REGISTER_METHOD_NAME(_js_emitter_prototype, "removeAllListeners", remove_all_listeners);
        REGISTER_METHOD_NAME(_js_emitter_prototype, "eventNames", get_event_names);
        REGISTER_METHOD(_js_emitter_prototype, destroy);

        jerry_set_object_native_pointer(_js_emitter_prototype, NULL, &event_proto_type_info);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Create emitter prototype\n");
    }
}

// 为 obj 对象设置 emitter prototype(相当于父类？)
void js_make_emitter(jerry_value_t obj)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"mnode_event, js_make_emitter\n");
    struct js_emitter *emitter = NULL;

    js_event_init_prototype();
    jerry_set_prototype(obj, _js_emitter_prototype);
    emitter = malloc(sizeof(struct js_emitter));
    if (emitter)
    {
        emitter->events = NULL;
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Jerry set native pointer\n");
        jerry_set_object_native_pointer(obj, emitter, &emitter_type_info);
    }
}

DECLARE_HANDLER(Event)
{
    if (emitter != 0)
        return jerry_acquire_value(emitter);

    emitter = jerry_create_object();
    js_make_emitter(emitter);
    return jerry_acquire_value(emitter);
}

int js_event_init(void)
{
    REGISTER_HANDLER(Event);
    return 0;
}

int js_event_deinit(void)
{
    if (emitter != 0)
    {
        js_destroy_emitter(emitter);
        jerry_release_value(emitter);
        emitter = 0;
    }

    if (_js_emitter_prototype != 0)
        jerry_release_value(_js_emitter_prototype);

    return 0;
}
