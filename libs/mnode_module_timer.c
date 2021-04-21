#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "clist.h"

#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

#include "mnode_utils.h"

clist_t *timerIdList;
uint8_t timerId = 0;

typedef struct jerry_timer_t
{
    jerry_value_t funcObj;
    uint8_t timerId;
    TimerHandle_t timerHandler;
    bool repeat;
} jerry_timer_t;

static void timeout_callback(TimerHandle_t pxTimer)
{
    list_node_t *node;
    jerry_timer_t *timer = NULL;
    list_iterator_t *it = list_iterator_new(timerIdList, LIST_HEAD);
    while ((node = list_iterator_next(it)))
    {
        timer = (jerry_timer_t *)node->val;
        if (timer->timerId == (uint8_t)pvTimerGetTimerID(pxTimer))
        {
            js_call_func_obj(timer->funcObj, NULL, NULL, 0);
            if (!timer->repeat)
            {
                jerry_release_value(timer->funcObj);
                xTimerStop(timer->timerHandler, NULL);
                xTimerDelete(timer->timerHandler, NULL);
                free(timer);
                list_remove(timerIdList, node);
            }
            break;
        }
    }
    list_iterator_destroy(it);
}

DECLARE_HANDLER(setInterval){
    jerry_value_t func_obj = args[0];
    jerry_value_t interval_obj = args[1];
    TickType_t interval = (TickType_t)jerry_get_number_value(interval_obj);

    if (timerId == 0)
    {
        timerId = 1;
    }
    jerry_timer_t *timer = (jerry_timer_t *)malloc(sizeof(struct jerry_timer_t));
    timer->repeat = true;
    // Acquire types with reference counter (increase the references).
    timer->funcObj = jerry_acquire_value(func_obj);
    timer->timerId = timerId;

    TimerHandle_t timerHandler = xTimerCreate("jtmr", interval / portTICK_PERIOD_MS, pdTRUE, (void *)timer->timerId, timeout_callback);
    timer->timerHandler = timerHandler;

    list_node_t *node = list_node_new(timer);

    list_rpush(timerIdList, node);

    xTimerStart(timerHandler, NULL);

    return jerry_create_number(timerId++);
}

DECLARE_HANDLER(setTimeout)
{
    jerry_value_t func_obj = args[0];
    jerry_value_t timeout_obj = args[1];
    TickType_t timeout = (TickType_t)jerry_get_number_value(timeout_obj);

    if (timerId == 0)
    {
        timerId = 1;
    }
    jerry_timer_t *timer = (jerry_timer_t *)malloc(sizeof(struct jerry_timer_t));
    timer->repeat = false;
    // Acquire types with reference counter (increase the references).
    timer->funcObj = jerry_acquire_value(func_obj);
    timer->timerId = timerId;

    TimerHandle_t timerHandler = xTimerCreate("jtmr", timeout / portTICK_PERIOD_MS, pdFALSE, (void *)timer->timerId, timeout_callback);
    timer->timerHandler = timerHandler;

    list_node_t *node = list_node_new(timer);

    list_rpush(timerIdList, node);

    xTimerStart(timerHandler, NULL);

    return jerry_create_number(timerId++);
}

DECLARE_HANDLER(clearTimer)
{
    jerry_value_t timerId = args[0];
    uint8_t id = (uint8_t)jerry_get_number_value(timerId);

    list_node_t *node;
    jerry_timer_t *timer;
    list_iterator_t *it = list_iterator_new(timerIdList, LIST_HEAD);
    while ((node = list_iterator_next(it)))
    {
        timer = (jerry_timer_t *)node->val;
        if (timer->timerId == id)
        {
            jerry_release_value(timer->funcObj);
            xTimerStop(timer->timerHandler, NULL);
            xTimerDelete(timer->timerHandler, NULL);
            free(timer);
            list_remove(timerIdList, node);
            break;
        }
    }
    list_iterator_destroy(it);
    return jerry_create_undefined();
}

void mnode_init_time()
{
    timerIdList = list_new();

    REGISTER_HANDLER_GLOBAL(setInterval);
    REGISTER_HANDLER_GLOBAL(setTimeout);
    REGISTER_HANDLER_GLOBAL_ALIAS(clearInterval, clearTimer_handler);
    REGISTER_HANDLER_GLOBAL_ALIAS(clearTimeou, clearTimer_handler);
}
