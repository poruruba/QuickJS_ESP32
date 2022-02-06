#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "main_config.h"
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_type.h"
#include "module_esp32.h"
#include "endpoint_udpApi.h"

#define GLOBAL_ESP32
#define GLOBAL_CONSOLE

#define MODEL_OTHER 0
#define MODEL_M5Core2 1
#define MODEL_M5Core 2
#define MODEL_M5Fire 3
#define MODEL_M5StickCPlus 4
#define MODEL_M5CoreInk 5
#define MODEL_M5Paper 6
#define MODEL_M5Tough 7
#define MODEL_M5StickC 8

static JSValue esp32_deep_sleep(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t t;
  JS_ToUint32(ctx, &t, argv[0]);
  ESP.deepSleep(t); // never return.
  return JS_UNDEFINED;
}

static JSValue esp32_reboot(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  ESP.restart();
  return JS_UNDEFINED;
}

static JSValue esp32_download(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t type;
  JS_ToInt32(ctx, &type, argv[0]);
  g_downloading = type;
  const char *p_url = JS_ToCString(ctx, argv[1]);
  strcpy(g_download_buffer, p_url);
  JS_FreeCString(ctx, p_url);

  return JS_UNDEFINED;
}

static JSValue esp32_restart(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_download_buffer[0] = '\0';
  g_downloading = FILE_LOADING_RESTART;

  return JS_UNDEFINED;
}

static JSValue esp32_set_loop(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(ctx);
  qjs->setLoopFunc(JS_DupValue(ctx, argv[0]));
  return JS_UNDEFINED;
}

static JSValue esp32_update(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(ctx);
  qjs->update_modules();
  return JS_UNDEFINED;
}

static JSValue esp32_get_ipaddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  IPAddress address = WiFi.localIP();  
  return JS_NewUint32(ctx, (uint32_t)(((uint32_t)address[0]) << 24 | address[1] << 16 | address[2] << 8 | address[3]));
}

static JSValue esp32_get_macaddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t address[6];
  WiFi.macAddress(address);

  JSValue jsArray = JS_NewArray(ctx);
  for (int i = 0; i < 6; i++)
    JS_SetPropertyUint32(ctx, jsArray, i, JS_NewInt32(ctx, address[i]));

  return jsArray;
}

static JSValue esp32_get_devicemodel(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t model = MODEL_OTHER;
#if defined(ARDUINO_M5Stack_Core_ESP32) // M5StackCore
  model = MODEL_M5Core;
#elif defined(ARDUINO_M5STACK_FIRE)  //M5Stack Fire
  model = MODEL_M5Fire;
#elif defined(ARDUINO_M5STACK_Core2) // M5Stack Core2
  model = MODEL_M5Core2;
#elif defined(ARDUINO_M5Stick_C) // M5Stick C
  model = MODEL_M5StickC;
#elif defined(ARDUINO_M5Stick_C_Plus) // M5StickCPlus
  model = MODEL_M5StickCPlus;
#elif defined(ARDUINO_M5Stack_CoreInk) // M5Stack CoreInk
  model = MODEL_M5CoreInk;
#elif defined(ARDUINO_M5STACK_Paper) // M5Paper
  model = MODEL_M5Paper;
#elif defined(ARDUINO_M5STACK_TOUGH) // M5Tough
  model = MODEL_M5Tough;
#endif
  return JS_NewUint32(ctx, model);
}

static JSValue esp32_console_log(JSContext *ctx, JSValueConst jsThis, int argc,
                            JSValueConst *argv, int magic) {
  int i = 0;
  if( magic == 0 ){
    bool enable = JS_ToBool(ctx, argv[0]);
    if( enable )
      return JS_UNDEFINED;
    i = 1;
  }
  for (; i < argc; i++) {
    const char *str = JS_ToCString(ctx, argv[i]);
    if (str) {
      Serial.println(str);
#ifdef _LOGIO_ENABLE_
      switch(magic){
        case 0: logio_log2("assert", str); break;
        case 1: logio_log2("log", str); break;
        case 2: logio_log2("info", str); break;
        case 3: logio_log2("debug", str); break;
        case 4: logio_log2("warn", str); break;
        case 5: logio_log2("error", str); break;
      }
#endif
      JS_FreeCString(ctx, str);
    }
  }
  return JS_UNDEFINED;
}

