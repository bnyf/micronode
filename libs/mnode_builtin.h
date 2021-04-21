#ifndef MNODE_BUILTIN_H
#define MNODE_BUILTIN_H

#include "mnode_utils.h"

typedef jerry_value_t (*module_register_fn)();
typedef struct
{
    const char *name;
    module_register_fn register_handlder;
} mnode_builtin_module_t;

/* buildin modules list */
extern jerry_value_t mnode_init_gpio(void);
extern jerry_value_t _jerry_request_init(void);
extern jerry_value_t mnode_init_wifi(void);
extern jerry_value_t mnode_init_time(void);
const mnode_builtin_module_t mnode_builtin_module[] = {
    {"gpio", mnode_init_gpio},
    {"http", _jerry_request_init},
    {"wifi", mnode_init_wifi}
};
/* *** */

typedef struct
{
    jerry_value_t jmodule;
} mnode_builtin_objects_t;

/* *** */

const int mnode_builtin_modules_count = sizeof(mnode_builtin_module) / sizeof(mnode_builtin_module_t);

mnode_builtin_objects_t mnode_module_objects[sizeof(mnode_builtin_module) / sizeof(mnode_builtin_module_t)];

jerry_value_t mnode_get_builtin_module(const char *name);

#endif
