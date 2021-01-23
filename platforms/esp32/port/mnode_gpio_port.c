#include "mnode_port.h"
#include "mnode_callback.h"
#include "driver/gpio.h"
#include "jerryscript-port.h"

BaseType_t mnode_port_set_gpio(int pin_num, int pin_mode, int pull, int intr) { 
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"gpio mode: %d, %d\n",pin_num, pin_mode);
    
    gpio_config_t io_conf;

    io_conf.pin_bit_mask = (1ULL << pin_num);
    io_conf.mode = GPIO_MODE_DEF_DISABLE;
    if(pin_mode == 1) {
        io_conf.mode = GPIO_MODE_OUTPUT;
    } else if(pin_mode == 2) {
        io_conf.mode = GPIO_MODE_INPUT;
    } else if(pin_mode == 3) {
        io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    }

    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    if(pull == 1) {
        io_conf.pull_up_en = 1;
    } else if(pull == 2) {
        io_conf.pull_down_en = 1; 
    }
    io_conf.intr_type = intr;
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"intr_type: %d\n",io_conf.intr_type);
    
    if(intr != 0) {
        gpio_install_isr_service(0);
    }
    //configure GPIO with the given settings
    if(gpio_config(&io_conf) == ESP_OK) {
        return pdPASS;
    }
    else {
        return pdFAIL;
    }
}

BaseType_t mnode_port_reset_gpio(int pin_num) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"gpio close: %d\n",pin_num);
    gpio_uninstall_isr_service();
    if(gpio_reset_pin(pin_num) == ESP_OK) {
        return pdPASS;
    } else {
        return pdFAIL;
    }
}

BaseType_t mnode_port_gpio_set_level_sync(int pin_num, int pin_level) {
    jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"gpio set: %d, %d\n",pin_num, pin_level);
    if(gpio_set_level(pin_num, pin_level) == ESP_OK) {
        return pdPASS;
    } else {
        return pdFAIL;
    }
}

int mnode_port_gpio_get_level_sync(int pin_num) {
    return gpio_get_level(pin_num);
}

// BaseType_t mnode_port_gpio_install_intr(int pin_num, jerry_value_t cb) {
//     if(gpio_isr_handler_add(pin_num, js_add_callback, (void*)cb) == ESP_OK) {
//         return pdPASS;
//     } else {
//         return pdFAIL;
//     }
// }
