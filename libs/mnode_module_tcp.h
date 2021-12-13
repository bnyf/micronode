#ifndef MNODE_MODULE_TCP_H
#define MNODE_MODULE_TCP_H

#include "mnode_builtin.h" 

#include "mnode_utils.h"
#include "mnode_event.h"
#include "mnode_callback.h"
#include "mnode_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "mnode_module_httpclient.h"
#include "stdlib.h"
#include "string.h"
#include "esp_log.h"
#include "mnode_debug.h"

typedef struct tcp_callback_info
{
    jerry_value_t target_value;
    jerry_value_t return_value;
    jerry_value_t data_value;
    char *callback_name;
}tcp_cbinfo_t;

typedef struct tcp_client_send
{
    char * ip;
    char * port;
    char * send_context;
    jerry_value_t send_context_length;
} tcp_client_send_t;


typedef struct tcp_client_recv
{
    char * ip;
    char * port;
    char message[1024] ;
    jerry_value_t recv_context_length;
} tcp_client_recv_t;


typedef struct tcp_client_info
{
    jerry_value_t target_value; 
    jerry_value_t tcp_value;
    tcp_client_recv_t *config;
    js_callback_func tcp_callback;
    js_callback_func close_callback;
} tcp_clientinfo_t;

int jerry_tcp_init(jerry_value_t obj);


#endif