extern "C" {
  #include "utility/wl_definitions.h"
  #include "utility/wl_types.h"
  #include "socket.h"
  #include "string.h"
  #include "utility/debug.h"
}

#include "WiFiRM04.h"
#include "WiFiRM04Client.h"
#include "WiFiRM04Server.h"
#include "server_drv.h"


uint16_t WiFiRM04Client::_srcport = 1024;

WiFiRM04Client::WiFiRM04Client() : _sock(MAX_SOCK_NUM) {
}

WiFiRM04Client::WiFiRM04Client(uint8_t sock) : _sock(sock) {
    if(sock >= MAX_SOCK_NUM) {
        _sock = 255;
    }
}

int WiFiRM04Client::connect(const char* host, uint16_t port) {
    if(_sock >= MAX_SOCK_NUM)
		_sock = getFirstSocket();
    if (_sock != NO_SOCKET_AVAIL)
    {
    	ServerDrv::startClient(host, port, _sock);
    	WiFiRM04Class::_state[_sock] = _sock;

    	unsigned long start = millis();

    	// wait 4 second for the connection to close
    	while (!connected() && millis() - start < 10000)
    		delay(1);

    	if (!connected())
       	{
    		return 0;
    	}
    }else{
    	Serial.println("No Socket available");
    	return 0;
    }
    return 1;
}

int WiFiRM04Client::connect(IPAddress ip, uint16_t port) {
    if(_sock == MAX_SOCK_NUM)
		_sock = getFirstSocket();
    if (_sock != NO_SOCKET_AVAIL)
    {
    	ServerDrv::startClient(uint32_t(ip), port, _sock);
    	WiFiRM04Class::_state[_sock] = _sock;

    	unsigned long start = millis();

    	// wait 4 second for the connection to close
    	while (!connected() && millis() - start < 10000)
    		delay(1);

    	if (!connected())
       	{
    		return 0;
    	}
    }else{
    	Serial.println("No Socket available");
    	return 0;
    }
    return 1;
}

size_t WiFiRM04Client::write(uint8_t b) {
	  return write(&b, 1);
}

size_t WiFiRM04Client::write(const uint8_t *buf, size_t size) {
  if (_sock >= MAX_SOCK_NUM)
  {
	  setWriteError();
	  return 0;
  }
  if (size==0)
  {
	  setWriteError();
      return 0;
  }


  if (!ServerDrv::sendData(_sock, buf, size))
  {
	  setWriteError();
      return 0;
  }
  if (!ServerDrv::checkDataSent(_sock))
  {
	  setWriteError();
      return 0;
  }

  return size;
}

int WiFiRM04Client::available() {
  if (_sock != 255)
  {
      return ServerDrv::availData(_sock);
  }
   
  return 0;
}

int WiFiRM04Client::read() {
  uint8_t b;
  if (!available())
    return -1;

  ServerDrv::getData(_sock, &b);
  return b;
}


int WiFiRM04Client::read(uint8_t* buf, size_t size) {
  if (!ServerDrv::getDataBuf(_sock, buf, &size))
      return -1;
  return 0;
}

int WiFiRM04Client::peek() {
	  uint8_t b;
	  if (!available())
	    return -1;

	  ServerDrv::getData(_sock, &b, 1);
	  return b;
}

void WiFiRM04Client::flush() {
  while (available())
    read();
}

void WiFiRM04Client::stop() {

  if (_sock == 255)
    return;

  ServerDrv::stopClient(_sock);
  WiFiRM04Class::_state[_sock] = NA_STATE;

  int count = 0;
  // wait maximum 5 secs for the connection to close
  // Don't wait since RM04 can't disconnect the connetion by itself
  //while (status() != CLOSED && ++count < 50)
  //  delay(100);

  _sock = 255;
}

uint8_t WiFiRM04Client::connected() {

  if (_sock == 255) {
    return 0;
  } else {
    uint8_t s = status();

    return !(s == LISTEN || s == CLOSED || s == FIN_WAIT_1 ||
    		s == FIN_WAIT_2 || s == TIME_WAIT ||
    		s == SYN_SENT || s== SYN_RCVD ||
    		(s == CLOSE_WAIT));
  }
}

uint8_t WiFiRM04Client::status() {
    if (_sock == 255) {
    return CLOSED;
  } else {
    return ServerDrv::getClientState(_sock);
  }
}

WiFiRM04Client::operator bool() {
  return _sock != 255;
}

// Private Methods
uint8_t WiFiRM04Client::getFirstSocket()
{
    for (int i = 0; i < MAX_SOCK_NUM; i++) {
      if (WiFiRM04Class::_state[i] == NA_STATE)
      {
          return i;
      }
    }
    return SOCK_NOT_AVAIL;
}

