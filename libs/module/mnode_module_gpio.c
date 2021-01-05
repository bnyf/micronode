#include "mnode_module_gpio.h"

static jerry_value_t open_handler(const jerry_value_t func_value, /**< function object >*/
                                  const jerry_value_t this_value, /**< this arg >*/
                                  const jerry_value_t args[],     /**< function arguments >*/
                                  const jerry_length_t args_cnt)  /**< number of function arguments >*/
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"open_handler\n");
  int pin_num = (int)jerry_get_number_value(args[0]);
  int pin_mode = (int)jerry_get_number_value(args[1]);
  mnode_status_e status = mnode_port_set_gpio_mode(pin_num, pin_mode);
  if (status != MNODE_OK)
  {
    jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_set_mode error!\n");
    return jerry_create_boolean(false);
  }
  return jerry_create_boolean(true);
}

static jerry_value_t close_handler(const jerry_value_t func_value, /**< function object >*/
                                  const jerry_value_t this_value, /**< this arg >*/
                                  const jerry_value_t args[],     /**< function arguments >*/
                                  const jerry_length_t args_cnt)  /**< number of function arguments >*/
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"close_handler\n");
  int pin_num = (int)jerry_get_number_value(args[0]);
  mnode_status_e status = mnode_port_close_gpio(pin_num);
  if (status != MNODE_OK)
  {
    jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_close error!\n");
    return jerry_create_boolean(false);
  }
  return jerry_create_boolean(true);
}

static jerry_value_t writeSync_handler(const jerry_value_t func_value, /**< function object */
                                   const jerry_value_t this_value, /**< this arg */
                                   const jerry_value_t args[],     /**< function arguments */
                                   const jerry_length_t args_cnt)  /**< number of function arguments */
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"writeSync_handler\n");
  int pin_num = (int)jerry_get_number_value(args[0]);
  uint32_t pin_level = (uint32_t)jerry_get_number_value(args[1]);
  mnode_status_e status = mnode_port_gpio_set_level(pin_num, pin_level);
  if (status != MNODE_OK)
  {
    jerry_port_log(JERRY_LOG_LEVEL_TRACE, "gpio_set_level error!\n");
    return jerry_create_boolean(false);
  }
  return jerry_create_boolean(true);
}

static jerry_value_t readSync_handler(const jerry_value_t func_value, /**< function object */
                                  const jerry_value_t this_value, /**< this arg */
                                  const jerry_value_t args[],     /**< function arguments */
                                  const jerry_length_t args_cnt)  /**< number of function arguments */
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"readSync_handler\n");
  int pin_num = (int)jerry_get_number_value(args[0]);
  int level = mnode_port_gpio_get_level(pin_num);
  return jerry_create_number((double)level);
}

jerry_value_t mnode_init_gpio()
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"mnode_init_gpio\n");

  jerry_value_t gpio = jerry_create_object();
  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"open");
  jerry_value_t value = jerry_create_external_function(open_handler);
  jerry_release_value(jerry_set_property(gpio, prop_name, value));
  jerry_release_value(prop_name);
  jerry_release_value(value);

  prop_name = jerry_create_string((const jerry_char_t *)"close");
  value = jerry_create_external_function(close_handler);
  jerry_release_value(jerry_set_property(gpio, prop_name, value));
  jerry_release_value(prop_name);
  jerry_release_value(value);

  prop_name = jerry_create_string((const jerry_char_t *)"writeSync");
  value = jerry_create_external_function(writeSync_handler);
  jerry_release_value(jerry_set_property(gpio, prop_name, value));
  jerry_release_value(prop_name);
  jerry_release_value(value);

  prop_name = jerry_create_string((const jerry_char_t *)"readSync");
  value = jerry_create_external_function(readSync_handler);
  jerry_release_value(jerry_set_property(gpio, prop_name, value));
  jerry_release_value(prop_name);
  jerry_release_value(value);

  return gpio;
}