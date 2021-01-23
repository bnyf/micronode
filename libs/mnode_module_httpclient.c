#include "mnode_module_httpclient.h"
#include "freertos/task.h"
#include "stdlib.h"
#include "string.h"

static const char *TAG = "mnode_module_httpclient";
#ifdef ESP32_HTTP_PKG

void request_callback_func(const void *args, uint32_t size) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"%s, modulerequest_callback_func\n",TAG);
    request_cbinfo_t *cb_info = (request_cbinfo_t *)args;
    if (cb_info->return_value != (jerry_value_t)NULL) {
        js_emit_event(cb_info->target_value, cb_info->callback_name, &(cb_info->return_value), 1);
    } else {
        js_emit_event(cb_info->target_value, cb_info->callback_name, NULL, 0);
    }
    if (cb_info->return_value != (jerry_value_t)NULL) {
        jerry_release_value(cb_info->return_value);
    }
    if (cb_info->data_value != (jerry_value_t)NULL) {
        jerry_release_value(cb_info->data_value);
    }
    if (cb_info->callback_name != NULL) {
        free(cb_info->callback_name);
    }
    free(cb_info);
}

void request_callback_free(const void *args, uint32_t size)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"%s, request_callback_free\n",TAG);
    request_tdinfo_t *rp = (request_tdinfo_t *)args;
    if (rp->config)
    {
        if (rp->config->esp_http_client)
        {
            esp_http_client_cleanup(rp->config->esp_http_client);
        }
        if (rp->config->url)
        {
            free(rp->config->url);
        }
        if (rp->config->data)
        {
            free(rp->config->data);
        }
        free(rp->config);
    }
    js_remove_callback(rp->request_callback);
    js_remove_callback(rp->close_callback);
    js_destroy_emitter(rp->target_value);
    jerry_release_value(rp->target_value);
    free(rp);
}

BaseType_t request_combine_header(esp_http_client_handle_t esp_http_client, char *host, char *user_agent, char *content_type)
{
    BaseType_t ret = pdPASS;
    if (host != NULL)
    {
        esp_http_client_set_header(esp_http_client, "Host", host);
        free(host);
        ret = pdPASS;
    }
    if (user_agent != NULL)
    {
        esp_http_client_set_header(esp_http_client, "User-Agent", user_agent);
        free(user_agent);
        ret = pdPASS;
    }
    if (content_type != NULL)
    {
        esp_http_client_set_header(esp_http_client, "Content-Type", content_type);
        free(content_type);
        ret = pdPASS;
    }
    return ret;
}

// void request_create_header(esp_http_client_handle_t esp_http_client, jerry_value_t header_value)
// {
//     int enter_index = 0;
//     int colon_index = 0;
//     int per_enter_index = -1;
//     char header_type[256];
//     char header_info[256];
    
//     for (int i = 0 ; i < esp_http_client ; i++)
//     {
//         if (session->header->buffer[i] == ':' && per_enter_index != enter_index)
//         {
//             colon_index = i;
//             per_enter_index = enter_index;
//             memset(header_type, 0, 64);
//             memset(header_info, 0, 128);
//             strncpy(header_type, session->header->buffer + enter_index + 1, colon_index - enter_index - 1);
//             strcpy(header_info, session->header->buffer + colon_index + 2);
//             jerry_value_t header_info_value = jerry_create_string((jerry_char_t *)header_info);
//             js_set_property(header_value, header_type, header_info_value);
//             jerry_release_value(header_info_value);
//         }
//         if (session->header->buffer[i] == '\0')
//         {
//             enter_index = i;
//         }
//     }
// }

