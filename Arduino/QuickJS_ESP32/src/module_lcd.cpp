#include <Arduino.h>
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_lcd.h"
#include "module_utils.h"
#include "module_type.h"

#ifdef _SD_ENABLE_
#include <SD.h>
#endif

#define LGFX_USE_V1
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
static LGFX lcd;

#define FONT_COLOR TFT_WHITE

#define FUNC_TYPE_WIDTH 1
#define FUNC_TYPE_HEIGHT 2
#define FUNC_TYPE_DEPTH 3
#define FUNC_TYPE_FONTHEIGHT 4

#define IMAGE_BUFFER_SIZE (10 * 1024)
static uint8_t image_buffer[IMAGE_BUFFER_SIZE];
static uint32_t image_length = 0;

static JSValue esp32_lcd_setRotation(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value;
  JS_ToInt32(ctx, &value, argv[0]);
  lcd.setRotation(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setBrigthness(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  lcd.setBrightness(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextColor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value0, value1;
  if( argc == 1 ){
    JS_ToUint32(ctx, &value0, argv[0]);
    lcd.setTextColor(value0);
  }else if( argc == 2 ){
    JS_ToUint32(ctx, &value0, argv[0]);
    JS_ToUint32(ctx, &value1, argv[1]);
    lcd.setTextColor(value0, value1);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextSize(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  double valueX, valueY;
  if( argc == 1 ){
    JS_ToFloat64(ctx, &valueX, argv[0]);
    lcd.setTextSize(valueX);
  }else if( argc == 2 ){
    JS_ToFloat64(ctx, &valueX, argv[0]);
    JS_ToFloat64(ctx, &valueY, argv[1]);
    lcd.setTextSize(valueX, valueY);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextDatum(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  lcd.setTextDatum(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_download_image(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *url = JS_ToCString(ctx, argv[0]);
  image_length = 0;
  unsigned long len = IMAGE_BUFFER_SIZE;
  long ret = http_get_binary(url, image_buffer, &len);
  JS_FreeCString(ctx, url);
  if( ret == 0 ){
    image_length = len;
    return JS_UNDEFINED;
  }else{
    return JS_EXCEPTION;
  }
}

static JSValue esp32_lcd_draw_image(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if (image_length == 0)
    return JS_NewInt32(ctx, -1);

  int32_t x;
  int32_t y;
  JS_ToInt32(ctx, &x, argv[0]);
  JS_ToInt32(ctx, &y, argv[1]);
  bool ret = false;
  if( image_buffer[0] == 0xff && image_buffer[1] == 0xd8 )
    ret = lcd.drawJpg(image_buffer, image_length, x, y);
  else if (image_buffer[0] == 0x89 && image_buffer[1] == 0x50 && image_buffer[2] == 0x4e && image_buffer[3] == 0x47 )
    ret = lcd.drawPng(image_buffer, image_length, x, y);

  if( ret )
    return JS_UNDEFINED;
  else
    return JS_EXCEPTION;
}

#ifdef _SD_ENABLE_
static JSValue esp32_lcd_draw_image_file(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fpath = JS_ToCString(ctx, argv[0]);
  File file = SD.open(fpath, FILE_READ);
  if( !file ){
    JS_FreeCString(ctx, fpath);
    return JS_EXCEPTION;
  }
  uint8_t image_buffer[4];
  int size = file.read(image_buffer, sizeof(image_buffer));
  file.close();
  if( size < sizeof(image_buffer) ){
    JS_FreeCString(ctx, fpath);
    return JS_EXCEPTION;
  }

  int32_t x;
  int32_t y;
  JS_ToInt32(ctx, &x, argv[1]);
  JS_ToInt32(ctx, &y, argv[2]);
  bool ret = false;
  if( image_buffer[0] == 0xff && image_buffer[1] == 0xd8 ){
    ret = lcd.drawJpgFile(SD, fpath, x, y);
  }else if (image_buffer[0] == 0x89 && image_buffer[1] == 0x50 && image_buffer[2] == 0x4e && image_buffer[3] == 0x47 ){
    ret = lcd.drawPngFile(SD, fpath, x, y);
  }
  JS_FreeCString(ctx, fpath);

  if( ret )
    return JS_UNDEFINED;
  else
    return JS_EXCEPTION;
}
#endif

static JSValue esp32_lcd_print(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  long ret;
  const char *text = JS_ToCString(ctx, argv[0]);
  if (magic == 0){
    ret = lcd.print(text);
  }else
  if (magic == 1){
    ret = lcd.println(text);
  }else{
    JS_FreeCString(ctx, text);
    return JS_EXCEPTION;
  }

  JS_FreeCString(ctx, text);
  return JS_NewInt32(ctx, ret);
}

static JSValue esp32_lcd_setFont(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value;
  JS_ToInt32(ctx, &value, argv[0]);
  switch (value){
//    case 8 : lcd.setFont(&fonts::lgfxJapanGothic_8); break;
//    case 12 : lcd.setFont(&fonts::lgfxJapanGothic_12); break;
    case 16 : lcd.setFont(&fonts::lgfxJapanGothic_16); break;
//    case 20 : lcd.setFont(&fonts::lgfxJapanGothic_20); break;
//    case 24 : lcd.setFont(&fonts::lgfxJapanGothic_24); break;
//    case 28 : lcd.setFont(&fonts::lgfxJapanGothic_28); break;
//    case 32 : lcd.setFont(&fonts::lgfxJapanGothic_32); break;
//    case 36 : lcd.setFont(&fonts::lgfxJapanGothic_36); break;
//    case 40 : lcd.setFont(&fonts::lgfxJapanGothic_40); break;
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setCursor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value0;
  int32_t value1;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  lcd.setCursor(value0, value1);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_getCursor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "x", JS_NewInt32(ctx, lcd.getCursorX()));
  JS_SetPropertyStr(ctx, obj, "y", JS_NewInt32(ctx, lcd.getCursorY()));
  return obj;
}

static JSValue esp32_lcd_textWidth(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *text = JS_ToCString(ctx, argv[0]);
  int32_t width = lcd.textWidth(text);
  JS_FreeCString(ctx, text);
  return JS_NewInt32(ctx, width);
}

static JSValue esp32_lcd_drawPixel(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value0;
  int32_t value1;
  int32_t value2;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  lcd.drawPixel(value0, value1, value2);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_drawLine(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value0;
  int32_t value1;
  int32_t value2;
  int32_t value3;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  JS_ToInt32(ctx, &value3, argv[3]);
  if(argc >= 5){
    int32_t value4;
    JS_ToInt32(ctx, &value4, argv[4]);
    lcd.drawLine(value0, value1, value2, value3, value4);
  }else{
    lcd.drawLine(value0, value1, value2, value3);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_getMetric(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  int32_t value = 0;

  switch(magic){
    case FUNC_TYPE_WIDTH: value = lcd.width(); break;
    case FUNC_TYPE_HEIGHT: value = lcd.height(); break;
    case FUNC_TYPE_DEPTH: value = lcd.getColorDepth(); break;
    case FUNC_TYPE_FONTHEIGHT: value = lcd.fontHeight(); break;
  }
  return JS_NewInt32(ctx, value);
}

static JSValue esp32_lcd_fillScreen(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  lcd.fillScreen(value);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry lcd_funcs[] = {
    JSCFunctionListEntry{"setRotation", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setRotation}
                         }},
    JSCFunctionListEntry{"setBrightness", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setBrigthness}
                         }},
    JSCFunctionListEntry{"setFont", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setFont}
                         }},
    JSCFunctionListEntry{"setTextColor", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_lcd_setTextColor}
                         }},
    JSCFunctionListEntry{"setTextSize", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_lcd_setTextSize}
                         }},
    JSCFunctionListEntry{"setTextDatum", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setTextDatum}
                         }},
    JSCFunctionListEntry{"drawPixel", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, esp32_lcd_drawPixel}
                         }},
    JSCFunctionListEntry{"drawLine", 0, JS_DEF_CFUNC, 0, {
                           func : {5, JS_CFUNC_generic, esp32_lcd_drawLine}
                         }},
    JSCFunctionListEntry{"setCursor", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_lcd_setCursor}
                         }},
    JSCFunctionListEntry{"getCursor", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_lcd_getCursor}
                         }},
    JSCFunctionListEntry{"textWidth", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_textWidth}
                         }},
    JSCFunctionListEntry{"print", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_print}}
                         }},
    JSCFunctionListEntry{"println", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_print}}
                         }},
    JSCFunctionListEntry{"fillScreen", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_fillScreen}
                         }},
    JSCFunctionListEntry{"downloadImage", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_download_image}
                         }},
    JSCFunctionListEntry{"drawImage", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_lcd_draw_image}
                         }},
