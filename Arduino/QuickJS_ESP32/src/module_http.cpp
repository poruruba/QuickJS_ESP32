#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "quickjs.h"
#include "main_config.h"
#include "module_type.h"
#include "module_utils.h"

//extern const char *HTTPPROXY_URL; // 中継サーバのURL

#define HTTP_TYPE_POST_JSON 0x10
#define HTTP_TYPE_POST_TEXT 0x11
#define HTTP_TYPE_GET_JSON  0x20
#define HTTP_TYPE_GET_TEXT  0x21

static const char *JSON = "JSON";
static const char *TEXT = "TEXT";
static const char *POST = "POST";
static const char *GET = "GET";

static JSValue http_json(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
#ifdef _HTTP_PROXY_ENABLE_
  const char *method;
  const char *response_type;
  switch(magic){
    case HTTP_TYPE_POST_JSON:{
      method = POST;
      response_type = JSON;
      break;
    }
    case HTTP_TYPE_POST_TEXT:{
      method = POST;
      response_type = TEXT;
      break;
    }
    case HTTP_TYPE_GET_JSON:{
      method = GET;
      response_type = JSON;
      break;
    }
    case HTTP_TYPE_GET_TEXT:{
      method = GET;
      response_type = TEXT;
      break;
    }
    default:
      return JS_EXCEPTION;
  }
  Serial.printf("method=%s response_type=%s\n", method, response_type);
  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "url", JS_DupValue(ctx, argv[0]));
  if (argc >= 1)
    JS_SetPropertyStr(ctx, obj, "params", JS_DupValue(ctx, argv[1]));
  if (argc >= 2)
    JS_SetPropertyStr(ctx, obj, "headers", JS_DupValue(ctx, argv[2]));
  JSValue method_val = JS_NewString(ctx, method);
  JSValue response_type_val = JS_NewString(ctx, response_type);
  JS_SetPropertyStr(ctx, obj, "method", method_val);
  JS_SetPropertyStr(ctx, obj, "response_type", response_type_val);

  JSValue json = JS_JSONStringify(ctx, obj, JS_UNDEFINED, JS_UNDEFINED);
  JS_FreeValue(ctx, obj);
  if( json == JS_UNDEFINED ){
    return JS_EXCEPTION;
  }
  const char *body = JS_ToCString(ctx, json);
  if (body == NULL){
    JS_FreeValue(ctx, json);
    return JS_EXCEPTION;
  }
  Serial.println(body);

  HTTPClient http;
  http.begin(HTTPPROXY_URL); //HTTP
  http.addHeader("Content-Type", "application/json");

  int status_code;
  // HTTP POST JSON
  status_code = http.POST(body);
  JS_FreeCString(ctx, body);
  JS_FreeValue(ctx, json);

  JSValue value = JS_EXCEPTION;
  if (status_code != 200){
    Serial.printf("status_code=%d\n", status_code);
    goto end;
  }

  if (strcmp(response_type, "JSON") == 0 ){
    String result = http.getString();
    const char *buffer = result.c_str();
    value = JS_ParseJSON(ctx, buffer, strlen(buffer), "json");
  }else if( strcmp(response_type, "TEXT") == 0 ){
    String result = http.getString();
    const char *buffer = result.c_str();
    value = JS_NewString(ctx, buffer);
  }

end:
  http.end();

  return value;
#else
  return JS_EXCEPTION;
#endif
}

static const JSCFunctionListEntry http_funcs[] = {
    JSCFunctionListEntry{"postJson", 0, JS_DEF_CFUNC, HTTP_TYPE_POST_JSON, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : http_json}}
                         }},
    JSCFunctionListEntry{"postText", 0, JS_DEF_CFUNC, HTTP_TYPE_POST_TEXT, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : http_json}}
                         }},
    JSCFunctionListEntry{"getJson", 0, JS_DEF_CFUNC, HTTP_TYPE_GET_JSON, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : http_json}}
                         }},
    JSCFunctionListEntry{"getText", 0, JS_DEF_CFUNC, HTTP_TYPE_GET_TEXT, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : http_json}}
                         }},
};

JSModuleDef *addModule_http(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "http", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, http_funcs,
                            sizeof(http_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, http_funcs,
        sizeof(http_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry http_module = {
  "gpio",
  NULL,
  addModule_http,
  NULL,
  NULL
};