BaseType_t request_get_header(esp_http_client_handle_t esp_http_client, jerry_value_t header_value)
{
    if (jerry_value_is_object(header_value))
    {
        char *host = NULL, *user_agent = NULL, *content_type = NULL;

        jerry_value_t Host_value = js_get_property(header_value, "Host");
        if (!jerry_value_is_undefined(Host_value))
        {
            host = js_value_to_string(Host_value);
        }
        jerry_release_value(Host_value);

        jerry_value_t User_Agent_value = js_get_property(header_value, "User-Agent");
        if (!jerry_value_is_undefined(User_Agent_value))
        {
            user_agent = js_value_to_string(User_Agent_value);
        }
        jerry_release_value(User_Agent_value);

        jerry_value_t Content_Type_value = js_get_property(header_value, "Content-Type");
        if (!jerry_value_is_undefined(Content_Type_value))
        {
            content_type = js_value_to_string(Content_Type_value);
        }
        jerry_release_value(Content_Type_value);

        return request_combine_header(esp_http_client, host, user_agent, content_type);
    }
    else
    {
        return pdFAIL;
    }
}

void request_read_entry(void *p)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "Enter request_read_entry task\n");
    request_tdinfo_t *rp = (request_tdinfo_t *)p;
    int content_length = 0;
    if (rp->config->esp_http_client != NULL)
    {
        esp_err_t err = ESP_FAIL;
        if (rp->config->method == HTTP_METHOD_GET)
        {   
           err = esp_http_client_open(rp->config->esp_http_client, 0);
           jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "Open http client\n");
        }
        else {
            char buffer[10];
            itoa(strlen(rp->config->data), buffer, 10);
            esp_http_client_set_header(rp->config->esp_http_client, "Content-Length", buffer);
            esp_http_client_set_header(rp->config->esp_http_client, "Content-Type", "application/octet-stream");
        }        
        if(err == ESP_OK) {
            content_length = esp_http_client_fetch_headers(rp->config->esp_http_client);
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Content_length: %d\n",content_length);
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "HTTP GET Status = %d, content_length = %d\n",
                esp_http_client_get_status_code(rp->config->esp_http_client),
                esp_http_client_get_content_length(rp->config->esp_http_client));
        }
    }

    free(rp->config->data);
    rp->config->data = NULL;
    free(rp->config->url);
    rp->config->url = NULL;

    rp->config->response = esp_http_client_get_status_code(rp->config->esp_http_client);
    if (rp->config->esp_http_client == NULL || rp->config->response != 200)
    {
        //do fail callback
        request_cbinfo_t *fail_info  = (request_cbinfo_t *)malloc(sizeof(request_cbinfo_t));
        fail_info->target_value = rp->target_value;
        fail_info->callback_name = strdup("fail");
        fail_info->return_value = (jerry_value_t)NULL;
        fail_info->data_value = (jerry_value_t)NULL;
        js_send_callback(rp->request_callback, fail_info, sizeof(request_cbinfo_t));
        free(fail_info);

        //do complete callback
        request_cbinfo_t *complete_info  = (request_cbinfo_t *)malloc(sizeof(request_cbinfo_t));;
        complete_info->target_value = rp->target_value;
        complete_info->callback_name = strdup("complete");
        complete_info->return_value = (jerry_value_t)NULL;
        complete_info->data_value = (jerry_value_t)NULL;
        js_send_callback(rp->request_callback, complete_info, sizeof(request_cbinfo_t));
        free(complete_info);
    }
    else {
        unsigned char *buffer = NULL;
        buffer = ( unsigned char *)malloc(READ_MAX_SIZE + 1);
        if (!buffer)
        {
            jerry_port_log(JERRY_LOG_LEVEL_ERROR, "no more memory to create read buffer\n");
            return;
        }
        memset(buffer, 0, READ_MAX_SIZE + 1);
        
        int data_read = 0;
        if (content_length < 0) {
            jerry_port_log(JERRY_LOG_LEVEL_ERROR, "HTTP client fetch headers failed");
        } else {
            data_read = esp_http_client_read(rp->config->esp_http_client, (char *)buffer, READ_MAX_SIZE);
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"data_read: %d\n",data_read);
            if (data_read >= 0) {
                jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "HTTP GET Status = %d, content_length = %d\n",
                esp_http_client_get_status_code(rp->config->esp_http_client),
                esp_http_client_get_content_length(rp->config->esp_http_client));
                jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "buffer and size: %s\n%d\n", (char *)buffer, data_read);
            } else {
                jerry_port_log(JERRY_LOG_LEVEL_ERROR, "Failed to read response");
            }
        }
    
        //If file's size is bigger than buffer's,
        //give up reading and send a fail callback
        if (content_length < 0 || data_read < content_length)
        {
            request_cbinfo_t *fail_info = (request_cbinfo_t *)malloc(sizeof(request_cbinfo_t));
            fail_info->target_value = rp->target_value;
            fail_info->callback_name = strdup("fail");
            fail_info->return_value = (jerry_value_t)NULL;
            fail_info->data_value = (jerry_value_t)NULL;
            js_send_callback(rp->request_callback, fail_info, sizeof(request_cbinfo_t));
            free(fail_info);
        }
        else
        {
            jerry_value_t return_value = jerry_create_object();

            /*  create the jerry_value_t of res  */
            jerry_value_t data_value = jerry_create_string(buffer);
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "buffer: %s\n", buffer);
            js_set_property(return_value, "data", data_value);
            js_value_dump(data_value);
            jerry_release_value(data_value);

            jerry_value_t statusCode_value = jerry_create_number(rp->config->response);
            js_set_property(return_value, "statusCode", statusCode_value);
            jerry_release_value(statusCode_value);

            /*** header's data saved as Object ***/
            // jerry_value_t header_value = jerry_create_object();
            // request_create_header(rp->config->esp_http_client, header_value);
            // js_set_property(return_value, "header", header_value);
            // jerry_release_value(header_value);

            esp_http_client_close(rp->config->esp_http_client);

            js_value_dump(rp->target_value);
            js_value_dump(return_value);

            //do success callback
            request_cbinfo_t *success_info  = (request_cbinfo_t *)malloc(sizeof(request_cbinfo_t));
            memset(success_info, 0, sizeof(request_cbinfo_t));
            success_info->target_value = rp->target_value;
            success_info->callback_name = strdup("success");
            success_info->return_value = return_value;
            success_info->data_value = data_value;
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js send callback,%d\n",(int)rp->request_callback->function);
            js_send_callback(rp->request_callback, success_info, sizeof(request_cbinfo_t));
            free(success_info);
        }
        free(buffer);
        //do complete callback
        request_cbinfo_t *complete_info  = (request_cbinfo_t *)malloc(sizeof(request_cbinfo_t));;
        complete_info->target_value = rp->target_value;
        complete_info->callback_name = strdup("complete");
        complete_info->return_value = (jerry_value_t)NULL;
        complete_info->data_value = (jerry_value_t)NULL;
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js send callback,%d\n",(int)rp->request_callback->function);
        js_send_callback(rp->request_callback, complete_info, sizeof(request_cbinfo_t));
        free(complete_info);
    }
    
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"Js send callback,%d\n",(int)rp->close_callback->function);
    //create a callback function to free manager and close webClient session
    js_send_callback(rp->close_callback, rp, sizeof(request_tdinfo_t));
    free(rp);

    vTaskDelete(NULL);
}

