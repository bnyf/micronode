#include "mnode_builtin.h"

#include <string.h>

jerry_value_t mnode_get_builtin_module(const char* name)
{
  for (unsigned i = 0; i < mnode_builtin_modules_count; i++) {
    if (!strcmp(name, mnode_builtin_module[i].name)) {
      if(mnode_module_objects[i].jmodule == 0) {
        jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"cache buildtin module: %d %s\n",i, name);
        mnode_module_objects[i].jmodule = mnode_builtin_module[i].register_handlder();
      }
      jerry_port_log(JERRY_LOG_LEVEL_DEBUG,"get buildtin module: %d\n",mnode_module_objects[i].jmodule);
      return mnode_module_objects[i].jmodule;
    }
  }
  // not a built-in module
  return (jerry_value_t)NULL;
}
