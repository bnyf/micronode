# 回调函数实现
## 1. 将一个方法`test`添加到JS引擎
1)  首先在`mnode_builtin.h`中添加如下代码
```C
#ifndef MODULE_TEST
#define MODULE_TEST 1
#endif

#if MODULE_TEST != 0
	extern jerry_value_t mnode_init_test(void)
#endif
```
2)  并在`mnode_builtin.h`中`static const mnode_module_t mnode_builtin_module[]` 的数组内添加如下代码
```C
#if MODULE_TEST != 0 
	{'test', mnode_init_test},
#endif
```
3)  在方法的声明和实现的文件`mnode_module_test.c`中最下部添加如下代码
```C
jerry_value_t mnode_init_test()
{
	jerry_value_t js_test = jerry_create_object();
	//三个变量分别是上面声明的js对象，在JS中调用时的名字，方法名
	REGISTER_METHOD_NAME(js_test, 'test', test);
	//如果在本.c文件中还需实现其他方法，如'abc'，只需在本方法内再加一行
	REGISTER_METHOD_NAME(js_test, 'abc', abc);
	return (js_test);
}
```

## 2.实现`test`方法
1) 在`mnode_module_test.c`中声明并实现`test`方法
```C
DECLARE_HANDLER(test)
{
	//判断传入参数为1且传入的时一个JS对象，若不是则返回 ECMA_VALUE_UNDEFINED(72)
	if(args_cnt !=1 || !jerry_value_is_object(args[0]))
	{
		return jerry_create_undefined();
	}
	//声明并创建一个js对象，用于实现回调
	jerry_value_t rqObj = jerry_create_object();
	//声明一个js对象，并将接收到的对象赋值给它
	jerry_value_t requestObj = args[0];
	
	//为rqObj添加事件派发
	js_make_emitter(rqObj);
	
	//声明两个callback函数句柄，用于承接'test_callback_func'和'test_callback_free'函数,两个函数的功能在下节讲解分析
	js_callback_func request_callback = test_callback_func;
	js_callback_func close_callback = test_callback_free;
	
	//添加事件监听器，通过requestObj来获取回调函数的三种状态:success, fail, complete
	request_add_event_listener(rqObj, requestObj);
	
	//声明一个结构体（自定义，用来承接参数）,如test_config_t
	test_config_t *config = (test_config_t)malloc(sizeof(test_config_t));
	memset(config, 0, sizeof(test_config_t));
	//通过给结构体内部参数赋值来承接传入参数并设置一些固定变量
	config->a = ...;
	config->b = ...;
	//也可通过实现一个方法来实现参数承接，如test_get_config(test_config_t a, jerry_value_t b);
	test_get_config(config, requestObj);
	
	//声明一个结构体（半自定义，用来将一些数据和接口传给创建的任务），如test_tdinfo_t
	test_tdinfo_t *rp = (test_tdifnto_t)malloc(sizeof(test_tdinfo_t));
	memset(rp, 0, sizeof(test_tdinfo_t));
	//结构体中需要包含以下几个参数
	rp->request_callback = request_callback;//为回调函数的三种状态注册派发并释放不需要的指针
	rp->close_callback = close_callback;//用于释放传入参数后不需要的指针
	rp->target_value = rqObj;//回调函数对象
	rp->request_value = requestObje;//函数的传入参数
	rp->config = config; //方法需要的配置参数
	
	//创建一个函数句柄，用于创建任务；如想在其他任务中控制创建出的函数，可以创建一个全局变量
	TaskHandle_t xHandle = NULL;
	//开启一个新任务，任务名为"testTest",任务入口test_Test_entry
	xTaskCreate(test_Test_entry,"testTest",1024*48,(void *)rp, 1, &xHandle);
	configASSERT(xHandle);//查看任务是否开启成果，可选
	
	return jerry_acquire_value(rqObj);//将回调对象返回
}
```
2) `test_callback_func`的实现
一般都是可以通用的，只用创建一个。对于部分自定义的结构体或函数，需要按照需求添加或修改本函数
```C
void test_callback_func(const void *args, uint32_t size) 
{
	//声明一个test_cbinfo_t结构体，注意：此结构体与test_tdinfo_t结构体不同，且基本通用
    test_cbinfo_t *cb_info = (test_cbinfo_t *)args;
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
```
3) `test_callback_free`的实现
需要根据自定义的结构体和需求进行修改
```C
void test_callback_free(const void *args, uint32_t size)
{
    test_tdinfo_t *rp = (test_tdinfo_t *)args;
    if (rp->config)
    {
        if (rp->config->a)
        {
            free(rp->config->a);
        }
        if (rp->config->b)
        {
            free(rp->config->b);
        }
        //...
        free(rp->config);
    }
    js_destroy_emitter(rp->target_value);
    jerry_release_value(rp->target_value);
    free(rp);
}
```

## 3.实现`test_Test_entry`任务函数
```C
void test_Test_entry(void *p){
	test_tdinfo_t *rp = (test_tdinfo_t *)p;
	
	//实现所需功能 
	...
	//需要返回的数据的返回方法
	jerry_value_t return_value = jerry_create_object();//创建一个jerry对象
	//需要返回的数据
	char * return_data_1 = "...";
	int return_data_2 = ...;
	//将数据转换为JerryScript的格式，并以属性方式添加到JS的对象中
	jerry_value_t data_value = jerry_create_string(return_bata_1);
	js_set_property(return_value, "return_data_1", datavalue);
	data_value = jerry_create_number(return_data_2);
	js_set_property(return)value, "return_data_2", databalue);
	...
	jerry_release_value(data_value);//在使用完后需要释放data_value
	
	//当需要调用某个回调函数状态时（以success状态举例）
	test_cbinfo_t * success_info = (test_cbinfo_t *)malloc(sizeof(test_cbinfo_t));
	memset(success_info, 0, sizeof(test_tdinfo_t));
	success_info->target_value = rp->target_value;
	success_info->callback_name = strdup("success");
	success_info->return_value = return_value;
	js_send_callback(rp->request_callback, success_info, sizeof(test_cbinfo_t));
	free(success_info);
	
	//在所有回到状态都配置完后
	js_send_callback(rp->close_callback, rp, sizeof(test_tdinfo_t));
	free(rp);
	
	vTaskDelete(NULL);
}
```

## 4.相关结构体说明
1) `test_cbinfo_t`
```C
typedef struct test_callback_info{
	jerry_value_t target_value; //回调对象
	jerry_value_t return_value; //需要返回的对象
	char * callback_name;	//调用的状态名
} test_cbinfo_t;
```
2) `test_tdinfo_t`
```C
typedef struct test_thread_info{
	jerry_value_t target_value;
	jerry_value_t request_value;	//函数传入的对象
	test_config_t *config;		//自定义的配置结构体
	js_callback_func test_callback;
	js_callback_func close_callback; 
} test_tdinfo_t;
```
3) `test_config_t`
```C
///完全自定义，根据需要配置此结构体
tyepdef struct test_config_info{
	int a;
	int b;
	...
} test_config_t;
```