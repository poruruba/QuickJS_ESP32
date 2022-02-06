#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <unordered_map> 

#include "main_config.h"
#include "endpoint_types.h"
#include "endpoint_udpApi.h"

static StaticJsonDocument<UDP_JSON_DOCUMENT_SIZE> udpRequest;
static StaticJsonDocument<UDP_JSON_DOCUMENT_SIZE> udpResponse;
//static char udpRequestBuffer[UDP_BUFFER_SIZE];
static char udpResponseBuffer[UDP_BUFFER_SIZE];

static std::unordered_map<std::string, EndpointEntry*> endpoint_list;
static WiFiUDP udp;

#ifdef _LOGIO_ENABLE_
WiFiClient logio_client;
static char default_stream_name[2 * 6 + 1] = "";
static const char *default_source_name = "quickjs";
#endif

bool udpapi_is_wifi_connected(void)
{
  return WiFi.status() == WL_CONNECTED;
}

void udpapi_appendEntry(EndpointEntry *tables, int num_of_entry)
{
  for(int i = 0 ; i < num_of_entry ; i++ )
    endpoint_list[tables[i].name] = &tables[i];
}

long udpapi_initialize(void)
{
  if( !udpapi_is_wifi_connected() )
    return -1;

  udp.begin(UDP_REQUEST_PORT);

  if (!MDNS.begin(MDNS_NAME)){
    Serial.println("MDNS.begin error");
  }else{
    Serial.printf("MDNS_NAME: %s\n", MDNS_NAME);
    MDNS.addService(MDNS_SERVICE, "udp", UDP_REQUEST_PORT);
    Serial.printf("serivce_name: %s, UDP_PORT: %d\n", MDNS_SERVICE, UDP_REQUEST_PORT);
    MDNS.addService("http", "tcp", HTTP_PORT);
    Serial.printf("serivce_name: %s, TCP_PORT: %d\n", "http", HTTP_PORT);
  }

  return 0;
}

void udpapi_update(void)
{
  if( !udpapi_is_wifi_connected() )
    return;

  int packetSize = udp.parsePacket();
  if( packetSize <= 0 || packetSize > sizeof(g_download_buffer))
    return;

  int len = udp.read(g_download_buffer, packetSize);
  if( len <= 0 )
    return;
  Serial.printf("UDP received(%d)\n", len);

  DeserializationError err = deserializeJson(udpRequest, g_download_buffer, len);
  if( err ){
    Serial.printf("deserializeJson error: %s\n", err.c_str());
    return;
  }

  const char *endpoint = udpRequest["endpoint"];
  bool oneway = udpRequest["oneway"];
  uint16_t respPort = udpRequest["respPort"];
  uint32_t msgId = udpRequest["msgId"];
  Serial.printf("endpoint: %s\n", endpoint);
  std::unordered_map<std::string, EndpointEntry*>::iterator itr = endpoint_list.find(endpoint);
  if( itr != endpoint_list.end() ){
    EndpointEntry *entry = itr->second;
    JsonObject responseResult = udpapi_makeResponse(endpoint, msgId);
    long ret = entry->impl(udpRequest["params"], responseResult, entry->magic);
    if( ret != 0 ){
      responseResult = udpapi_makeResponse(endpoint, msgId);
      responseResult["status"] = "NG";
      responseResult["message"] = "unknown";
    }
    if( !oneway )
      udpapi_responsePublish(udp.remoteIP(), respPort == 0 ? UDP_REPORT_PORT : respPort);

    return;
  }

  Serial.println("endpoint not found");
}

JsonObject udpapi_makeResponse(const char *endpoint, uint32_t msgId)
{
  JsonObject jsonObject = udpResponse.to<JsonObject>();
  jsonObject["client"]["ipaddress"] = (char*)WiFi.localIP().toString().c_str();
  jsonObject["status"] = "OK";
  jsonObject["msgId"] = msgId;
  jsonObject["endpoint"] = (char*)endpoint;

  return jsonObject;
}

long udpapi_responsePublish(IPAddress address, uint16_t port)
{
  long length = serializeJson(udpResponse, udpResponseBuffer, sizeof(udpResponseBuffer));
  if( length <= 0 ){
    Serial.println("Udp buffer size over");
    return -1;
  }

  udp.beginPacket(address, port);
  udp.write((uint8_t*)udpResponseBuffer, strlen(udpResponseBuffer));
  udp.endPacket();

  return 0;
}

#ifdef _LOGIO_ENABLE_
static bool enable_logio = false;

void logio_enable(bool enable){
  enable_logio = enable;
}

void logio_log(const char* message) {
  logio_log3(default_stream_name, default_source_name, message);
}

void logio_log2(const char* source, const char* message) {
  logio_log3(default_stream_name, source, message);
}

void logio_log3(const char* stream, const char* source, const char* message) {
  if( !enable_logio )
    return;
    
  if( !logio_client.connected() ){
    if( default_stream_name[0] == '\0'){
      uint8_t address[6];
      WiFi.macAddress(address);
      sprintf(default_stream_name, "%02x%02x%02x%02x%02x%02x", address[0], address[1], address[2], address[3], address[4], address[5]);
    }
    if( !logio_client.connect(LOGIO_HOST, LOGIO_PORT) ){
      Serial.println("connection failed");
      return;
    }
  }
  if( logio_client.connected() ){
    String packet = "+msg|";
    packet += stream;
    packet += "|";
    packet += source;
    packet +="|";
    packet += message;
    logio_client.write(packet.c_str(), strlen(packet.c_str()) + 1);
    logio_client.flush();
  }
}
#endif