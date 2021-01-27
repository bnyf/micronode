#include "mnode_port.h"

static jerry_value_t open_handler(const jerry_value_t func_value, /**< function object >*/
                                  const jerry_value_t this_value, /**< this arg >*/
                                  const jerry_value_t args[],     /**< function arguments >*/
                                  const jerry_length_t args_cnt)  /**< number of function arguments >*/
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"open_handler\n");
  int pin_num = (int)jerry_get_number_value(args[0]);
  int pin_mode = (int)jerry_get_number_value(args[1]);
  int pull = 0;
  int intr = 0;
  if(pin_mode == 2 || pin_mode == 3) {
    pull = (int)jerry_get_number_value(args[2]);
    intr = (int)jerry_get_number_value(args[3]);
  }
  
  BaseType_t status = mnode_port_set_gpio(pin_num, pin_mode, pull, intr);
  if (status != pdPASS)
  {
    jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_set_mode error!\n");
    return jerry_create_boolean(false);
  }
  return jerry_create_boolean(true);
}

// static jerry_value_t reset_handler(const jerry_value_t func_value, /**< function object >*/
//                                   const jerry_value_t this_value, /**< this arg >*/
//                                   const jerry_value_t args[],     /**< function arguments >*/
//                                   const jerry_length_t args_cnt)  /**< number of function arguments >*/
// {
//   jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"close_handler\n");
//   int pin_num = (int)jerry_get_number_value(args[0]);
//   BaseType_t status = mnode_port_reset_gpio(pin_num);
//   if (status != pdPASS)
//   {
//     jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_close error!\n");
//     return jerry_create_boolean(false);
//   }
//   return jerry_create_boolean(true);
// }

static jerry_value_t writeSync_handler(const jerry_value_t func_value, /**< function object */
                                   const jerry_value_t this_value, /**< this arg */
                                   const jerry_value_t args[],     /**< function arguments */
                                   const jerry_length_t args_cnt)  /**< number of function arguments */
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"writeSync_handler\n");
  int pin_num = (int)jerry_get_number_value(args[0]);
  uint32_t pin_level = (uint32_t)jerry_get_number_value(args[1]);
  BaseType_t status = mnode_port_gpio_set_level_sync(pin_num, pin_level);
  if (status != pdPASS)
  {
    jerry_port_log(JERRY_LOG_LEVEL_ERROR, "gpio_set_level error!\n");
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
  int level = mnode_port_gpio_get_level_sync(pin_num);

  return jerry_create_number((double)level);
}

// static jerry_value_t installIntr_handler(const jerry_value_t func_value, /**< function object */
//                                   const jerry_value_t this_value, /**< this arg */
//                                   const jerry_value_t args[],     /**< function arguments */
//                                   const jerry_length_t args_cnt)  /**< number of function arguments */
// {
//   jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "readAsync_handler\n");
//   int pin_num = (int)jerry_get_number_value(args[0]);
//   jerry_value_t cb = args[1];
//   if(!jerry_value_is_function(cb)) {
//     jerry_port_log(JERRY_LOG_LEVEL_ERROR, "readAsync args 2 must a func!\n");
//     jerry_create_boolean(false); 
//   }

//   mnode_port_gpio_install_intr(pin_num, cb);

//   return jerry_create_boolean(true);
// }

jerry_value_t mnode_init_gpio()
{
  jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"mnode_init_gpio\n");

  jerry_value_t gpio = jerry_create_object();
  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"open");
  jerry_value_t value = jerry_create_external_function(open_handler);
  jerry_release_value(jerry_set_property(gpio, prop_name, value));
  jerry_release_value(prop_name);
  jerry_release_value(value);

  // prop_name = jerry_create_string((const jerry_char_t *)"reset");
  // value = jerry_create_external_function(reset_handler);
  // jerry_release_value(jerry_set_property(gpio, prop_name, value));
  // jerry_release_value(prop_name);
  // jerry_release_value(value);

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

  // prop_name = jerry_create_string((const jerry_char_t *)"installIntr");
  // value = jerry_create_external_function(installIntr_handler);
  // jerry_release_value(jerry_set_property(gpio, prop_name, value));
  // jerry_release_value(prop_name);
  // jerry_release_value(value);

  return gpio;
}