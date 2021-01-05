#ifndef MNODE_GPIO_PORT_H
#define MNODE_GPIO_PORT_H

#include "mnode_port.h"

mnode_status_e mnode_port_set_gpio_mode(int pin_num, int pin_mode);
mnode_status_e mnode_port_close_gpio(int pin_num);
mnode_status_e mnode_port_gpio_set_level(int pin_num, int pin_level);
int mnode_port_gpio_get_level(int pin_num);

#endif