static JSValue esp32_enable_logio(JSContext *ctx, JSValueConst jsThis, int argc,
                            JSValueConst *argv) {
#ifdef _LOGIO_ENABLE_
  bool enable = JS_ToBool(ctx, argv[0]);
  logio_enable(enable);
#endif
  
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry console_funcs[] = {
    JSCFunctionListEntry{"assert", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"log", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"info", 0, JS_DEF_CFUNC, 2, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"debug", 0, JS_DEF_CFUNC, 3, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"warn", 0, JS_DEF_CFUNC, 4, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"error", 0, JS_DEF_CFUNC, 5, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_console_log }}
                         }},
    JSCFunctionListEntry{"enableLogio", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_enable_logio}
                         }},
};

JSModuleDef *addModule_console(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "console", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, console_funcs,
                            sizeof(console_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, console_funcs,
        sizeof(console_funcs) / sizeof(JSCFunctionListEntry));
  }

#ifdef GLOBAL_CONSOLE
  // import * as console from "console";
  JSValue console = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, global, "console", console);
  JS_SetPropertyFunctionList(
      ctx, console, console_funcs,
      sizeof(console_funcs) / sizeof(JSCFunctionListEntry));
#endif

  return mod;
}

static const JSCFunctionListEntry esp32_funcs[] = {
    JSCFunctionListEntry{"deepSleep", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_deep_sleep}
                         }},
    JSCFunctionListEntry{"reboot", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_reboot}
                         }},
    JSCFunctionListEntry{"restart", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_restart}
                         }},
    JSCFunctionListEntry{"download", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_download}
                         }},
    JSCFunctionListEntry{"setLoop", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_set_loop}
                         }},
    JSCFunctionListEntry{"update", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_update}
                         }},
    JSCFunctionListEntry{"getIpAddress", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_get_ipaddress}
                         }},
    JSCFunctionListEntry{"getMacAddress", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_get_macaddress}
                         }},
    JSCFunctionListEntry{"getDeviceModel", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_get_devicemodel}
                         }},
    JSCFunctionListEntry{
        "MODEL_OTHER", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_OTHER
        }},
    JSCFunctionListEntry{
        "MODEL_M5Core2", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Core2
        }},
    JSCFunctionListEntry{
        "MODEL_M5Core", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Core
        }},
    JSCFunctionListEntry{
        "MODEL_M5Fire", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Fire
        }},
    JSCFunctionListEntry{
        "MODEL_M5StickCPlus", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StickCPlus
        }},
    JSCFunctionListEntry{
        "MODEL_M5CoreInk", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5CoreInk
        }},
    JSCFunctionListEntry{
        "MODEL_M5Paper", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Paper
        }},
    JSCFunctionListEntry{
        "MODEL_M5Tough", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5Tough
        }},
    JSCFunctionListEntry{
        "MODEL_M5StickC", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MODEL_M5StickC
        }},
};

JSModuleDef *addModule_esp32(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "esp32", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, esp32_funcs,
                            sizeof(esp32_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, esp32_funcs,
        sizeof(esp32_funcs) / sizeof(JSCFunctionListEntry));
  }

#ifdef GLOBAL_ESP32
  // import * as esp32 from "esp32";
  JSValue esp32 = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, global, "esp32", esp32);
  JS_SetPropertyFunctionList(
      ctx, esp32, esp32_funcs,
      sizeof(esp32_funcs) / sizeof(JSCFunctionListEntry));
#endif

  return mod;
}

void loopModule_esp32(void)
{
  M5.update();
}

JsModuleEntry esp32_module = {
  "esp32",
  NULL,
  addModule_esp32,
  loopModule_esp32
};

JsModuleEntry console_module = {
  "console",
  NULL,
  addModule_console,
  NULL,
  NULL
};
