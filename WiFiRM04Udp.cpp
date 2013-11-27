
extern "C" {
  #include "utility/debug.h"
  #include "utility/wifi_spi.h"
}
#include <string.h>
#include "server_drv.h"
#include "wifi_drv.h"

#include "WiFiRM04.h"
#include "WiFiRM04Udp.h"
#include "WiFiRM04Client.h"
#include "WiFiRM04Server.h"


/* Constructor */
WiFiRM04UDP::WiFiRM04UDP() : _sock(NO_SOCKET_AVAIL) {}

/* Start WiFiRM04UDP socket, listening at local port PORT */
uint8_t WiFiRM04UDP::begin(uint16_t port) {

    uint8_t sock = WiFiRM04Class::getSocket();
    if (sock != NO_SOCKET_AVAIL)
    {
        ServerDrv::startServer(port, sock, UDP_MODE);
        WiFiRM04Class::_server_port[sock] = port;
        _sock = sock;
        _port = port;
        return 1;
    }
    return 0;

}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int WiFiRM04UDP::available() {
	 if (_sock != NO_SOCKET_AVAIL)
	 {
	      return ServerDrv::availData(_sock);
	 }
	 return 0;
}

/* Release any resources being used by this WiFiRM04UDP instance */
void WiFiRM04UDP::stop()
{
	  if (_sock == NO_SOCKET_AVAIL)
	    return;

	  ServerDrv::stopClient(_sock);

	  _sock = NO_SOCKET_AVAIL;
}

int WiFiRM04UDP::beginPacket(const char *host, uint16_t port)
{
	// Look up the host first
	int ret = 0;
	IPAddress remote_addr;
	if (WiFi.hostByName(host, remote_addr))
	{
		return beginPacket(remote_addr, port);
	}
	return ret;
}

int WiFiRM04UDP::beginPacket(IPAddress ip, uint16_t port)
{
  if (_sock == NO_SOCKET_AVAIL)
	  _sock = WiFiRM04Class::getSocket();
  if (_sock != NO_SOCKET_AVAIL)
  {
	  ServerDrv::startClient(uint32_t(ip), port, _sock, UDP_MODE);
	  WiFiRM04Class::_state[_sock] = _sock;
	  return 1;
  }
  return 0;
}

int WiFiRM04UDP::endPacket()
{
	return ServerDrv::sendUdpData(_sock);
}

size_t WiFiRM04UDP::write(uint8_t byte)
{
  return write(&byte, 1);
}

size_t WiFiRM04UDP::write(const uint8_t *buffer, size_t size)
{
	ServerDrv::insertDataBuf(_sock, buffer, size);
	return size;
}

int WiFiRM04UDP::parsePacket()
{
	return available();
}

int WiFiRM04UDP::read()
{
  uint8_t b;
  if (available())
  {
	  ServerDrv::getData(_sock, &b);
  	  return b;
  }else{
	  return -1;
  }
}

int WiFiRM04UDP::read(unsigned char* buffer, size_t len)
{
  if (available())
  {
	  size_t size = 0;
	  if (!ServerDrv::getDataBuf(_sock, buffer, &size))
		  return -1;
	  // TODO check if the buffer is too smal respect to buffer size
	  return size;
  }else{
	  return -1;
  }
}

int WiFiRM04UDP::peek()
{
  uint8_t b;
  if (!available())
    return -1;

  ServerDrv::getData(_sock, &b, 1);
  return b;
}

void WiFiRM04UDP::flush()
{
  while (available())
    read();
}

IPAddress  WiFiRM04UDP::remoteIP()
{
	uint8_t _remoteIp[4] = {0};
	uint8_t _remotePort[2] = {0};

	WiFiDrv::getRemoteData(_sock, _remoteIp, _remotePort);
	IPAddress ip(_remoteIp);
	return ip;
}

uint16_t  WiFiRM04UDP::remotePort()
{
	uint8_t _remoteIp[4] = {0};
	uint8_t _remotePort[2] = {0};

	WiFiDrv::getRemoteData(_sock, _remoteIp, _remotePort);
	uint16_t port = (_remotePort[0]<<8)+_remotePort[1];
	return port;
}

