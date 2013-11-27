#include <string.h>
#include "server_drv.h"

extern "C" {
  #include "utility/debug.h"
}

#include "WiFiRM04.h"
#include "WiFiRM04Client.h"
#include "WiFiRM04Server.h"

WiFiRM04Server::WiFiRM04Server(uint16_t port)
{
    _port = port;
}

void WiFiRM04Server::begin()
{
    uint8_t _sock = WiFiRM04Class::getSocket();
    if (_sock != NO_SOCKET_AVAIL)
    {
        ServerDrv::startServer(_port, _sock);
        WiFiRM04Class::_server_port[_sock] = _port;
        WiFiRM04Class::_state[_sock] = _sock;
    }
}

WiFiRM04Client WiFiRM04Server::available(byte* status)
{
	static int cycle_server_down = 0;
	const int TH_SERVER_DOWN = 50;

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++)
    {
        if (WiFiRM04Class::_server_port[sock] == _port)
        {
        	WiFiRM04Client client(sock);
            uint8_t _status = client.status();
            uint8_t _ser_status = this->status();

            if (status != NULL)
            	*status = _status;

            //server not in listen state, restart it
            if ((_ser_status == 0)&&(cycle_server_down++ > TH_SERVER_DOWN))
            {
            	ServerDrv::startServer(_port, sock);
            	cycle_server_down = 0;
            }

            if (_status == ESTABLISHED)
            {                
                return client;  //TODO 
            }
        }
    }

    return WiFiRM04Client(255);
}

uint8_t WiFiRM04Server::status() {
    return ServerDrv::getServerState(0);
}


size_t WiFiRM04Server::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiRM04Server::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++)
    {
        if (WiFiRM04Class::_server_port[sock] != 0)
        {
        	WiFiRM04Client client(sock);

            if (WiFiRM04Class::_server_port[sock] == _port &&
                client.status() == ESTABLISHED)
            {                
                n+=client.write(buffer, size);
            }
        }
    }
    return n;
}
