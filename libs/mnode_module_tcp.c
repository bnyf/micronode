#include "mnode_module_tcp.h"

static const char *TAG = "MNODE_TCP";

#define SEND 0
#define MaxLength 1024


void tcp_callback_func(const void *args, uint32_t size)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "%s, moduletcp_callback_func\n", TAG);
    tcp_cbinfo_t *cb_info = (tcp_cbinfo_t *)args;
    if (cb_info->return_value != (jerry_value_t)NULL)
    {
        js_emit_event(cb_info->target_value, cb_info->callback_name, &(cb_info->return_value), 1);
    }
    else
    {
        js_emit_event(cb_info->target_value, cb_info->callback_name, NULL, 0);
    }
    //release or free the elements in args
    if (cb_info->return_value != (jerry_value_t)NULL)
    {
        jerry_release_value(cb_info->return_value);
    }
    if (cb_info->data_value != (jerry_value_t)NULL)
    {
        jerry_release_value(cb_info->data_value);
    }
    if (cb_info->callback_name != NULL)
    {
        free(cb_info->callback_name);
    }
    free(cb_info);
}

void tcp_add_event_listener(jerry_value_t js_target, jerry_value_t tcpObj)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "Tcp add event listener\n");
    jerry_value_t success_func = js_get_property(tcpObj, "success");
    if (jerry_value_is_function(success_func))
    {
        js_add_event_listener(js_target, "success", success_func);
    }
    jerry_release_value(success_func);

    jerry_value_t fail_func = js_get_property(tcpObj, "fail");
    if (jerry_value_is_function(fail_func))
    {
        js_add_event_listener(js_target, "fail", fail_func);
    }
    jerry_release_value(fail_func);

    jerry_value_t complete_func = js_get_property(tcpObj, "complete");
    if (jerry_value_is_function(complete_func))
    {
        js_add_event_listener(js_target, "complete", complete_func);
    }
    jerry_release_value(complete_func);
}

void tcp_callback_free_client(const void *args, uint32_t size)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "%s, tcp_callback_free_client\n", TAG);
    tcp_clientinfo_t *rp = (tcp_clientinfo_t *)args;
    if (rp->config)
    {
        if (rp->config->ip)
        {
            c_debug(1,1);
            free(rp->config->ip);
        }
        if (rp->config->message)
        {
            c_debug(2,1);
           // free(rp->config->message);
        }
        if (rp->config->port)
        {
            c_debug(3,1);
            free(rp->config->port);
        }
        c_debug(4,1);
        free(rp->config);
    }
    js_destroy_emitter(rp->target_value);
    jerry_release_value(rp->target_value);
    free(rp);
}

void tcp_get_recv_config(tcp_client_recv_t *config, jerry_value_t tcpObj)
{
    /* get ip*/
    jerry_value_t js_ip = js_get_property(tcpObj, "ip");
    if(jerry_value_is_string(js_ip)){
        config->ip = js_value_to_string(js_ip);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"js_ip: %s\n", config->ip);
#ifdef RECV_DEBUG
    printf("js_ip : %s\n",config->ip);
#endif
    }
    jerry_release_value(js_ip);


    /* get port*/
    jerry_value_t js_port = js_get_property(tcpObj, "port");
    if(jerry_value_is_string(js_port)){
        config->port = js_value_to_string(js_port);
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"js_port: %s\n", config->port);
    }
#ifdef RECV_DEBUG
    printf("js_port : %s\n",config->port);
#endif
    jerry_release_value(js_port);

}



DECLARE_HANDLER(DNSget)
{

    char *straddr;
    char *port;

    const struct addrinfo hints = {
        .ai_family = AF_INET,       //指定返回地址的协议簇，AF_INET(IPv4), AF_INET6(IPv6), AF_UNSPEC(v4 and v6)
        .ai_socktype = SOCK_STREAM, //设定返回地址的socket类型，流式套接字
    };
    //Get the length of Realm name and Port
    int straddr_length = jerry_get_string_length(args[0]);
    int port_length = jerry_get_string_length(args[1]);
    //Allocate the memory to straddr and port
    straddr = malloc(straddr_length + 1);
    port = malloc(port_length + 1);
    //Create the JS objection and pending variable
    //Warning: pending_name and pending_value need to be released by jerry_release_value()
    jerry_value_t dnsget = jerry_create_object();
    jerry_value_t pending_name, pending_value;

    jerry_string_to_char_buffer(args[0], (jerry_char_t *)straddr, straddr_length);
    jerry_string_to_char_buffer(args[1], (jerry_char_t *)port, port_length);
    straddr[straddr_length] = 0;
    port[port_length] = 0;
    struct addrinfo *result;
    int err;
    err = getaddrinfo(straddr, port, &hints, &result);
    if (err != 0)
    {
        printf("getaddrinfo err: %d \n", err);
    }

    char buf[20];
    struct sockaddr_in *ipv4 = NULL;
    if (result->ai_family == AF_INET)
    {
        ipv4 = (struct sockaddr_in *)result->ai_addr;
        inet_ntop(result->ai_family, &ipv4->sin_addr, buf, sizeof(buf));
        printf("[ipv4]%s [port]%d \n", buf, ntohs(ipv4->sin_port));
    }
    else
    {
        printf("got IPv4 err !!!\n");
    }

    pending_value = jerry_create_number(((double)ntohs(ipv4->sin_port)));
    pending_name = jerry_create_string((jerry_char_t *)"port");
    jerry_release_value(jerry_set_property(dnsget, pending_name, pending_value));

    pending_name = jerry_create_string((jerry_char_t *)"ip");
    pending_value = jerry_create_string((jerry_char_t *)buf);
    jerry_release_value(jerry_set_property(dnsget, pending_name, pending_value));

    jerry_release_value(pending_value);
    jerry_release_value(pending_name);

    free(straddr);
    free(port);
    freeaddrinfo(result);

    return dnsget;
}

