#include "mnode_port.h"
#include "mnode_module_gpio.h"
#include "mnode_callback.h"
#include "mnode_utils.h"
#include "driver/gpio.h"
#include <string.h>

static void gpio_get_config(gpio_config_t *config, int pin_num, jerry_value_t requestObj) {
    /*get pin mode*/
    jerry_value_t pin_mode = js_get_property(requestObj, "mode");
    if (jerry_value_is_string(pin_mode)) {
        char *buf = js_value_to_string(pin_mode);
        if(strcmp(buf, "IN") == 0) {
            config->mode = GPIO_MODE_DEF_INPUT;
        } else if(strcmp(buf, "OUT") == 0) {
            config->mode = GPIO_MODE_DEF_OUTPUT;
        } else if(strcmp(buf, "BOTH") == 0) {
            config->mode = GPIO_MODE_DEF_INPUT | GPIO_MODE_DEF_OUTPUT;
        }
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "js_mode: %s\n", buf);
        free(buf);
    }
    jerry_release_value(pin_mode);

    /*get pull mode*/
    config->pull_up_en = 0;
    config->pull_down_en = 0;
    jerry_value_t pin_pull_mode = js_get_property(requestObj, "pull");
    if (jerry_value_is_string(pin_pull_mode)) {
        char *buf = js_value_to_string(pin_pull_mode);
        if(strcmp(buf, "UP") == 0) {
            config->pull_up_en = 1;
        } else if(strcmp(buf, "DOWN") == 0) {
            config->pull_down_en = 1;
        } else if(strcmp(buf, "BOTH") == 0) {
            config->pull_down_en = 1;
            config->pull_down_en = 1;
        }
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "js_mode: %s\n", buf);
        free(buf);
    }
    jerry_release_value(pin_pull_mode);

    /* intr */
    config->intr_type = GPIO_INTR_DISABLE;
    jerry_value_t pin_intr_mode = js_get_property(requestObj, "intr");
    if (jerry_value_is_string(pin_intr_mode)) {
        char *buf = js_value_to_string(pin_intr_mode);
        if(strcmp(buf, "POS") == 0) {
            config->intr_type = GPIO_INTR_POSEDGE;
        } else if(strcmp(buf, "NEG") == 0) {
            config->intr_type = GPIO_INTR_NEGEDGE;
        } else if(strcmp(buf, "BOTH") == 0) {
            config->intr_type = GPIO_INTR_ANYEDGE;
        }
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "js_mode: %s\n", buf);
        free(buf);
    }
    jerry_release_value(pin_intr_mode);

    if (gpio_config(config) != ESP_OK) {
        jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_set_mode error!\n");
    }

    /* func */
    config->intr_type = GPIO_INTR_DISABLE;
    jerry_value_t intr_func = js_get_property(requestObj, "func");

    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin_num, js_send_callback_from_isr, (void*) intr_func);
}

DECLARE_HANDLER(open) {
    gpio_config_t io_conf;
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "open_handler\n");
    int pin_num = (int)jerry_get_number_value(args[0]);
    gpio_get_config(&io_conf, pin_num, args[1]);

    return jerry_create_boolean(true);
}

static jerry_value_t reset_handler(const jerry_value_t func_value, /**< function object >*/
                                  const jerry_value_t this_value, /**< this arg >*/
                                  const jerry_value_t args[],     /**< function arguments >*/
                                  const jerry_length_t args_cnt)  /**< number of function arguments >*/
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"close_handler\n");
  int pin_num = (int)jerry_get_number_value(args[0]);
  BaseType_t status = gpio_reset_pin(pin_num);
  if (status != pdPASS)
  {
    jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_close error!\n");
    return jerry_create_boolean(false);
  }
  return jerry_create_boolean(true);
}

DECLARE_HANDLER(writeSync)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "writeSync_handler\n");
    int pin_num = (int)jerry_get_number_value(args[0]);
    uint32_t pin_level = (uint32_t)jerry_get_number_value(args[1]);
    BaseType_t status = gpio_set_level(pin_num, pin_level);
    if (status != pdPASS)
    {
        jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_set_level error!\n");
        return jerry_create_boolean(false);
    }
    return jerry_create_boolean(true);
}

DECLARE_HANDLER(readSync)
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "readSync_handler\n");
    int pin_num = (int)jerry_get_number_value(args[0]);
    int level = gpio_get_level(pin_num);

    return jerry_create_number((double)level);
}

jerry_value_t mnode_init_gpio()
{
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "mnode_init_gpio\n");
    jerry_value_t gpio = jerry_create_object();

    REGISTER_METHOD(gpio, open);
    REGISTER_METHOD(gpio, writeSync);
    REGISTER_METHOD(gpio, readSync);
    REGISTER_METHOD(gpio, reset);

    return gpio;
}
