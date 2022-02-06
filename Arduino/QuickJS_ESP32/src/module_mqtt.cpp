#include <Arduino.h>
#include <WiFi.h>
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "main_config.h"
#include "module_type.h"
#include "module_mqtt.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define MQTT_BUFFER_SIZE 256            // MQTT送受信のバッファサイズ

JSContext *g_ctx = NULL;

WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static char g_client_name[33] = "";
static char g_topic_name[33] = "";
static JSValue g_callback_func = JS_UNDEFINED;
static char g_received_topic[33] = "";
static byte *g_received_payload = NULL;

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  Serial.println("mqtt_callback");

  if(g_ctx == NULL || g_callback_func == JS_UNDEFINED)
    return;
  
  if( length <= MQTT_BUFFER_SIZE - 1 && strlen(topic) < sizeof(g_received_topic) - 1){
    strcpy(g_received_topic, topic);
    g_received_payload = payload;
    g_received_payload[length] = '\0';
  }
}

long mqttUnsubscribe(void){
  if( g_callback_func != JS_UNDEFINED ){
    mqttClient.unsubscribe(g_topic_name);
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }
  g_received_payload = NULL;

  return 0;
}

long mqttSubscribe(const char* topic_name, JSValue callback){
  mqttUnsubscribe();
  if( mqttClient.subscribe(topic_name) ){
    strcpy(g_topic_name, topic_name);
    g_callback_func = callback;
    return 0;
  }else{
    return -1;
  }
}

void mqttDisconnect(void){
  mqttUnsubscribe();
  if (mqttClient.connected())
      mqttClient.disconnect();

  g_ctx = NULL;
}

boolean mqttConnect(JSContext *ctx, const char *client_name, const char *broker_url, uint16_t broker_port)
{
  mqttDisconnect();

  strcpy(g_client_name, client_name);

  // バッファサイズの変更
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  // MQTTコールバック関数の設定
  mqttClient.setCallback(mqttCallback);
  // MQTTブローカに接続
  mqttClient.setServer(broker_url, broker_port);

  g_ctx = ctx;

  return mqttClient.connect(g_client_name);
}

static JSValue mqtt_connect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
#ifdef _MQTT_ENABLE_
  const char *client_name = JS_ToCString(ctx, argv[0]);
  if( client_name ){
    if (argc >= 2){
      const char *broker_url = JS_ToCString(ctx, argv[1]);
      uint32_t broker_port = MQTT_BROKER_PORT;
      if( argc >= 3 )
        JS_ToUint32(ctx, &broker_port, argv[2]);
      mqttConnect(ctx, client_name, broker_url, (uint16_t)broker_port);
      JS_FreeCString(ctx, broker_url);
    }else{
      mqttConnect(ctx, client_name, MQTT_BROKER_URL, MQTT_BROKER_PORT);
    }
    JS_FreeCString(ctx, client_name);
  }

  return JS_UNDEFINED;
#else
  return JS_EXCEPTION;
#endif
}

static JSValue mqtt_get_payload(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if (g_received_payload == NULL)
    return JS_EXCEPTION;
  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "payload", JS_NewString(ctx, (const char *)g_received_payload));
  JS_SetPropertyStr(ctx, obj, "topic", JS_NewString(ctx, (const char *)g_received_topic));

  return obj;
}

static JSValue mqtt_disconnect(JSContext * ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  mqttDisconnect();
  return JS_UNDEFINED;
}

static JSValue mqtt_subscribe(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *topic_name = JS_ToCString(ctx, argv[0]);
  if( topic_name ){
    JSValue func = JS_DupValue(g_ctx, argv[1]);
    long ret = mqttSubscribe(topic_name, func);
    JS_FreeCString(ctx, topic_name);
    if( ret != 0 )
      return JS_EXCEPTION;
  }else{
      return JS_EXCEPTION;
  }

  return JS_UNDEFINED;
}

static JSValue mqtt_unsubscribe(JSContext * ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  mqttUnsubscribe();

  return JS_UNDEFINED;
}

static JSValue mqtt_publish(JSContext * ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *topic = JS_ToCString(ctx, argv[0]);
  if( topic == NULL )
    return JS_EXCEPTION;

  const char *payload = JS_ToCString(ctx, argv[1]);
  if( payload == NULL ){
    JS_FreeCString(ctx, topic);
    return JS_EXCEPTION;
  }

  bool ret = mqttClient.publish(topic, payload);
  JS_FreeCString(ctx, payload);
  JS_FreeCString(ctx, topic);

  if( ret )
    return JS_UNDEFINED;
  else
    return JS_EXCEPTION;
}

static const JSCFunctionListEntry mqtt_funcs[] = {
    JSCFunctionListEntry{
        "connect", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, mqtt_connect}
        }},
    JSCFunctionListEntry{
        "disconnect", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, mqtt_disconnect}
        }},
    JSCFunctionListEntry{
        "subscribe", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, mqtt_subscribe}
        }},
    JSCFunctionListEntry{
        "unsubscribe", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, mqtt_unsubscribe}
        }},
    JSCFunctionListEntry{
        "publish", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, mqtt_publish}
        }},
    JSCFunctionListEntry{
        "getPayload", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, mqtt_get_payload}
        }},
};

JSModuleDef *addModule_mqtt(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "mqtt", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, mqtt_funcs,
              sizeof(mqtt_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, mqtt_funcs,
        sizeof(mqtt_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void loopModule_mqtt(void){
  if( g_ctx != NULL ){
    mqttClient.loop();
    // MQTT未接続の場合、再接続
    while (!mqttClient.connected()){
      Serial.println("Mqtt Reconnecting");
      if (mqttClient.connect(g_client_name)){
        if( g_callback_func != JS_UNDEFINED )
          mqttClient.subscribe(g_topic_name);
        Serial.println("Mqtt Reconnected");

        break;
      }

      delay(1000);
    }

    if( g_received_payload != NULL && g_callback_func != JS_UNDEFINED){
      ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
      JSValue ret = qjs->callJsFunc(g_ctx, g_callback_func, g_callback_func);
      JS_FreeValue(g_ctx, ret);
      g_received_payload = NULL;
    }
  }
}

void endModule_mqtt(void){
  mqttDisconnect();
}

JsModuleEntry mqtt_module = {
  "mqtt",
  NULL,
  addModule_mqtt,
  loopModule_mqtt,
  endModule_mqtt,
};