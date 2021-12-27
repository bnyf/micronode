#ifndef MNODE_BUILTIN_H
#define MNODE_BUILTIN_H

#include "mnode_utils.h"

typedef jerry_value_t (*module_register_fn)();
/*mnode_builtin_module_t is a struct contains a char pointer 'name' and a function potiner 'register_handler'
whose input parameter is none and return an uint32 parameter */
typedef struct
{
    const char *name;
    module_register_fn register_handlder;
} mnode_builtin_module_t;

#ifndef MODULE_GPIO
#define MODULE_GPIO 1
#endif

#ifndef MODULE_HTTP
#define MODULE_HTTP 1
#endif

#ifndef MODULE_WIFI
#define MODULE_WIFI 1
#endif

#ifndef MODULE_TCP
#define MODULE_TCP 1
#endif

/* buildin modules list */
#if MODULE_GPIO != 0
extern jerry_value_t mnode_init_gpio(void);
#endif
#if MODULE_HTTP != 0
extern jerry_value_t mnode_init_http(void);
#endif
#if MODULE_WIFI != 0
extern jerry_value_t mnode_init_wifi(void);
#endif
#if MODULE_TCP != 0
extern jerry_value_t mnode_init_tcp(void);
#endif

//At present stage, there are 3 module in mnode_builtin_module[], gpio, http and wifi.
static const mnode_builtin_module_t mnode_builtin_module[] = {

#if MODULE_GPIO != 0
    {"gpio", mnode_init_gpio},
#endif

#if MODULE_HTTP != 0
    {"http", mnode_init_http},
#endif

#if MODULE_WIFI != 0
    {"wifi", mnode_init_wifi},
#endif

#if MODULE_TCP != 9
    {"tcp", mnode_init_tcp},
#endif

};
/* *** */

typedef struct
{
    jerry_value_t jmodule;
} mnode_builtin_objects_t;

/* *** */
//mnode_builtin_modules_count is the number of builtin modules
static const int mnode_builtin_modules_count = sizeof(mnode_builtin_module) / sizeof(mnode_builtin_module_t);
//mnode_builtin_objects_t is a struct only contains a uint32 jmodule, and mnode_module_objects seems equal to mnode_builtin_modules_count
mnode_builtin_objects_t mnode_module_objects[sizeof(mnode_builtin_module) / sizeof(mnode_builtin_module_t)];

jerry_value_t mnode_get_builtin_module(const char *name);

#endif
