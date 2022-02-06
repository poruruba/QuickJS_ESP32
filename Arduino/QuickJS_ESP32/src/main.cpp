#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "quickjs_esp32.h"
#include "main_config.h"
#include "endpoint_udpApi.h"
#include "endpoint_types.h"
#include "endpoint_esp32.h"
#include "endpoint_gpio.h"
#include "endpoint_ledc.h"
#include "endpoint_pixels.h"
#include "endpoint_wire.h"

#define DUMMY_FNAME  "/dummy"
#define MAIN_FNAME  "/main.js"
#define MODULE_FNAME  "/module.js"

static char js_code[JSCODE_BUFFER_SIZE];
static char module_code[JSMODULE_BUFFER_SIZE];

char g_download_buffer[FILE_BUFFER_SIZE];
unsigned char g_downloading = FILE_LOADING_NONE;
unsigned char g_fileloading = FILE_LOADING_NONE;

extern const char jscode_default[] asm("_binary_rom_default_js_start");
extern const char jscode_epilogue[] asm("_binary_rom_epilogue_js_start");

ESP32QuickJS qjs;
AsyncWebServer server(HTTP_PORT);

static long m5_initialize(void);
static long download_jscode_from_url(const char *main_url);
static long download_jsmodules_from_url(const char *modules_url);
static long download_jsmodules_from_text(const char *json_text);
static long download_jsmodules(JsonDocument *doc);
static long load_jsmodules_and_jscode(void);
static long save_jscode(void);
static long save_jsmodules(void);

void handleJsUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if(index == 0)
    Serial.printf("UploadStart: %s\n", filename.c_str());

  if( index + len >= sizeof(g_download_buffer) ){
    Serial.print("buffer overflow");
    return;
  }

  memmove(&g_download_buffer[index], data, len);

  if(final){
    Serial.printf("UploadEnd: %s, %u bytes\n", filename.c_str(), index + len);
    g_download_buffer[index + len] = '\0';
    Serial.println(g_download_buffer);
    g_fileloading = FILE_LOADING_JS;
  }
}

void handleModuleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if(index == 0)
    Serial.printf("UploadStart: %s\n", filename.c_str());

  if( index + len >= sizeof(g_download_buffer) ){
    Serial.print("buffer overflow");
    return;
  }

  memmove(&g_download_buffer[index], data, len);

  if(final){
    Serial.printf("UploadEnd: %s, %u bytes\n", filename.c_str(), index + len);
    g_download_buffer[index + len] = '\0';
    Serial.println(g_download_buffer);
    g_fileloading = FILE_LOADING_MODULE;
  }
}

void notFound(AsyncWebServerRequest *request){
  if (request->method() == HTTP_OPTIONS){
    request->send(200);
  }else{
    request->send(404);
  }
}

void setup()
{
  SPIFFS.begin();
  delay(50);

  if( !SPIFFS.exists(DUMMY_FNAME) ){
    File fp = SPIFFS.open(DUMMY_FNAME, FILE_WRITE);
    if( !fp ){
      Serial.println("SPIFFS FORMAT START");
      SPIFFS.format();
      Serial.println("SPIFFS FORMAT END");

      File fp = SPIFFS.open(DUMMY_FNAME, FILE_WRITE);
      fp.close();
    }else{
      fp.close();
    }
  }  

  long ret;

  ret = m5_initialize();
  if( ret == 0 ){
    udpapi_initialize();
    udpapi_appendEntry(esp32_table, num_of_esp32_entry);
    udpapi_appendEntry(gpio_table, num_of_gpio_entry);
    udpapi_appendEntry(wire_table, num_of_wire_entry);
    udpapi_appendEntry(ledc_table, num_of_ledc_entry);
    udpapi_appendEntry(pixels_table, num_of_pixels_entry);

    server.on("/upload_js", HTTP_POST, [](AsyncWebServerRequest *request){
      request->send(200);
    }, handleJsUpload);
    server.on("/upload_module", HTTP_POST, [](AsyncWebServerRequest *request){
      request->send(200);
    }, handleModuleUpload);
    server.on("/download_js", HTTP_GET, [](AsyncWebServerRequest *request) {
      long len = strlen(js_code) - strlen(jscode_epilogue);
      request->send(200, "text/javascript", String(js_code).substring(0, len));
    });
    
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");
    server.serveStatic("/", SPIFFS, "/html/").setDefaultFile("index.html");
    server.onNotFound(notFound);
    server.begin();
  }
 
  qjs.initialize_modules();
  qjs.begin();

  ret = load_jsmodules_and_jscode();
  if( ret == 0 ){
    Serial.println("[executing]");
    qjs.exec(js_code);
  }else{
    Serial.println("[can't load]");
    qjs.exec(jscode_default);
  }
}

