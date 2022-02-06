#ifndef _MODULE_TYPE_H_
#define _MODULE_TYPE_H_

#include "quickjs.h"

typedef long(*InitializeModuleImpl)(void);
typedef JSModuleDef*(*AddModuleImpl)(JSContext *ctx, JSValue global);
typedef void(*LoopModuleImpl)(void);
typedef void(*EndModuleImpl)(void);

typedef struct {
  const char *name;
  InitializeModuleImpl initializeImpl;
  AddModuleImpl addImpl;
  LoopModuleImpl loopImpl;
  EndModuleImpl endImpl;
} JsModuleEntry;

#endif
