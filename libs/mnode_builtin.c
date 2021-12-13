#include "mnode_builtin.h"

#include <string.h>

jerry_value_t mnode_get_builtin_module(const char *name)
{//mnode_builtin_modules_count is the number of builtin modules, given from mnode_builtin.h
    for (unsigned i = 0; i < mnode_builtin_modules_count; i++)
    {//mnode_builtin_modle contains 'name' and 'register_handler'
        if (!strcmp(name, mnode_builtin_module[i].name))// if name equal a biltin module
        {
            if (mnode_module_objects[i].jmodule == 0)
            {//JERRY_LOG_LEVEL_DUBUG = 2
                jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "cache builtin module: %d %s\n", i, name);
                mnode_module_objects[i].jmodule = mnode_builtin_module[i].register_handlder();
            }
            jerry_port_log(JERRY_LOG_LEVEL_DEBUG, "get buildtin module: %d\n", mnode_module_objects[i].jmodule);
            //return a function handler
            return mnode_module_objects[i].jmodule;
        }
    }
    // not a built-in module
    return (jerry_value_t)NULL;
}
