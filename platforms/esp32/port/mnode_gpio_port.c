#include "mnode_gpio_port.h"
#include "driver/gpio.h"
#include "jerryscript-port.h"

mnode_status_e mnode_port_set_gpio_mode(int pin_num, int pin_mode) { //0:output;1:intput;2:both
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"gpio mode: %d, %d\n",pin_num, pin_mode);
    gpio_config_t io_conf;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL << pin_num);
    if(pin_mode == 0) {
        //set as output mode
        io_conf.mode = GPIO_MODE_OUTPUT;
        //disable pull-down mode
        io_conf.pull_down_en = 0;
        //disable pull-up mode
        io_conf.pull_up_en = 0;
        //disable interrupt
        io_conf.intr_type = GPIO_INTR_DISABLE;  
    } else if(pin_mode == 1) {
        //set as output mode
        io_conf.mode = GPIO_MODE_INPUT;
        //disable pull-down mode
        io_conf.pull_down_en = 0;
        //disable pull-up mode
        io_conf.pull_up_en = 0;
        //disable interrupt
        io_conf.intr_type = GPIO_INTR_DISABLE;
    } else {
        //set as output mode
        io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
        //disable pull-down mode
        io_conf.pull_down_en = 0;
        //disable pull-up mode
        io_conf.pull_up_en = 0;
        //disable interrupt
        io_conf.intr_type = GPIO_INTR_DISABLE;
    }
    //configure GPIO with the given settings
    if(gpio_config(&io_conf) == ESP_OK) {
        return MNODE_OK;
    }
    else {
        return MNODE_ERROR;
    }
}
mnode_status_e mnode_port_close_gpio(int pin_num) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"gpio close: %d\n",pin_num);
    if(gpio_reset_pin(pin_num) == ESP_OK) {
        return MNODE_OK;
    } else {
        return MNODE_ERROR;
    }
}

mnode_status_e mnode_port_gpio_set_level(int pin_num, int pin_level) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"gpio set: %d, %d\n",pin_num, pin_level);
    if(gpio_set_level(pin_num, pin_level) == ESP_OK) {
        return MNODE_OK;
    } else {
        return MNODE_ERROR;
    }
}

int mnode_port_gpio_get_level(int pin_num) {
    return gpio_get_level(pin_num);
}
