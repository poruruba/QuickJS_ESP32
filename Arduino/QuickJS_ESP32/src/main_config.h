#ifndef _MAIN_CONFIG_H_
#define _MAIN_CONFIG_H_

#include <Arduino.h>
#if defined(ARDUINO_M5Stick_C)
#include <M5StickC.h>
#elif defined(ARDUINO_M5STACK_Core2)
#include <M5Core2.h>
#endif

//#define _HTTP_PROXY_ENABLE_
//#define _LOGIO_ENABLE_
//#define _MQTT_ENABLE_
#define _LCD_ENABLE_
#define _RTC_ENABLE_
#define _IMU_ENABLE_
#if defined(ARDUINO_M5STACK_Core2)
#define _SD_ENABLE_
#endif

//#define WIFI_SSID "【固定のWiFiアクセスポイントのSSID】" // WiFiアクセスポイントのSSID
//#define WIFI_PASSWORD "【固定のWiFIアクセスポイントのパスワード】" // WiFIアクセスポイントのパスワード
#define WIFI_SSID NULL // WiFiアクセスポイントのSSID
#define WIFI_PASSWORD NULL // WiFIアクセスポイントのパスワード

#ifdef _LOGIO_ENABLE_
#define LOGIO_HOST  "【log.ioのホスト名】" // log.ioのホスト名
#define LOGIO_PORT  6689 // log.ioのポート番号
#endif

#ifdef _HTTP_PROXY_ENABLE_
#define HTTPPROXY_URL "【HTTP中継サーバのURL】" // HTTP中継サーバのURL
#endif

#ifdef _MQTT_ENABLE_
#define MQTT_BROKER_URL "【MQTTブローカのホスト名】"  // MQTTブローカのホスト名
#define MQTT_BROKER_PORT  1883 // MQTTブローカのポート番号(TCP接続)
#endif

#define HTTP_PORT 80

#define WIFI_TIMEOUT  10000
#define SERIAL_TIMEOUT1  10000
#define SERIAL_TIMEOUT2  20000

#define MDNS_NAME "QuickJS_ESP32" // mDNSサービスホスト名
#define MDNS_SERVICE "quickjs" // mDNSサービスタイプ

#define UDP_BUFFER_SIZE  1500
#define UDP_JSON_DOCUMENT_SIZE  1500

#define UDP_REQUEST_PORT  3333 //Node.jsサーバからのUDP受信を待ち受けるポート番号
#define UDP_REPORT_PORT   3333 //Node.jsサーバ(UDP)へUDP送信する先のポート番号

#define JSCODE_BUFFER_SIZE 10000
#define FILE_BUFFER_SIZE 10000
#define JSMODULE_BUFFER_SIZE 20000
#define JSDOCUMENT_MODULES_SIZE 2000

#define FILE_LOADING_NONE     0
#define FILE_LOADING_RESTART  1
#define FILE_LOADING_JS       2
#define FILE_LOADING_MODULE   3
extern unsigned char g_downloading;
extern unsigned char g_fileloading;
extern char g_download_buffer[FILE_BUFFER_SIZE];

#endif
