#ifndef _ENDPOINT_UDPAPI_H_
#define _ENDPOINT_UDPAPI_H_

#include <WiFi.h>
#include "endpoint_types.h"

bool udpapi_is_wifi_connected(void);
long udpapi_initialize(void);
void udpapi_update(void);
void udpapi_appendEntry(EndpointEntry *tables, int num_of_entry);
JsonObject udpapi_makeResponse(const char *endpoint, uint32_t msgId);
long udpapi_responsePublish(IPAddress address, uint16_t port);

#ifdef _LOGIO_ENABLE_
void logio_enable(bool enable);
void logio_log(const char* message);
void logio_log2(const char* source, const char* message);
void logio_log3(const char* stream, const char* source, const char* message);
#endif

#endif