#ifdef _SD_ENABLE_
    JSCFunctionListEntry{"drawImageFile", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, esp32_lcd_draw_image_file}
                         }},
#endif
    JSCFunctionListEntry{"width", 0, JS_DEF_CFUNC, FUNC_TYPE_WIDTH, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},
    JSCFunctionListEntry{"height", 0, JS_DEF_CFUNC, FUNC_TYPE_HEIGHT, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},
    JSCFunctionListEntry{"getColorDepth", 0, JS_DEF_CFUNC, FUNC_TYPE_DEPTH, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},
    JSCFunctionListEntry{"fontHeight", 0, JS_DEF_CFUNC, FUNC_TYPE_FONTHEIGHT, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},

    JSCFunctionListEntry{
        "top_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::top_left
        }},
    JSCFunctionListEntry{
        "top_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::top_center
        }},
    JSCFunctionListEntry{
        "top_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::top_right
        }},
    JSCFunctionListEntry{
        "middle_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::middle_left
        }},
    JSCFunctionListEntry{
        "middle_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::middle_center
        }},
    JSCFunctionListEntry{
        "middle_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::middle_right
        }},
    JSCFunctionListEntry{
        "bottom_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::bottom_left
        }},
    JSCFunctionListEntry{
        "bottom_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::bottom_center
        }},
    JSCFunctionListEntry{
        "bottom_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::bottom_right
        }},
    JSCFunctionListEntry{
        "baseline_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::baseline_left
        }},
    JSCFunctionListEntry{
        "baseline_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::baseline_center
        }},
    JSCFunctionListEntry{
        "baseline_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::baseline_right
        }},
};

JSModuleDef *addModule_lcd(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "lcd", [](JSContext *ctx, JSModuleDef *m){
        return JS_SetModuleExportList(
                            ctx, m, lcd_funcs,
                            sizeof(lcd_funcs) / sizeof(JSCFunctionListEntry));
      });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, lcd_funcs,
        sizeof(lcd_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

long initialize_lcd(void)
{
  lcd.init();
  lcd.setFont(&fonts::lgfxJapanGothic_16);
  lcd.setColorDepth(16);
  lcd.setBrightness(128);
  lcd.setRotation(1);
  lcd.setCursor(0, 0);
  lcd.setTextColor(FONT_COLOR);

  return 0;
}

JsModuleEntry lcd_module = {
  "input",
  initialize_lcd,
  addModule_lcd,
  NULL,
  NULL
};