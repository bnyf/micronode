#ifndef MNODE_BUILTIN_H
#define MNODE_BUILTIN_H

#include "mnode_utils.h"

typedef jerry_value_t (*module_register_fn)();
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

};
/* *** */

typedef struct
{
    jerry_value_t jmodule;
} mnode_builtin_objects_t;

/* *** */

static const int mnode_builtin_modules_count = sizeof(mnode_builtin_module) / sizeof(mnode_builtin_module_t);

mnode_builtin_objects_t mnode_module_objects[sizeof(mnode_builtin_module) / sizeof(mnode_builtin_module_t)];

jerry_value_t mnode_get_builtin_module(const char *name);

#endif