void requeset_add_event_listener(jerry_value_t js_target, jerry_value_t requestObj) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "Request add event listener\n");
    jerry_value_t success_func = js_get_property(requestObj, "success");
    if (jerry_value_is_function(success_func)) {
        js_add_event_listener(js_target, "success", success_func);
    }
    jerry_release_value(success_func);

    jerry_value_t fail_func = js_get_property(requestObj, "fail");
    if (jerry_value_is_function(fail_func)) {
        js_add_event_listener(js_target, "fail", fail_func);
    }
    jerry_release_value(fail_func);

    jerry_value_t complete_func = js_get_property(requestObj, "complete");
    if (jerry_value_is_function(complete_func)) {
        js_add_event_listener(js_target, "complete", complete_func);
    }
    jerry_release_value(complete_func);
}

void reqeuset_get_config(request_config_t *config, jerry_value_t requestObj)
{
    /*get url*/
    jerry_value_t js_url = js_get_property(requestObj, "url");
    if (jerry_value_is_string(js_url)) {
        config->url = js_value_to_string(js_url);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "js_url: %s\n", config->url);
    }
    jerry_release_value(js_url);

    /*get data*/
    jerry_value_t js_data = js_get_property(requestObj, "data");
    if (jerry_value_is_object(js_data)) {
        jerry_value_t stringified = jerry_json_stringify(js_data);
        config->data = js_value_to_string(stringified);
        jerry_release_value(stringified);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "data: %s\n", config->data);
    } else if (jerry_value_is_string(js_data)) {
        config->data = js_value_to_string(js_data);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "data: %s\n", config->data);
    }
    jerry_release_value(js_data);

    /*get method*/
    jerry_value_t method_value = js_get_property(requestObj, "method");
    if (jerry_value_is_string(method_value)) {
        char *method_str = js_value_to_string(method_value);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "method_str: %s\n", method_str);
        if (method_str) {
            if (strcmp(method_str, "POST") == 0) {
                config->method = HTTP_METHOD_POST;
            }
            free(method_str);
        }
    }
    jerry_release_value(method_value);

    
    esp_http_client_config_t esp_http_config = {
        .url = config->url,
        .method = config->method,
    };
    config->esp_http_client = esp_http_client_init(&esp_http_config);
    if(config->method == HTTP_METHOD_POST) {
        esp_http_client_set_post_field(config->esp_http_client, config->data, sizeof(config->data));
    }

    /* get header */
    jerry_value_t js_header = js_get_property(requestObj, "header");
    request_get_header(config->esp_http_client, js_header);

    jerry_release_value(js_header);
}