void client_connect_send(void * pd)
{
    //create  socket descriptor to receive opened socket
    int sockfd = -1;
    int ret = -1;
    //the number of repeat send
    int cnt = 5;
    //open a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    tcp_client_send_t *p = (tcp_client_send_t *) pd;  

    if (sockfd == -1)
    {
        printf("socket open failre !\n");
        return jerry_create_undefined();
    }
    else
    {
        struct sockaddr_in seraddr = {0};
        //set sin_family to IPv4
        seraddr.sin_family = AF_INET;
        //set sin_port from the input parameter
        //atoi() can transfor char * to int
        seraddr.sin_port = htons(atoi(p->port));
        //set IP address
        seraddr.sin_addr.s_addr = inet_addr(p->ip);
        ret = connect(sockfd, (const struct sockaddr *)&seraddr, sizeof(seraddr));

        if (ret < 0)
        {
            printf("Connect to server failure !!! ret = %d \n", ret);
        }
        else
        {
            printf("connect success. \n");
            while (cnt--)
            {
                ret = send(sockfd, p->send_context, p->send_context_length, 0);
                if (ret < 0)
                {
                    printf("Send err !! \n");
                }
                else
                {
                    printf("Send success. \n");
                }
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            }
        }
        close(sockfd);
    }

    free(p);
    vTaskDelete(NULL);    
}

DECLARE_HANDLER(client_connect)
{
    char *ipaddr;
    char *port;
    char *send_context;
    int ipaddr_length, port_length, send_context_length;
    tcp_client_send_t *pd = (tcp_client_send_t *)malloc(sizeof(tcp_client_send_t));
    memset(pd,0,sizeof(tcp_client_send_t));

    //get the length of each parameter
    ipaddr_length = jerry_get_string_length(args[0]);
    port_length = jerry_get_string_length(args[1]);
    send_context_length = jerry_get_string_length(args[2]);
    //allocate the memory to each parameter
    ipaddr = malloc(ipaddr_length + 1);
    port = malloc(port_length + 1);
    send_context = malloc(send_context_length + 1);
    //get the contents from input parameter
    jerry_string_to_char_buffer(args[0], (jerry_char_t *)ipaddr, ipaddr_length);
    jerry_string_to_char_buffer(args[1], (jerry_char_t *)port, port_length);
    jerry_string_to_char_buffer(args[2], (jerry_char_t *)send_context, send_context_length);
    ipaddr[ipaddr_length] = 0;
    port[port_length] = 0;
    send_context[send_context_length] = 0;

    pd->ip = ipaddr;
    pd->port = port;
    pd->send_context = send_context;
    pd->send_context_length = send_context_length;

    TaskHandle_t xhandle = NULL;
    xTaskCreate(client_connect_send, "sendContext",1024*24, (void *)pd, 1, &xhandle);
#ifdef REC_DEBUG
    printf("The client_connect_send task has created successfully.\n");
#endif
   // free(pd);
    return jerry_create_undefined();
}


void tcp_recv_entry(void * rp)
{
    //create  socket descriptor to receive opened socket
    int sockfd = -1;
    int ret = -1;
    //open a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    //create a jerry object for return
    tcp_clientinfo_t *p = (tcp_clientinfo_t *)rp;
    jerry_value_t pending_object = jerry_create_object();
    jerry_value_t pending_name, pending_value;

        vTaskDelay(1000/portTICK_PERIOD_MS);
        if(sockfd == -1)
        {
            printf("socket open failre !\n");
            return jerry_create_undefined();
        }
        else
        {
            struct sockaddr_in seraddr = {0};
            //set sin_family to IPv4
            seraddr.sin_family = AF_INET;
            //set sin_port from the input parameter
            //atoi() can transfor char * to int
            seraddr.sin_port = htons(atoi(p->config->port));
            //set IP address
            seraddr.sin_addr.s_addr = inet_addr(p->config->ip);
            ret = connect(sockfd, (const struct sockaddr *)&seraddr, sizeof(seraddr));

            if (ret < 0)
            {
                printf("Connect to server failure !!! ret = %d \n", ret);
            }
            else
            {
                printf("connect success. \n");
                p->config->recv_context_length = read(sockfd, p->config->message, MaxLength-1);
                if(p->config->recv_context_length == -1){
                    printf("read() errror!\n");
                    return jerry_create_undefined();
                } 

                printf("got message from server \n");
                pending_value = jerry_create_string((jerry_char_t *) p->config->message);
                pending_name = jerry_create_string((jerry_char_t *)"recvdata");
                jerry_release_value(jerry_set_property(pending_object, pending_name, pending_value));

                pending_value = jerry_create_number((double) p->config->recv_context_length);
                pending_name = jerry_create_string((jerry_char_t *)"datalen");
                jerry_release_value(jerry_set_property(pending_object, pending_name, pending_value));

                jerry_release_value(pending_name);
                jerry_release_value(pending_value);
                close(sockfd);

                //do success callback
                tcp_cbinfo_t *success_info = (tcp_cbinfo_t *)malloc(sizeof(tcp_cbinfo_t));
                memset(success_info, 0, sizeof(tcp_cbinfo_t));
                success_info->target_value = p->target_value;
                success_info->callback_name = strdup("success");
                success_info->return_value = pending_object;

                js_send_callback(p->tcp_callback, success_info, sizeof(tcp_cbinfo_t));
                free(success_info);
                js_send_callback(p->close_callback,rp,sizeof(tcp_clientinfo_t));
            }
        }
    printf("delete task \n");
    free(p);
    vTaskDelete(NULL);
}

