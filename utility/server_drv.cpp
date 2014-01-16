//#define _DEBUG_

#include "server_drv.h"

#include "Arduino.h"
#include "at_drv.h"

extern "C" {
#include "wl_types.h"
#include "debug.h"
}


// Start server TCP on port specified
void ServerDrv::startServer(uint16_t port, uint8_t sock, uint8_t protMode)
{
    AtDrv::startServer(sock, port, protMode);
}

// Start server TCP on port specified
void ServerDrv::startClient(uint32_t ipAddress, uint16_t port, uint8_t sock, uint8_t protMode)
{
    AtDrv::startClient(sock, ipAddress, port, protMode);
}

// Start server TCP on port specified
void ServerDrv::startClient(const char *host, uint16_t port, uint8_t sock, uint8_t protMode)
{
    AtDrv::startClient(sock, host, port, protMode);
}

// Start server TCP on port specified
void ServerDrv::stopClient(uint8_t sock)
{
	AtDrv::stopClient(sock);
}


uint8_t ServerDrv::getServerState(uint8_t sock)
{
    // Since RM04 can't get current connection status, we always return LISTEN
    return LISTEN;
}

uint8_t ServerDrv::getClientState(uint8_t sock)
{
	return AtDrv::getClientState(sock);
}

uint16_t ServerDrv::availData(uint8_t sock)
{
    return AtDrv::availData(sock);
}

bool ServerDrv::getData(uint8_t sock, uint8_t *data, uint8_t peek)
{
    int ret;

    if(!data)
      return false;

    if(peek)
      ret = AtDrv::peek(sock);
    else
      ret = AtDrv::read(sock);

    *data = (uint8_t) ret;
#ifdef _DEBUG_
	if(ret >= 0)
		Serial.write(*data);
#endif
    return (ret < 0)? false:true;
}

bool ServerDrv::getDataBuf(uint8_t sock, uint8_t *_data, uint16_t *_dataLen)
{
    uint16_t ret;

    ret = AtDrv::readBytes(sock, _data, _dataLen);
#ifdef _DEBUG_
	if(ret)
		Serial.write(_data, *_dataLen);
#endif
    return (ret)? true:false;
}

bool ServerDrv::insertDataBuf(uint8_t sock, const uint8_t *data, uint16_t _len)
{
    // Not Support
    return false;
}

bool ServerDrv::sendUdpData(uint8_t sock)
{
    // Not Support
    return false;
}

bool ServerDrv::sendData(uint8_t sock, const uint8_t *data, uint16_t len)
{
    uint16_t ret;

    ret = AtDrv::write(sock, data, len);
#ifdef _DEBUG_
	Serial.write(data, len);
#endif
    return (ret)? true:false;
}


uint8_t ServerDrv::checkDataSent(uint8_t sock)
{
    return AtDrv::checkDataSent(sock);
}

ServerDrv serverDrv;