void loop()
{
  // For timer, async, etc.
  if( !qjs.loop() ){
    qjs.end();

    Serial.println("now downloading");
    long ret;
    while(true){
      if( g_downloading != FILE_LOADING_NONE ){
        if( g_downloading == FILE_LOADING_MODULE ){
          ret = download_jsmodules_from_url(g_download_buffer);
          if( ret != 0 ){
            Serial.println("download_jsmodules error");
            qjs.begin();
            qjs.exec(jscode_default);
            break;
          }
        }
        if( g_downloading == FILE_LOADING_JS ){
          ret = download_jscode_from_url(g_download_buffer);
          if( ret != 0 ){
            Serial.println("download_jscode error");
            qjs.begin();
            qjs.exec(jscode_default);
            break;
          }
        }
      }
      if( g_fileloading != FILE_LOADING_NONE ){
        if( g_fileloading == FILE_LOADING_JS ){
          strcpy(js_code, g_download_buffer);

          save_jscode();
          strcat(js_code, jscode_epilogue);
        }
        if( g_fileloading == FILE_LOADING_MODULE ){
          ret = download_jsmodules_from_text(g_download_buffer);
          if( ret != 0 ){
            Serial.println("download_jscode error");
            qjs.begin();
            qjs.exec(jscode_default);
            break;
          }
        }
      }
  
      qjs.begin();
      qjs.exec(js_code);
      break;
    }
  }

  udpapi_update();
}

static long save_jscode(void)
{
  File fp = SPIFFS.open(MAIN_FNAME, FILE_WRITE);
  if( !fp )
    return -1;
  fp.write((uint8_t*)js_code, strlen(js_code) + 1);
  fp.close();

  return 0;
}

static long save_jsmodules(void)
{
  File fp = SPIFFS.open(MODULE_FNAME, FILE_WRITE);
  if( !fp )
    return -1;
  fp.write((uint8_t*)module_code, strlen(module_code) + 1 );
  fp.close();

  return 0;
}

static long download_jscode_from_url(const char *main_url)
{
  long ret;
  unsigned long jscode_len = sizeof(js_code) - strlen(jscode_epilogue) - 1;
  ret = http_get_binary(main_url, (uint8_t *)js_code, &jscode_len);
  if (ret != 0)
    return ret;

  js_code[jscode_len] = '\0';

  save_jscode();
  strcat(js_code, jscode_epilogue);

  return 0;
}

static long download_jsmodules_from_text(const char *json_text)
{
  DynamicJsonDocument doc(JSDOCUMENT_MODULES_SIZE);
  DeserializationError err = deserializeJson(doc, json_text);
  if( err ){
    Serial.printf("deserialzeJson error: %s\n", err.c_str());
    return -1;
  }

  return download_jsmodules(&doc);
}

static long download_jsmodules_from_url(const char *modules_url)
{
  DynamicJsonDocument doc(JSDOCUMENT_MODULES_SIZE);
  long ret = http_get_json(modules_url, &doc);
  if( ret != 0 )
    return ret;

  return download_jsmodules(&doc);
}