DECLARE_HANDLER(RecvData)
{
    if (args_cnt != 1 || !jerry_value_is_object(args[0]))
    {
        return jerry_create_undefined();
    }

    c_debug(1,RECV_DEBUG);

    char message[1024];
    jerry_value_t rqObj = jerry_create_object();
    jerry_value_t tcpObj = args[0];
    js_callback_func tcp_callback = tcp_callback_func;
    js_callback_func close_callback = tcp_callback_free_client;
    tcp_client_recv_t *pd =(tcp_client_recv_t *)malloc(sizeof(tcp_client_recv_t));
    memset(pd,0,sizeof(tcp_client_recv_t));

    js_make_emitter(rqObj);
    tcp_add_event_listener(rqObj,tcpObj); 
    tcp_get_recv_config(pd,tcpObj);

    c_debug(2,RECV_DEBUG);

    tcp_clientinfo_t * rp =(tcp_clientinfo_t *)malloc(sizeof(tcp_clientinfo_t));
    memset(rp,0,sizeof(tcp_clientinfo_t));
    rp->tcp_callback = tcp_callback;
    rp->config = pd;
    rp->target_value = rqObj;
    rp->tcp_value = tcpObj;
    rp->close_callback = close_callback;

    js_value_dump(tcpObj);

    c_debug(3,RECV_DEBUG);

    TaskHandle_t xHandle = NULL;

    xTaskCreate(tcp_recv_entry, "tcpRecv", 1024*48, (void *)rp, 1, &xHandle);
    configASSERT(xHandle);

    c_debug(4,RECV_DEBUG);

    return jerry_acquire_value(rqObj);
}


void error_handling(char *message)
{
    fputs(message, stderr);
    fputc("\n", stderr);
    exit(1);
}


DECLARE_HANDLER(esp32server)
{
    int serv_sock_fd;
    int clnt_sock_fd;
    int clnt_addr_size;
    char *port_add;
    int port_add_length;

    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_adr_size;

    char send_data[] = "Message from ESP32 server!";
    char * message;

    port_add_length = jerry_get_string_length(args[0]);
    port_add = malloc(port_add_length+1);
    jerry_string_to_char_buffer(args[0], (jerry_char_t *)port_add, port_add_length);
    port_add[port_add_length] = 0;

    serv_sock_fd = socket(AF_INET, SOCK_STREAM,0);
    if (serv_sock_fd == -1)
    {
        error_handling("sock() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(port_add));

    if (bind(serv_sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        error_handling("bind() error");
    }
    if (listen(serv_sock_fd, 5) == -1)
    {
        error_handling("listen() error");
    }

    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock_fd = accept(serv_sock_fd, (struct sockaddr *)&clnt_addr, &clnt_adr_size);
    //while(1){
        int n;
        if(clnt_sock_fd == -1)
        {
               error_handling("accept() error");
        }
        if(SEND == 1)
        {
            n = recv(clnt_sock_fd, message, 100,0);
            if(n == 0){
    //            break;
            }
            message[n] = '\0';
            printf("recv message : %s \n",message);
        }
        else{
            write(clnt_sock_fd, send_data, sizeof(send_data));
            printf("Data have been sent\n");
        }
    //}
    close(clnt_sock_fd);
    close(serv_sock_fd);

    free(port_add);
    
    return jerry_create_undefined();
}


jerry_value_t mnode_init_tcp()
{
    jerry_value_t js_tcp = jerry_create_object();

    REGISTER_METHOD_NAME(js_tcp, "DNSget", DNSget);
    REGISTER_METHOD_NAME(js_tcp, "connect", client_connect);
    REGISTER_METHOD_NAME(js_tcp, "RecvData", RecvData);
    REGISTER_METHOD_NAME(js_tcp, "esp32server", esp32server);

    return (js_tcp);
}