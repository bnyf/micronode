#ifndef MNODE_PORT_H
#define MNODE_PORT_H

#include "jerryscript.h"
#include "freertos/FreeRTOS.h"
/*

gpio

pin_num: number
pin_mode: 0 dis, 1 out, 2 in, 3 both
[input mode]pull: 0 dis, 1 pullup, 2 pulldown
[input mode]intr: 0 dis, 1 rising, 2 failing, 3 both

*/

BaseType_t mnode_port_set_gpio(int pin_num, int pin_mode, int pull, int intr);
BaseType_t mnode_port_reset_gpio(int pin_num);
BaseType_t mnode_port_gpio_set_level_sync(int pin_num, int pin_level);
int mnode_port_gpio_get_level_sync(int pin_num);
// BaseType_t mnode_port_gpio_install_intr(int pin_num, jerry_value_t cb);
/* *** */

#endif