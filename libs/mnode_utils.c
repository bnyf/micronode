#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "mnode_utils.h"
#include "mnode_event.h"

extern int js_event_init(void);
extern int js_console_init(void);
// extern void js_callback_init(void);
extern void mnode_init_require(void);
extern void mnode_init_time(void);

static void _js_value_dump(jerry_value_t value);

static SemaphoreHandle_t xUtilsMutex;
static js_util_user _user_init = NULL, _user_cleanup = NULL;

void js_set_property(const jerry_value_t obj, const char *name,
                     const jerry_value_t prop)
{
    jerry_value_t str = jerry_create_string((const jerry_char_t *)name);
    jerry_set_property(obj, str, prop);
    jerry_release_value (str);
}

jerry_value_t js_get_property(const jerry_value_t obj, const char *name)
{
    jerry_value_t ret;

    const jerry_value_t str = jerry_create_string ((const jerry_char_t*)name);
    ret = jerry_get_property(obj, str);
    jerry_release_value (str);

    return ret;
}

void js_set_string_property(const jerry_value_t obj, const char *name,
                            char* value)
{
    jerry_value_t str       = jerry_create_string((const jerry_char_t *)name);
    jerry_value_t value_str = jerry_create_string((const jerry_char_t *)value);
    jerry_set_property(obj, str, value_str);
    jerry_release_value (str);
    jerry_release_value (value_str);
}

void js_set_boolean_property(const jerry_value_t obj, const char *name,
                             bool value)
{
    jerry_value_t str = jerry_create_string((const jerry_char_t *)name);
    jerry_value_t value_bool = jerry_create_boolean(value);
    jerry_set_property(obj, str, value_bool);
    jerry_release_value(str);
    jerry_release_value(value_bool);
}

void js_add_function(const jerry_value_t obj, const char *name,
                     jerry_external_handler_t func)
{
    jerry_value_t str = jerry_create_string((const jerry_char_t *)name);
    jerry_value_t jfunc = jerry_create_external_function(func);

    jerry_set_property(obj, str, jfunc);

    jerry_release_value(str);
    jerry_release_value(jfunc);
}

jerry_value_t js_string_to_value(const char *value)
{
    return jerry_create_string((const jerry_char_t *)value);
}

char *js_value_to_string(const jerry_value_t value)
{
    int len;
    char *str;

    len = jerry_get_string_length(value);

    str = (char*)malloc(len + 1);
    if (str)
    {
        jerry_string_to_char_buffer(value, (jerry_char_t*)str, len);
        str[len] = '\0';
    }

    return str;
}

jerry_value_t js_call_func_obj(const jerry_value_t func_obj_val, /**< function object to call */
                               const jerry_value_t this_val, /**< object for 'this' binding */
                               const jerry_value_t args_p[], /**< function's call arguments */
                               jerry_size_t args_count) /**< number of the arguments */
{
    jerry_value_t ret = 0;

    if(js_util_lock()) {
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Jerry call funtion\n");
        ret = jerry_call_function(func_obj_val, this_val, args_p, args_count);
        js_util_unlock();
    }

    return ret;
}

jerry_value_t js_call_function(const jerry_value_t obj, const char *name,
                               const jerry_value_t args[], jerry_size_t args_cnt)
{
    jerry_value_t ret;
    jerry_value_t function = js_get_property(obj, name);

    if (jerry_value_is_function(function))
    {
        ret = js_call_func_obj(function, obj, args, args_cnt);
    }
    else
    {
        ret = jerry_create_null();
    }

    jerry_release_value(function);
    return ret;
}

bool object_dump_foreach(const jerry_value_t property_name,
                         const jerry_value_t property_value, void *user_data_p)
{
    char *str;
    int str_size;
    int *first_property;

    first_property = (int *)user_data_p;

    if (*first_property) *first_property = 0;
    else {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,", ");
    }

    if (jerry_value_is_string(property_name)) {
        str_size = jerry_get_string_size(property_name);
        str = (char*) malloc (str_size + 1);
        configASSERT(str != NULL);

        jerry_string_to_char_buffer(property_name, (jerry_char_t*)str, str_size);
        str[str_size] = '\0';
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"%s : ", str);
        free(str);
    }
    _js_value_dump(property_value);

    return true;
}

static void _js_value_dump(jerry_value_t value)
{
    if (jerry_value_is_undefined(value))
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"undefined");
    }
    else if (jerry_value_is_boolean(value))
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"%s", jerry_get_boolean_value(value)? "true" : "false");
    }
    else if (jerry_value_is_number(value))
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"%f", jerry_get_number_value(value));
    }
    else if (jerry_value_is_null(value))
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"null");
    }
    else if (jerry_value_is_string(value))
    {
        char *str;
        int str_size;

        str_size = jerry_get_string_size(value);
        str = (char*) malloc (str_size + 1);
        configASSERT(str != NULL);

        jerry_string_to_char_buffer(value, (jerry_char_t*)str, str_size);
        str[str_size] = '\0';
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"\"%s\"", str);
        free(str);
    }
    else if (jerry_value_is_promise(value))
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"promise??");
    }
    else if (jerry_value_is_function(value))
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"[function]");
    }
    else if (jerry_value_is_constructor(value))
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"constructor");
    }
    else if (jerry_value_is_array(value))
    {
        int index;
        uint32_t length = jerry_get_array_length(value);
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"[");
        for (index = 0; index < length; index ++)
        {
            jerry_value_t item = jerry_get_property_by_index(value, index);
            _js_value_dump(item);
            jerry_port_log(JERRY_LOG_LEVEL_TRACE,", ");
            jerry_release_value(item);
        }
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"]");
    }
    else if (jerry_value_is_object(value))
    {
        int first_property = 1;
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"{");
        jerry_foreach_object_property(value, object_dump_foreach, &first_property);
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"}");
    }
    else if(jerry_value_is_arraybuffer(value)) {
        uint32_t length = jerry_get_arraybuffer_byte_length(value);
        uint8_t buffer[length+1];
        uint32_t read_length = jerry_arraybuffer_read(value,0,buffer,length);
        if(read_length != length) {
            jerry_port_log(JERRY_LOG_LEVEL_ERROR,"read arraybuffer error!");
        }
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"[");
        for(int i=0;i<length;++i) {
            jerry_port_log(JERRY_LOG_LEVEL_TRACE,"%02X ",buffer[i]);
        }
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"]");
    }
    else
    {
        jerry_port_log(JERRY_LOG_LEVEL_TRACE,"what?");
    }
}

void js_value_dump(jerry_value_t value)
{
    _js_value_dump(value);
    jerry_port_log(JERRY_LOG_LEVEL_TRACE,"\n");
}

int js_util_init(void)
{
    vSemaphoreCreateBinary(xUtilsMutex);
    js_event_init();
    js_console_init();
    mnode_init_require();
    mnode_init_time();
    if (_user_init != NULL)
    {
        _user_init();
    }

    return 0;
}

int js_util_cleanup(void)
{
    js_event_deinit();
    if (_user_cleanup != NULL)
    {
        _user_cleanup();
    }

    return 0;
}

void js_util_user_init(js_util_user func)
{
    _user_init = func;
}

void js_util_user_cleanup(js_util_user func)
{
    _user_cleanup = func;
}

bool js_util_lock(void)
{
    return xSemaphoreTake( xUtilsMutex, portMAX_DELAY );
}

void js_util_unlock(void)
{
    xSemaphoreGive(xUtilsMutex);
}