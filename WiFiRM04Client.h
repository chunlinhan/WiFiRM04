#ifndef WIFI_RM04_CLIENT_H
#define WIFI_RM04_CLIENT_H
#include "Arduino.h"	
#include "Print.h"
#include "Client.h"
#include "IPAddress.h"

class WiFiRM04Client : public Client {

public:
  WiFiRM04Client();
  WiFiRM04Client(uint8_t sock);

  uint8_t status();
  virtual int connect(IPAddress ip, uint16_t port);
  virtual int connect(const char *host, uint16_t port);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual int available();
  virtual int read();
  virtual int read(uint8_t *buf, size_t size);
  virtual int peek();
  virtual void flush();
  virtual void stop();
  virtual uint8_t connected();
  virtual operator bool();

  friend class WiFiRM04Server;

  using Print::write;

private:
  static uint16_t _srcport;
  uint8_t _sock;   //not used
  uint16_t  _socket;

  uint8_t getFirstSocket();
};

#endif
