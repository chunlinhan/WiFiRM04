#ifndef AT_DRV_H
#define AT_DRV_H

#include "wifi_spi.h"
#include <QueueList.h>

#define SERIAL_TIMEOUT			50

#define ENC_TYPE_NONE			0
#define ENC_TYPE_WEP_OPEN		1
#define ENC_TYPE_WEP			2
#define ENC_TYPE_WPA_TKIP		3
#define ENC_TYPE_WPA_AES		4
#define ENC_TYPE_WPA2_AES		5
#define ENC_TYPE_WPAWPA2_TKIP	6
#define ENC_TYPE_WPAWPA2_AES	7
#define ENC_TYPE_AUTO 			8


#define NET_MODE_DEFAULT		0
#define NET_MODE_ETH			1
#define NET_MODE_WIFI_CLIENT	2
#define NET_MODE_WIFI_AP		3


#define MODE_NONE				0
#define MODE_CLIENT				1
#define MODE_SERVER				2

#define PROTO_MODE_TCP			0
#define PROTO_MODE_UDP			1

typedef enum LinuxTcpState {
	TCP_ESTABLISHED = 1,
	TCP_SYN_SENT,
	TCP_SYN_RECV,
	TCP_FIN_WAIT1,
	TCP_FIN_WAIT2,
	TCP_TIME_WAIT,
	TCP_CLOSE,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_LISTEN,
	TCP_CLOSING,    /* Now a valid state */

	TCP_MAX_STATES  /* Leave at the end! */
};

class AtDrv
{
private:
	static HardwareSerial serialPort[2];
	static bool atMode;
	static uint16_t sockPort[2];
	static QueueList<uint8_t> sock0DataQueue;
	static bool sockConnected[2];
	static bool switchToAtMode();
	static bool switchToDataMode(long timeout = SERIAL_TIMEOUT);
	static void setAtMode(bool mode);
	static bool echoTest(long timeout = SERIAL_TIMEOUT);
	static void clearSerialRxData();
	static bool setWiFiConfig(char *ssid, int type, const char *password, long timeout = SERIAL_TIMEOUT);
	static bool getRemoteIp(uint8_t sock, uint8_t *ip, long timeout = SERIAL_TIMEOUT);
	static bool getLocalPort(uint8_t sock, uint16_t *port, long timeout = SERIAL_TIMEOUT);
	static bool getRemotePort(uint8_t sock, uint16_t *port, long timeout = SERIAL_TIMEOUT);
	static bool setNetMode(int netMode, long timeout = SERIAL_TIMEOUT);
	static void netCommit();
	static bool reConnect(long timeout = SERIAL_TIMEOUT);
	static bool getMode(uint8_t sock, int *mode, long timeout = SERIAL_TIMEOUT);
	static bool setMode(uint8_t sock, int mode, long timeout = SERIAL_TIMEOUT);
	static bool getProtocol(uint8_t sock, uint8_t *protocol, long timeout = SERIAL_TIMEOUT);
	static bool setProtocol(uint8_t sock, uint8_t protocol, long timeout = SERIAL_TIMEOUT);
	static bool setLocalPort(uint8_t sock, uint16_t port, long timeout = SERIAL_TIMEOUT);
	static bool setPort(uint8_t sock, uint16_t port, long timeout = SERIAL_TIMEOUT);
	static bool setRemoteIp(uint8_t sock, uint32_t ip, long timeout = SERIAL_TIMEOUT);
	static bool getRemoteHost(uint8_t sock, char *host, long timeout = SERIAL_TIMEOUT);
	static bool setRemoteHost(uint8_t sock, const char *host, long timeout = SERIAL_TIMEOUT);
	static bool setDhcpd(bool enable, long timeout = SERIAL_TIMEOUT);
	static bool setDhcpdIp(uint32_t ipStart, uint32_t ipEnd, uint32_t mask, uint32_t gateway, long timeout = SERIAL_TIMEOUT);
	static bool setDhcpdDns(uint32_t dns1, uint32_t dns2, long timeout = SERIAL_TIMEOUT);
	static bool setDhcpdTime(uint32_t time, long timeout = SERIAL_TIMEOUT);
	static bool setNetIp(uint32_t ip, uint32_t mask, uint32_t gateway, long timeout= SERIAL_TIMEOUT);
	static bool setNetDns(uint32_t dns1, uint32_t dns2, long timeout = SERIAL_TIMEOUT);
	static bool getTcpAuto(uint8_t sock, bool *enable, long timeout = SERIAL_TIMEOUT);
	static bool setTcpAuto(uint8_t sock, bool enable, long timeout = SERIAL_TIMEOUT);
	static bool getNetworkTimeout(uint8_t sock, uint32_t *time, long timeout = SERIAL_TIMEOUT);
	static bool setNetworkTimeout(uint8_t sock, uint32_t time, long timeout = SERIAL_TIMEOUT);
	static uint8_t portStateMapping(LinuxTcpState linuxPortState);
	static void getLocalIp(uint8_t *ip, long timeout = SERIAL_TIMEOUT*5);
	static void getNetmask(uint8_t *mask, long timeout = SERIAL_TIMEOUT*5);
	static void getGateway(uint8_t *gwip, long timeout = SERIAL_TIMEOUT*5);
  
public:
	AtDrv();
	static bool isAtMode();
    static void begin();
    static void end();
	static void getRemoteData(uint8_t sock, uint8_t *ip, uint16_t *port);
	static bool getMAC(uint8_t *mac, long timeout = SERIAL_TIMEOUT);
	static bool setApSSID(char *ssid, int type, const char *password);
	static bool setSSID(char *ssid, int type, const char *password = NULL);
	static void startServer(uint8_t sock, uint16_t port, uint8_t protMode);
	static void startClient(uint8_t sock, uint32_t ipAddress, uint16_t port, uint8_t protMode);
	static void startClient(uint8_t sock, const char *host, uint16_t port, uint8_t protMode);
	static uint16_t availData(uint8_t sock);
	static int peek(uint8_t sock);
	static int read(uint8_t sock);
	static uint16_t readBytes(uint8_t sock, uint8_t *_data, uint16_t *_dataLen);
	static uint16_t write(uint8_t sock, const uint8_t *data, uint16_t _len);
	static uint8_t getClientState(uint8_t sock, long timeout = SERIAL_TIMEOUT*5);
	static void stopClient(uint8_t sock);
	static bool isWiFiConnected(long timeout = SERIAL_TIMEOUT);
	static void getNetworkData(uint8_t *ip, uint8_t *mask, uint8_t *gwip);
	static bool disconnect();
	static uint8_t getScanNetworks(char _networkSsid[][WL_SSID_MAX_LENGTH+1], int32_t _networkRssi[], uint8_t _networkEncr[], uint8_t maxItems, long timeout = SERIAL_TIMEOUT*20);
	static void getCurrentSSID(char _ssid[], long timeout = SERIAL_TIMEOUT*5);
	static void getCurrentBSSID(uint8_t _ssid[], long timeout = SERIAL_TIMEOUT*5);
	static int32_t getCurrentRSSI(long timeout = SERIAL_TIMEOUT*5);
	static uint8_t getCurrentEncryptionType(long timeout = SERIAL_TIMEOUT*5);
	static void getFwVersion(char fwVersion[], uint8_t bufLength, long timeout = SERIAL_TIMEOUT);
	static uint8_t checkDataSent(uint8_t sock);
};

extern AtDrv atDrv;

#endif
