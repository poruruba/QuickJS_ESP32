#include <Arduino.h>
#include "quickjs.h"
#include "module_type.h"
#include "quickjs_esp32.h"
#include "module_sd.h"
#include <SD.h>

static JSValue sd_exists(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;
  bool ret = SD.exists(fname);
  JS_FreeCString(ctx, fname);
  return JS_NewBool(ctx, ret);
}

static JSValue sd_mkdir(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *dname = JS_ToCString(ctx, argv[0]);
  if( dname == NULL )
    return JS_EXCEPTION;
  bool ret = SD.mkdir(dname);
  JS_FreeCString(ctx, dname);
  return JS_NewBool(ctx, ret);
}

static JSValue sd_remove(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;
  bool ret = SD.remove(fname);
  JS_FreeCString(ctx, fname);
  return JS_NewBool(ctx, ret);
}

static JSValue sd_rmdir(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *dname = JS_ToCString(ctx, argv[0]);
  if( dname == NULL )
    return JS_EXCEPTION;
  bool ret = SD.rmdir(dname);
  JS_FreeCString(ctx, dname);
  return JS_NewBool(ctx, ret);
}

static JSValue sd_size(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;
  File file = SD.open(fname);
  JS_FreeCString(ctx, fname);
  if( !file )
    return JS_NewInt32(ctx, -1);
  uint32_t size = file.size();
  file.close();

  return JS_NewUint32(ctx, size);
}

static JSValue sd_readText(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;
  File file = SD.open(fname, FILE_READ);
  JS_FreeCString(ctx, fname);
  if( !file )
    return JS_EXCEPTION;
  uint32_t fsize = file.size();
  char *buffer = (char*)malloc(fsize + 1);
  if( buffer == NULL ){
    file.close();
    return JS_EXCEPTION;
  }
  uint32_t ret = file.readBytes(buffer, fsize);
  file.close();
  if( ret < 0 ){
    free(buffer);
    return JS_EXCEPTION;
  }
  buffer[ret] = '\0';
  JSValue value = JS_NewString(ctx, buffer);
  free(buffer);

  return value;
}

static JSValue sd_writeText(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;

  File file = SD.open(fname, FILE_WRITE);
  JS_FreeCString(ctx, fname);
  if( !file )
    return JS_EXCEPTION;

  const char *str = JS_ToCString(ctx, argv[1]);
  size_t size = strlen(str);
  size_t wrote = file.write((const uint8_t*)str, size + 1);
  file.close();
  JS_FreeCString(ctx, str);
  if( wrote != size + 1 )
    return JS_EXCEPTION;

  return JS_NewInt32(ctx, wrote);
}

static JSValue sd_readBinary(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;
  File file = SD.open(fname, FILE_READ);
  JS_FreeCString(ctx, fname);
  if( !file )
    return JS_EXCEPTION;
  uint32_t fsize = file.size();
  int32_t offset = 0;
  if( argc >= 2 )
    JS_ToInt32(ctx, &offset, argv[1]);
  if( offset < 0 )
    offset = 0;
  if( offset > fsize )
    offset = fsize;
  int32_t size = -1;
  if( argc >= 3)
    JS_ToInt32(ctx, &size, argv[2]);
  if( size < 0 || offset + size > fsize )
    size = fsize - offset;

  unsigned char *buffer = (unsigned char*)malloc(size);
  if( buffer == NULL ){
    file.close();
    return JS_EXCEPTION;
  }
  file.seek(offset);
  file.read(buffer, size);
  file.close();

  JSValue value = JS_NewArrayBufferCopy(ctx, buffer, size);
  free(buffer);

  return value;
}

static JSValue sd_writeBinary(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;

  File file = SD.open(fname, FILE_WRITE);
  JS_FreeCString(ctx, fname);
  if( !file )
    return JS_EXCEPTION;

  size_t bsize;
  uint8_t *buffer = JS_GetArrayBuffer(ctx, &bsize, argv[1]);
  if( buffer == NULL ){
    JSValue value = JS_GetPropertyStr(ctx, argv[1], "buffer");
    buffer = JS_GetArrayBuffer(ctx, &bsize, value);
    if( buffer == NULL ){
      file.close();
      return JS_EXCEPTION;
    }
  }

  uint32_t fsize = file.size();
  int32_t offset = -1;
  if( argc >= 3 )
    JS_ToInt32(ctx, &offset, argv[2]);
  if( offset < 0 )
    offset = fsize;
  int32_t size = -1;
  if( argc >= 4)
    JS_ToInt32(ctx, &size, argv[3]);
  if( size < 0 || size > bsize )
    size = bsize;

  file.seek(offset);
  size_t wrote = file.write(buffer, size);
  file.close();

  return JS_NewInt32(ctx, wrote);
}

static JSValue sd_isDirectory(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;
  File file = SD.open(fname, FILE_WRITE);
  JS_FreeCString(ctx, fname);
  if( !file )
    return JS_EXCEPTION;
  
  bool ret = file.isDirectory();
  file.close();

  return JS_NewBool(ctx, ret);
}

static JSValue sd_list(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fname = JS_ToCString(ctx, argv[0]);
  if( fname == NULL )
    return JS_EXCEPTION;
  File base = SD.open(fname, FILE_READ);
  JS_FreeCString(ctx, fname);

  JSValue jsArray = JS_NewArray(ctx);
  File file;
  for( int i = 0 ; file = base.openNextFile(); i++ ){
    JS_SetPropertyUint32(ctx, jsArray, i, JS_NewString(ctx, file.name()));
    file.close();
  }
  base.close();

  return jsArray;
}

static const JSCFunctionListEntry sd_funcs[] = {
    JSCFunctionListEntry{
        "exists", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_exists}
        }},
    JSCFunctionListEntry{
        "mkdir", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_mkdir}
        }},
    JSCFunctionListEntry{
        "remove", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_remove}
        }},
    JSCFunctionListEntry{
        "rmdir", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_rmdir}
        }},
    JSCFunctionListEntry{
        "size", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_size}
        }},
    JSCFunctionListEntry{
        "readBinary", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, sd_readBinary}
        }},
    JSCFunctionListEntry{
        "writeBinary", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, sd_writeBinary}
        }},
    JSCFunctionListEntry{
        "readText", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_readText}
        }},
    JSCFunctionListEntry{
        "writeText", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, sd_writeText}
        }},
    JSCFunctionListEntry{
        "isDirectory", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_isDirectory}
        }},
    JSCFunctionListEntry{
        "list", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, sd_list}
        }},
};

JSModuleDef *addModule_sd(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "sd", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, sd_funcs,
              sizeof(sd_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, sd_funcs,
        sizeof(sd_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

long initialize_sd(void)
{
  bool ret = SD.begin();

  return ret ? 0 : -1;
}

JsModuleEntry sd_module = {
  "sd",
  initialize_sd,
  addModule_sd,
  NULL,
  NULL,
};