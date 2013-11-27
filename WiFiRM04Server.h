#ifndef WIFI_RM04_SERVER_H
#define WIFI_RM04_SERVER_H

extern "C" {
  #include "utility/wl_definitions.h"
}

#include "Server.h"

class WiFiRM04Client;

class WiFiRM04Server : public Server {
private:
  uint16_t _port;
  void*     pcb;
public:
  WiFiRM04Server(uint16_t);
  WiFiRM04Client available(uint8_t* status = NULL);
  void begin();
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  uint8_t status();

  using Print::write;
};

#endif
