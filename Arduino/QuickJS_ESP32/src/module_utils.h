#ifndef _MODULE_UTILS_H_
#define _MODULE_UTILS_H_

#include <ArduinoJson.h>
#include "module_type.h"

extern JsModuleEntry utils_module;

long http_get(String url, String *response);
long http_get_binary(String url, uint8_t *p_buffer, unsigned long *p_len);
long http_get_json(String url, JsonDocument *doc);

#endif