static long download_jsmodules(JsonDocument *doc)
{
  long ret;

  unsigned long load_buffer_len = 0;
  JsonArray array = (*doc)["modules"].as<JsonArray>();
  for (JsonVariant val : array){
    const char *url = val["url"];
    const char *name = val["name"];

    Serial.println(name);
    unsigned long buffer_len = sizeof(module_code) - load_buffer_len - 1;
    long ret = http_get_binary(url, (uint8_t *)&module_code[load_buffer_len], &buffer_len);
    if (ret != 0){
      Serial.println("module get error");
      return ret;
    }
    module_code[load_buffer_len + buffer_len] = '\0';

    qjs.load_module(&module_code[load_buffer_len], buffer_len, name);
    load_buffer_len += buffer_len + 1;
  }

  save_jsmodules();

  const char *url = (*doc)["main"]["url"];
  const char *name = (*doc)["main"]["name"];
  if( url == NULL )
    return 0;

  if( name )
    Serial.println(name);

  ret = download_jscode_from_url(url);
  if( ret != 0 )
    return ret;

  return 0;
}

static long load_jsmodules_and_jscode(void)
{
  module_code[0] = '\0';
  if( SPIFFS.exists(MODULE_FNAME) ){
    File fp = SPIFFS.open(MODULE_FNAME, FILE_READ);
    if( fp ){
      size_t size = fp.size();
      fp.readBytes(module_code, size);
      fp.close();
      module_code[size] = '\0';
    }
  }

  js_code[0] = '\0';
  if( SPIFFS.exists(MAIN_FNAME) ){
    File fp = SPIFFS.open(MAIN_FNAME, FILE_READ);
    if( !fp )
      return -1;
    size_t size = fp.size();
    fp.readBytes(js_code, size);
    fp.close();
    js_code[size] = '\0';

    strcat(js_code, jscode_epilogue);

    return 0;
  }else{
    return -1;
  }
}

static long wifi_connect(const char *ssid, const char *password, unsigned long timeout)
{
  Serial.println("");
  Serial.print("WiFi Connenting");

  if( ssid == NULL && password == NULL )
    WiFi.begin();
  else
    WiFi.begin(ssid, password);
  unsigned long past = 0;
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
    past += 500;
    if( past > timeout ){
      Serial.println("\nCan't Connect");
      return -1;
    }
  }
  Serial.print("\nConnected : IP=");
  Serial.print(WiFi.localIP());
  Serial.print(" Mac=");
  Serial.println(WiFi.macAddress());

  return 0;
}

static long m5_initialize(void)
{
#if defined(ARDUINO_M5Stick_C)
  M5.begin(true, true, true);
#elif defined(ARDUINO_M5STACK_Core2)
  M5.begin(true, true, true, true);
#endif
  Serial.begin(115200);
  Serial.println("[initializing]");

  long ret = wifi_connect(WIFI_SSID, WIFI_PASSWORD, WIFI_TIMEOUT);
  if( ret != 0 ){
    char ssid[32 + 1] = {'\0'};
    Serial.print("\ninput SSID:");
    Serial.setTimeout(SERIAL_TIMEOUT1);
    ret = Serial.readBytesUntil('\r', ssid, sizeof(ssid) - 1);
    if( ret <= 0 )
      return -1;

    delay(10);
    Serial.read();
    char password[32 + 1] = {'\0'};
    Serial.print("\ninput PASSWORD:");
    Serial.setTimeout(SERIAL_TIMEOUT2);
    ret = Serial.readBytesUntil('\r', password, sizeof(password) - 1);
    if( ret <= 0 )
      return -1;

    delay(10);
    Serial.read();
    Serial.printf("\nSSID=%s PASSWORD=", ssid);
    for( int i = 0 ; i < strlen(password); i++ )
      Serial.print("*");
    Serial.println("");
    ret = wifi_connect(ssid, password, WIFI_TIMEOUT);
    if( ret != 0 )
      return ret;
  }

  return 0;
}