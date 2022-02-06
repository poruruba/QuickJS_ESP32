#include <Arduino.h>
#include "main_config.h"
#include "quickjs.h"
#include "module_type.h"

#define TOUCHPANEL  0
#define BUTTON_A    1
#define BUTTON_B    2
#define BUTTON_C    3

#define FUNC_TYPE_IS_PRESSED        0
#define FUNC_TYPE_IS_RELEASED       1
#define FUNC_TYPE_WAS_PRESSED       2
#define FUNC_TYPE_WAS_RELEASED      3
#define FUNC_TYPE_PRESSED_FOR       4
#define FUNC_TYPE_RELEASED_FOR      5
#define FUNC_TYPE_WAS_RELEASE_FOR   6
#define FUNC_TYPE_IS_TOUCHED        7

static JSValue input_checkButtonState(JSContext *ctx, JSValueConst jsThis, int argc,
                                JSValueConst *argv, int magic)
{
  uint32_t value0, value1 = 0;
  JS_ToUint32(ctx, &value0, argv[0]);
  if (magic == FUNC_TYPE_PRESSED_FOR || magic == FUNC_TYPE_RELEASED_FOR || magic == FUNC_TYPE_WAS_RELEASE_FOR)
    JS_ToUint32(ctx, &value1, argv[1]);

  bool result = false;
  switch(magic){
    case FUNC_TYPE_IS_PRESSED: {
      switch(value0){
        case BUTTON_A: result = M5.BtnA.isPressed(); break;
        case BUTTON_B: result = M5.BtnB.isPressed();
#if defined(ARDUINO_M5Stick_C)
#elif defined(ARDUINO_M5STACK_Core2)
        case BUTTON_C: result = M5.BtnC.isPressed(); break;
#endif
      }
      break;
    }
    case FUNC_TYPE_IS_RELEASED: {
      switch(value0){
        case BUTTON_A: result = M5.BtnA.isReleased(); break;
        case BUTTON_B: result = M5.BtnB.isReleased(); break;
#if defined(ARDUINO_M5Stick_C)
#elif defined(ARDUINO_M5STACK_Core2)
        case BUTTON_C: result = M5.BtnC.isReleased(); break;
#endif
      }
      break;
    }
    case FUNC_TYPE_WAS_PRESSED: {
      switch(value0){
        case BUTTON_A: result = M5.BtnA.wasPressed(); break;
        case BUTTON_B: result = M5.BtnB.wasPressed(); break;
#if defined(ARDUINO_M5Stick_C)
#elif defined(ARDUINO_M5STACK_Core2)
        case BUTTON_C: result = M5.BtnC.wasPressed(); break;
#endif
      }
      break;
    }
    case FUNC_TYPE_WAS_RELEASED: {
      switch(value0){
        case BUTTON_A: result = M5.BtnA.wasReleased(); break;
        case BUTTON_B: result = M5.BtnB.wasReleased(); break;
#if defined(ARDUINO_M5Stick_C)
#elif defined(ARDUINO_M5STACK_Core2)
        case BUTTON_C: result = M5.BtnC.wasReleased(); break;
#endif
      }
      break;
    }
    case FUNC_TYPE_PRESSED_FOR: {
      switch(value0){
        case BUTTON_A: result = M5.BtnA.pressedFor(value1); break;
        case BUTTON_B: result = M5.BtnB.pressedFor(value1); break;
#if defined(ARDUINO_M5Stick_C)
#elif defined(ARDUINO_M5STACK_Core2)
        case BUTTON_C: result = M5.BtnC.pressedFor(value1); break;
#endif
      }
      break;
    }
    case FUNC_TYPE_RELEASED_FOR: {
      switch(value0){
        case BUTTON_A: result = M5.BtnA.releasedFor(value1); break;
        case BUTTON_B: result = M5.BtnB.releasedFor(value1); break;
#if defined(ARDUINO_M5Stick_C)
#elif defined(ARDUINO_M5STACK_Core2)
        case BUTTON_C: result = M5.BtnC.releasedFor(value1); break;
#endif
      }
      break;
    }
    case FUNC_TYPE_WAS_RELEASE_FOR: {
      switch(value0){
        case BUTTON_A: result = M5.BtnA.wasReleasefor(value1); break;
        case BUTTON_B: result = M5.BtnB.wasReleasefor(value1); break;
#if defined(ARDUINO_M5Stick_C)
#elif defined(ARDUINO_M5STACK_Core2)
        case BUTTON_C: result = M5.BtnC.wasReleasefor(value1); break;
#endif
      }
      break;
    }
  }

  return JS_NewBool(ctx, result);
}

#if defined(ARDUINO_M5STACK_Core2)
static JSValue input_isTouched(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  if( M5.Touch.ispressed() ){
    TouchPoint_t pos = M5.Touch.getPressPoint();
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "x", JS_NewInt32(ctx, pos.x));
    JS_SetPropertyStr(ctx, obj, "y", JS_NewInt32(ctx, pos.y));
    return obj;
  }else{
    return JS_NULL;
  }
}
#endif

static const JSCFunctionListEntry input_funcs[] = {
    JSCFunctionListEntry{"isPressed", 0, JS_DEF_CFUNC, FUNC_TYPE_IS_PRESSED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"isReleased", 0, JS_DEF_CFUNC, FUNC_TYPE_IS_RELEASED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"wasPressed", 0, JS_DEF_CFUNC, FUNC_TYPE_WAS_PRESSED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"wasReleased", 0, JS_DEF_CFUNC, FUNC_TYPE_WAS_RELEASED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"pressedFor", 0, JS_DEF_CFUNC, FUNC_TYPE_PRESSED_FOR, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"releasedFor", 0, JS_DEF_CFUNC, FUNC_TYPE_RELEASED_FOR, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"wasReleaseFor", 0, JS_DEF_CFUNC, FUNC_TYPE_WAS_RELEASE_FOR, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
#if defined(ARDUINO_M5STACK_Core2)
    JSCFunctionListEntry{
        "isTouched", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, input_isTouched}
        }},
#endif
    JSCFunctionListEntry{
        "BUTTON_A", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BUTTON_A
        }},
    JSCFunctionListEntry{
        "BUTTON_B", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BUTTON_B
        }},
    JSCFunctionListEntry{
        "BUTTON_C", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BUTTON_C
        }},
};

JSModuleDef *addModule_input(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "input", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, input_funcs,
              sizeof(input_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, input_funcs,
        sizeof(input_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry input_module = {
  "input",
  NULL,
  addModule_input,
  NULL,
  NULL
};