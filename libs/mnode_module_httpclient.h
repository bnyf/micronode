#ifndef MNODE_MODULE_HTTPCLIENT_H
#define MNODE_MODULE_HTTPCLIENT_H

#include "mnode_utils.h"
#include "mnode_event.h"
#include "mnode_callback.h"
#include "mnode_config.h"
#include "freertos/FreeRTOS.h"

#ifdef ESP32_HTTP_PKG

#include "esp_http_client.h"

#define READ_MAX_SIZE 1024
#define HEADER_BUFSZ 1024

struct request_callback_info
{
    jerry_value_t target_value; // request返回的对象
    jerry_value_t return_value; //  response 对象
    jerry_value_t data_value; // 数据对象
    char *callback_name;
} typedef request_cbinfo_t;

struct request_config_info
{
    char *url;
    char *data;
    int method;
    int response;
    // struct webclient_session *session;
    esp_http_client_handle_t esp_http_client;
} typedef request_config_t;

struct request_thread_info
{
    jerry_value_t target_value; 
    request_config_t *config;
    struct js_callback *request_callback;
    struct js_callback *close_callback;
} typedef request_tdinfo_t;

int jerry_request_init(jerry_value_t obj);
#endif

#endif