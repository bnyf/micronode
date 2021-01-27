#include <string.h>
#include <stdlib.h>

#include "mnode_utils.h"
#include "jerryscript-ext/handler.h"
#include "jerryscript-ext/handle-scope.h"
#include "jerryscript-port.h"

extern jerry_value_t mnode_get_builtin_module(const char* name);

static jerry_value_t mnode_require_handler(const jerry_value_t func_value, /**< function object >*/
                                     const jerry_value_t this_value, /**< this arg >*/
                                     const jerry_value_t args[],     /**< function arguments >*/
                                     const jerry_length_t args_cnt)  /**< number of function arguments >*/
{
  jerry_size_t strLen = jerry_get_string_size(args[0]);
  jerry_char_t name[strLen + 1];
  jerry_string_to_char_buffer(args[0], name, strLen);
  name[strLen] = '\0';
  //gpio.js
  if(strLen > 3 && strcmp((char *)(name + strLen - 3), ".js") == 0) {
    strLen -= 3;
    name[strLen] = '\0';
  }
  jerry_value_t builtin_module = mnode_get_builtin_module((char *)name);
  jerry_port_log(JERRY_LOG_LEVEL_WARNING, "buitin_module_name: %s\n",name);
  if (builtin_module != (jerry_value_t)NULL) {
    return builtin_module;
  }
  
  
  // not built-in module
  strcpy((char *)(name + strLen), ".js");
  char *file_name = malloc(strLen + 20);
  jerry_port_normalize_path((char*)name, file_name, 1024, "/framework");
  size_t size = 0;
  jerry_char_t *script = jerry_port_read_source((char *)file_name, &size);
  free(file_name);
  if (script == NULL || size == 0) { 
    return jerry_create_undefined();
  }

  // registered require module
  jerryx_handle_scope scope;
  jerryx_open_handle_scope(&scope);

  static const char *jargs = "exports, module, __filename";
  jerry_value_t res = jerryx_create_handle(jerry_parse_function((jerry_char_t *)name, strLen,
                                          (jerry_char_t *)jargs, strlen(jargs),
                                          (jerry_char_t *)script, size, JERRY_PARSE_NO_OPTS));
  jerry_port_release_source(script);
  jerry_value_t module = jerryx_create_handle(jerry_create_object());
  jerry_value_t exports = jerryx_create_handle(jerry_create_object());
  jerry_value_t prop_name = jerryx_create_handle(jerry_create_string((jerry_char_t *)"exports"));
  jerryx_create_handle(jerry_set_property(module, prop_name, exports));
  jerry_value_t filename = jerryx_create_handle(jerry_create_string((jerry_char_t *)name));
  jerry_value_t jargs_p[] = { exports, module, filename };
  jerry_value_t jres = jerryx_create_handle(jerry_call_function(res, (jerry_value_t)NULL, jargs_p, 3));
  (void)jres;
  jerry_value_t escaped_exports = jerry_get_property(module, prop_name);
  jerryx_close_handle_scope(scope);

  return escaped_exports;
}

void mnode_init_require()
{
  jerry_value_t global = jerry_get_global_object();

  jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"require");
  jerry_value_t value = jerry_create_external_function(mnode_require_handler);
  jerry_release_value(jerry_set_property(global, prop_name, value));
  jerry_release_value(prop_name);
  jerry_release_value(value);

  jerry_release_value(global);
}