DECLARE_HANDLER(request)
{
    if (args_cnt != 1 || !jerry_value_is_object(args[0]))
    {
        return jerry_create_undefined();
    }

    jerry_value_t requestObj = args[0];
    jerry_value_t rqObj = jerry_create_object();
    js_make_emitter(rqObj); //request emitter object
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"request_callback_func addr: %d\n",(int)request_callback_func);
    struct js_callback *request_callback = js_add_callback(request_callback_func);
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"request_callback_free addr: %d\n",(int)request_callback_free);
    struct js_callback *close_callback = js_add_callback(request_callback_free);

    requeset_add_event_listener(rqObj, requestObj); // requestObj, 用户传入的参数对象

    request_config_t *config = (request_config_t *)malloc(sizeof(request_config_t));
    config->method = HTTP_METHOD_GET;
    config->url = NULL;
    config->data = NULL;
    config->response = 0;
    config->esp_http_client = NULL;

    reqeuset_get_config(config, requestObj);

    js_value_dump(requestObj);

    request_tdinfo_t *rp = (request_tdinfo_t *)malloc(sizeof(request_tdinfo_t));
    memset(rp, 0, sizeof(request_tdinfo_t));
    rp->request_callback = request_callback;
    rp->close_callback = close_callback;
    rp->target_value = rqObj;
    rp->config = config;
    TaskHandle_t xHandle = NULL;
    xTaskCreate( request_read_entry, "requestRead", 1024*4, (void *)rp, 1, &xHandle );
    configASSERT( xHandle );
    
    return jerry_acquire_value(rqObj);
}

int jerry_request_init(jerry_value_t obj)
{
    REGISTER_METHOD(obj, request);
    return 0;
}

jerry_value_t _jerry_request_init()
{
    jerry_value_t js_requset = jerry_create_object();
    REGISTER_METHOD_NAME(js_requset, "request", request);
    return (js_requset);
}

// JS_MODULE(http, _jerry_request_init)

#endif
