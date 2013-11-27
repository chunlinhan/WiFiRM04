#include <stdio.h>
#include <string.h>

#include "Arduino.h"
#include "IPAddress.h"
#include "at_drv.h"                   
#include "pins_arduino.h"
#define _DEBUG_
extern "C" {
#include "debug.h"
}

// define USE_ESCAPE_PIN to use HW pin to switch mode
#define USE_ESCAPE_PIN
// use this pin to pull up/down module's ES/RTS pin to switch mode
#define ESCAPE_PIN				21
#define ESCAPE_PIN_ACTIVE		LOW

// CHECK_TCP_STATE is an experimental feature, disable it by default
// #define CHECK_TCP_STATE

// Suggest to use 38400, even using hardware UART
#define DEFAULT_BAUD1			38400
#define DEFAULT_BAUD2			38400

// default Tes time should be 100 ms
#define TES_TIME_IN_MS			100
// defualt Tpt time should be 10ms
#define TPT_TIME_IN_MS			10

#define MAX_CMD_BUF_SIZE		64
#define MAX_IP_BUF_SIZE			16
#define MAX_WIFI_CONF_BUF_SIZE	64
#define MAX_RESP_BUF_SIZE		64
#define MAX_HOST_NAME_BUF_SIZE	300
#define MAX_TEMP_BUF_SIZE		64
#define MAX_DHCPD_IP_BUF_SIZE	128
#define MAX_LINE_BUF_SIZE		512

// use Serial1 as default serial port to communicate with WiFi module
#define AT_DRV_SERIAL Serial1
// use Serial2 to communicate the uart2 of our WiFi module
#define AT_DRV_SERIAL1 Serial2

// define the client socket port #
#define SOCK0_LOCAL_PORT		1024
#define SOCK1_LOCAL_PORT		1025


const uint16_t localSockPort[] = {SOCK0_LOCAL_PORT, SOCK1_LOCAL_PORT};

char at_ok[] = "ok\r\n";
char at_Connected[] = "Connected\r\n";
char at_out_trans[] = "at+out_trans=0\r\r";
char *at_remoteip[2] = {"at+remoteip=%s\r\r", "at+C2_remoteip=%s\r\r"};
char *at_remoteport[2] = {"at+remoteport=%s\r\r", "at+C2_port=%s\r\r"};
char *at_CLport[2] = {"at+CLport=%s\r\r", "at+C2_CLport=%s\r\r"};
char at_wifi_conf[] = "at+wifi_conf=%s\r\r";
char at_wifi_ConState[] = "at+wifi_ConState=?\r\r";
char at_Get_MAC[] = "at+Get_MAC=?\r\r";
char at_netmode[] = "at+netmode=%c\r\r";
char at_net_commit[] = "at+net_commit=1\r\r";
char at_reconn[] = "at+reconn=1\r\r";
char *at_mode[2] = {"at+mode=%s\r\r" , "at+C2_mode=%s\r\r"};
char *at_remotepro[2] = {"at+remotepro=%s\r\r", "at+C2_protocol=%s\r\r"};
char at_dhcpd[] = "at+dhcpd=%c\r\r";
char at_dhcpd_ip[] = "at+dhcpd_ip=%s\r\r";
char at_dhcpd_dns[] = "at+dhcpd_dns=%s\r\r";
char at_dhcpd_time[] = "at+dhcpd_time=%s\r\r";
char at_net_ip[] = "at+net_ip=%s\r\r";
char at_net_dns[] = "at+net_dns=%s\r\r";
char *at_tcp_auto[2] = {"at+tcp_auto=%c\r\r" , "at+C2_tcp_auto=%c\r\r"};
char *at_timeout[2] = {"at+timeout=%s\r\r", "at+C2_timeout=%s\r\r"};
char at_tcp_port_stat[] = "at+excxxx=cat /proc/net/tcp | grep '[0-9]: [^0].*:%04X'> /dev/ttyS1";
char at_get_ip[] = "at+excxxx=ifconfig ra0 | grep 'inet addr:' | sed -e 's/inet addr:\\(.*\\).*Bcast:.*Mask:.*/\\1/' > /dev/ttyS1";
char at_get_mask[] = "at+excxxx=ifconfig ra0 | grep 'inet addr:' | sed -e 's/.*Mask:\\(.*\\)/\\1/' > /dev/ttyS1";
char at_get_gateway[] = "at+excxxx=cat /proc/net/route  | grep -e 'ra0[[:space:]]\\+00000000' > /dev/ttyS1";
char at_get_ssid[] = "at+excxxx=iwconfig ra0 | grep ESSID | sed -e 's/.*ESSID:\"\\(.*\\)\".*Nickname:\".*\"/\\1/'  > dev/ttyS1";
char at_get_bssid[] = "at+excxxx=iwconfig ra0 | grep 'Access Point' | sed -e 's/.*Access Point: \\(.*\\)/\\1/'  > dev/ttyS1";
char at_get_rssi[] = "at+excxxx=iwconfig ra0 | grep 'Link Quality' | sed -e 's/.*Link Quality=\\(.*\\)\\/.*/\\1/'  > dev/ttyS1";
char at_get_enc[] = "at+excxxx=iwpriv ra0 show EncrypType | sed -e 's/ra0.*show:[[:space:]]\\(.*\\).*/\\1/'  > dev/ttyS1";
char at_wifi_Scan[] = "at+wifi_Scan=?\r\r";
char at_ver[] = "at+ver=?\r\r";

void AtDrv::begin()
{
	serialPort[0].begin(DEFAULT_BAUD1);
	serialPort[1].begin(DEFAULT_BAUD2);
	clearSerialRxData();
}

void AtDrv::end() {
	serialPort[0].end();
	serialPort[1].end();
}

bool AtDrv::isAtMode()
{
   return atMode;
}

void AtDrv::setAtMode(bool mode)
{
    atMode = mode;
}

bool AtDrv::echoTest(long timeout)
{
	char buf[MAX_TEMP_BUF_SIZE+1];
	int i;
	char *token;
  
	clearSerialRxData();
	serialPort[0].println("#32767");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	i = serialPort[0].readBytes(buf, MAX_TEMP_BUF_SIZE);
	if(i == 0) {
		INFO1("Echo No resp");
		return false;
	}

	buf[i] = NULL;

	token = buf;
	while(*token && *token++ != '#');

	int ret = strtol(token, NULL, 10);

	clearSerialRxData();
	return (ret == 32767)? true:false;
}

void AtDrv::clearSerialRxData()
{
	while(serialPort[0].available())
		serialPort[0].read();
}

#ifdef USE_ESCAPE_PIN
bool AtDrv::switchToAtMode()
{
	int retryCount = 0;
	// flush tx buffer's data first
	serialPort[0].flush();
retry:
	digitalWrite(ESCAPE_PIN, ESCAPE_PIN_ACTIVE);
	// according to user manual, delay should be > Tes time, so we plus 50ms here
	delay(TES_TIME_IN_MS+50*(retryCount+1));
	digitalWrite(ESCAPE_PIN, !ESCAPE_PIN_ACTIVE);

	// MUST preserve all data in rx buffer here, or these data would be mixed with at command response
	// TODO: QueueList would cause heap fragmentation?
	if(retryCount == 0 && sockConnected[0] == true) {
		while(serialPort[0].available()) {
			sock0DataQueue.push(serialPort[0].read());
		}
	}

	if(echoTest()) {
		setAtMode(true);
		return true;
	}
	else
		setAtMode(false);

	if(retryCount++ < 5)
		goto retry;
		
	return false;
}

#else

bool AtDrv::switchToAtMode()
{
	// flush tx buffer's data first
	serialPort[0].flush();
	// preserve all data in rx buffer, or these data would be mixed with at command response
	// TODO: QueueList would cause heap fragmentation?
	if(sockConnected[0] == true) {
		while(serialPort[0].available()) {
			sock0DataQueue.push(serialPort[0].read());
		}
	}

	if(isAtMode() && echoTest())
		return true;
	
	// flush all first
	serialPort[0].flush();
	// according to user manual, delay should be greater than Tpt, so we plus 5ms here
	delay(TPT_TIME_IN_MS+5);
	serialPort[0].write(0x2b);
	// flush every single byte to prevent buffering effect and cause wrong timing sequence
	serialPort[0].flush();
	delay(TPT_TIME_IN_MS+5);
	serialPort[0].write(0x2b);
	serialPort[0].flush();
	delay(TPT_TIME_IN_MS+5);
	serialPort[0].write(0x2b);
	serialPort[0].flush();
	// according to user manual, shuold be > 400ms  < 600ms, so we use 500ms
	delay(500);
	serialPort[0].write(0x1b);
	serialPort[0].flush();
	delay(TPT_TIME_IN_MS+5);
	serialPort[0].write(0x1b);
	serialPort[0].flush();
	//delayMicroseconds(tptDealy);
	delay(TPT_TIME_IN_MS+5);
	serialPort[0].write(0x1b);
	serialPort[0].flush();
	delay(TPT_TIME_IN_MS+5);
	
	if(echoTest()) {
		setAtMode(true);
		return true;
	}
	else
		setAtMode(false);

	return false;
}
#endif

bool AtDrv::switchToDataMode(long timeout)
{
	int retryCount = 0;

	if(!isAtMode())
		return true;
retry:
	clearSerialRxData();
	INFO1(at_out_trans);
	serialPort[0].print(at_out_trans);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(serialPort[0].find(at_ok)) {
		setAtMode(false);
		return true;
	}

	if(retryCount++ < 5)
		goto retry;

	return false;
}

bool AtDrv::getRemoteIp(uint8_t sock, uint8_t *ip, long timeout)
{
	bool ret = false;
	char cmdBuf[MAX_CMD_BUF_SIZE];

	sprintf(cmdBuf, at_remoteip[sock], "?");
	clearSerialRxData();
	INFO1(cmdBuf);
	serialPort[0].print(cmdBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].findUntil(cmdBuf, "\r\n")) {
		INFO1("fail to get remote IP");
		ip[0] = ip[1] = ip[2] = ip[3] = 0;
		goto end;
	}
	
	ip[0] = serialPort[0].parseInt();
	ip[1] = serialPort[0].parseInt();
	ip[2] = serialPort[0].parseInt();
	ip[3] = serialPort[0].parseInt();
	
	INFO("Remote IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	
	clearSerialRxData();
	
	ret = true;
end:
	return ret;
}

bool AtDrv::getLocalPort(uint8_t sock, uint16_t *port, long timeout)
{
	bool ret = false;
	char cmdBuf[MAX_CMD_BUF_SIZE];

	sprintf(cmdBuf, at_CLport[sock], "?");
	clearSerialRxData();
	INFO1(cmdBuf);
	serialPort[0].print(cmdBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(cmdBuf)) {
		INFO1("fail to get remote port");
		*port = 0;
		goto end;
	}
	
	*port = serialPort[0].parseInt();
	
	INFO("Local port: %d", *port);
	
	clearSerialRxData();
	
	ret = true;
end:
	return ret;
}

bool AtDrv::getRemotePort(uint8_t sock, uint16_t *port, long timeout)
{
	bool ret = false;
	char cmdBuf[MAX_CMD_BUF_SIZE];

	sprintf(cmdBuf, at_remoteport[sock], "?");
	clearSerialRxData();
	INFO1(cmdBuf);
	serialPort[0].print(cmdBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(cmdBuf)) {
		INFO1("fail to get remote port");
		*port = 0;
		goto end;
	}
	
	*port = serialPort[0].parseInt();
	
	INFO("Remote port: %d", *port);
	
	clearSerialRxData();
	
	ret = true;
end:
	return ret;
}

void AtDrv::getRemoteData(uint8_t sock, uint8_t *ip, uint16_t *port)
{
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			memset(ip, 0, 4);
			*port = 0;
			goto end;
		}
	}

	getRemoteIp(sock, ip);
	getRemotePort(sock, port);

end:
	return;
}

void AtDrv::getLocalIp(uint8_t *ip, long timeout)
{
	clearSerialRxData();
	INFO1(at_get_ip);
	serialPort[0].print(at_get_ip);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	
	// skip cmd response
	if(!serialPort[0].find(at_get_ip)) {
		INFO1("fail to get local IP");
		ip[0] = ip[1] = ip[2] = ip[3] = 0;
		goto end;
	}

	ip[0] = serialPort[0].parseInt();
	ip[1] = serialPort[0].parseInt();
	ip[2] = serialPort[0].parseInt();
	ip[3] = serialPort[0].parseInt();
	
	INFO("Local IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	
end:
	clearSerialRxData();
	return;	
}

void AtDrv::getNetmask(uint8_t *mask, long timeout)
{
	clearSerialRxData();
	INFO1(at_get_mask);
	serialPort[0].print(at_get_mask);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	
	// skip cmd response
	if(!serialPort[0].find(at_get_mask)) {
		INFO1("fail to get netmask");
		mask[0] = mask[1] = mask[2] = mask[3] = 0;
		goto end;
	}

	mask[0] = serialPort[0].parseInt();
	mask[1] = serialPort[0].parseInt();
	mask[2] = serialPort[0].parseInt();
	mask[3] = serialPort[0].parseInt();
	
	INFO("Netmask: %d.%d.%d.%d", mask[0], mask[1], mask[2], mask[3]);
	
end:
	clearSerialRxData();
	return;	
}

void AtDrv::getGateway(uint8_t *gwip, long timeout)
{
	int bytes;
	int start;
	char lineBuf[MAX_LINE_BUF_SIZE];
	char *token;

	clearSerialRxData();
	INFO1(at_get_gateway);
	serialPort[0].print(at_get_gateway);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	
	// skip cmd response && skip until "00000000"
	if(!serialPort[0].find(at_get_gateway) || !serialPort[0].find("00000000")) {
		INFO1("fail to get gateway");
		gwip[0] = gwip[1] = gwip[2] = gwip[3] = 0;
		goto end;
	}
	
	bytes = serialPort[0].readBytesUntil('\n', lineBuf, MAX_LINE_BUF_SIZE);
	if(bytes >= MAX_LINE_BUF_SIZE) {
		INFO1("Buffer is not enough");
		goto end;
	}	
	
	token = strtok(lineBuf, " \t");
	
	if(token)
		*((uint32_t *)gwip) = strtol(token, NULL, 16);
	else {
		INFO1("fail to parse netmask");
		gwip[0] = gwip[1] = gwip[2] = gwip[3] = 0;
		goto end;
	}
	
	INFO("Gateway: %d.%d.%d.%d", gwip[0], gwip[1], gwip[2], gwip[3]);
	
end:
	clearSerialRxData();
	return;	
}

void AtDrv::getNetworkData(uint8_t *ip, uint8_t *mask, uint8_t *gwip)
{
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			memset(ip, 0, 4);
			memcpy(mask, 0, 4);
			memcpy(gwip, 0, 4);
			goto end;
		}
	}

	getLocalIp(ip);
	getNetmask(mask);
	getGateway(gwip);

end:
	return;
}

bool AtDrv::isWiFiConnected(long timeout)
{
	bool ret = false;
	byte respBuf[MAX_RESP_BUF_SIZE];
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}	
	
	clearSerialRxData();
	INFO1(at_wifi_ConState);
	serialPort[0].print(at_wifi_ConState);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(at_Connected)) {
		INFO1("Not Connected");
		goto end;
	}
	ret = true;
end:
	return ret;
}

bool AtDrv::setWiFiConfig(char *ssid, int type, const char * password, long timeout)
{
	bool ret = false;
	char wifiConfBuf[MAX_WIFI_CONF_BUF_SIZE];
	char conSetting[MAX_TEMP_BUF_SIZE];

	if(type == ENC_TYPE_NONE)
		sprintf(conSetting, "%s,none,", ssid);
	else if(type == ENC_TYPE_WEP_OPEN)
		sprintf(conSetting, "%s,wep_open,%s", ssid, password);
	else
		sprintf(conSetting, "%s,auto,%s", ssid, password);

	sprintf(wifiConfBuf, at_wifi_conf, conSetting);
	
	clearSerialRxData();
	INFO1(wifiConfBuf);
	serialPort[0].print(wifiConfBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(wifiConfBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set wifi conf");
		goto end;
	}
	
	ret = true;		
end:	
	return ret;
}

bool AtDrv::setNetMode(int netMode, long timeout)
{
	bool ret = false;
	char netModeBuf[MAX_TEMP_BUF_SIZE];

	sprintf(netModeBuf, at_netmode, '0'+netMode);
	
	clearSerialRxData();
	INFO1(netModeBuf);
	serialPort[0].print(netModeBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(netModeBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set net mode");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

void AtDrv::netCommit()
{
	clearSerialRxData();
	INFO1(at_net_commit);
	serialPort[0].print(at_net_commit);
	serialPort[0].flush();
	delay(30000);
	// No any response
}

bool AtDrv::reConnect(long timeout)
{
	bool ret = false;

	clearSerialRxData();
	INFO1(at_reconn);
	serialPort[0].print(at_reconn);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(at_reconn) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to reconnect");
		goto end;
	}
	setAtMode(false);
#ifndef CHECK_TCP_STATE	
	// Since we don't know port status, try to wait 5 sec. anyway 
	delay(5000);
#endif
	clearSerialRxData();
	ret = true;		
end:
	return ret;
}

bool AtDrv::getTcpAuto(uint8_t sock, bool *enable, long timeout)
{
	bool ret = false;
	char tcpAutoBuf[MAX_TEMP_BUF_SIZE];

    sprintf(tcpAutoBuf, at_tcp_auto[sock], '?');
  
	clearSerialRxData();
	INFO1(tcpAutoBuf);
	serialPort[0].print(tcpAutoBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(tcpAutoBuf)) {
		INFO1("Fail to get tcp auto");
		goto end;
	}

	*enable = serialPort[0].parseInt();

	INFO("tcp auto: %d", *enable);

	clearSerialRxData();

	ret = true;		
end:
	return ret;
}

bool AtDrv::setTcpAuto(uint8_t sock, bool enable, long timeout)
{
	bool ret = false;
	char tcpAutoBuf[MAX_TEMP_BUF_SIZE];

  if(enable)
    sprintf(tcpAutoBuf, at_tcp_auto[sock], '1');
  else
    sprintf(tcpAutoBuf, at_tcp_auto[sock], '0');
  
	clearSerialRxData();
	INFO1(tcpAutoBuf);
	serialPort[0].print(tcpAutoBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(tcpAutoBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set tcp auto");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setDhcpd(bool enable, long timeout)
{
	bool ret = false;
	char dhcpdBuf[MAX_TEMP_BUF_SIZE];

  if(enable)
    sprintf(dhcpdBuf, at_dhcpd, '1');
  else
    sprintf(dhcpdBuf, at_dhcpd, '0');
  
	clearSerialRxData();
	INFO1(dhcpdBuf);
	serialPort[0].print(dhcpdBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(dhcpdBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set dhcpd");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setDhcpdIp(uint32_t ipStart, uint32_t ipEnd, uint32_t mask, uint32_t gateway, long timeout)
{
	bool ret = false;
	char dhcpIpBuf[MAX_DHCPD_IP_BUF_SIZE];
	char tempBuf[MAX_DHCPD_IP_BUF_SIZE];
	char ipStartBuf[MAX_IP_BUF_SIZE];
	char ipEndBuf[MAX_IP_BUF_SIZE];
	char maskBuf[MAX_IP_BUF_SIZE];
	char gatewayBuf[MAX_IP_BUF_SIZE];

	uint8_t *pIp = (uint8_t*) &ipStart;
	sprintf(ipStartBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

	pIp = (uint8_t*) &ipEnd;
	sprintf(ipEndBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

	pIp = (uint8_t*) &mask;
	sprintf(maskBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

	pIp = (uint8_t*) &gateway;
	sprintf(gatewayBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

	sprintf(tempBuf, "%s,%s,%s,%s", ipStartBuf, ipEndBuf, maskBuf, gatewayBuf);
  
	sprintf(dhcpIpBuf, at_dhcpd_ip, tempBuf);
    
	clearSerialRxData();
	INFO1(dhcpIpBuf);
	serialPort[0].print(dhcpIpBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(dhcpIpBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set dhcpd ip");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setDhcpdDns(uint32_t dns1, uint32_t dns2, long timeout)
{
	bool ret = false;
	char dhcpDnsBuf[MAX_DHCPD_IP_BUF_SIZE];
	char tempBuf[MAX_DHCPD_IP_BUF_SIZE];
	char dns1Buf[MAX_IP_BUF_SIZE];
	char dns2Buf[MAX_IP_BUF_SIZE];

 	uint8_t *pIp = (uint8_t*) &dns1;
	sprintf(dns1Buf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);
  
 	pIp = (uint8_t*) &dns2;
	sprintf(dns2Buf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);
  
	sprintf(tempBuf, "%s,%s", dns1Buf, dns2Buf);
  
	sprintf(dhcpDnsBuf, at_dhcpd_dns, tempBuf);
    
	clearSerialRxData();
	INFO1(dhcpDnsBuf);
	serialPort[0].print(dhcpDnsBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(dhcpDnsBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set dhcpd dns");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setDhcpdTime(uint32_t time, long timeout)
{
	bool ret = false;
	char dhcpTimeBuf[MAX_TEMP_BUF_SIZE];
	char timeBuf[MAX_TEMP_BUF_SIZE];

	sprintf(timeBuf, "%u", time);
	sprintf(dhcpTimeBuf, at_dhcpd_time, timeBuf);

	clearSerialRxData();
	INFO1(dhcpTimeBuf);
	serialPort[0].print(dhcpTimeBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(dhcpTimeBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set dhcpd time");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::getNetworkTimeout(uint8_t sock, uint32_t *time, long timeout)
{
	bool ret = false;
	char timeoutBuf[MAX_TEMP_BUF_SIZE];

	sprintf(timeoutBuf, at_timeout[sock], "?");

	clearSerialRxData();
	INFO1(timeoutBuf);
	serialPort[0].print(timeoutBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(timeoutBuf)) {
		INFO1("Fail to set timeout");
		goto end;
	}
	
	*time = serialPort[0].parseInt();
	ret = true;
	clearSerialRxData();
end:
	return ret;
}

bool AtDrv::setNetworkTimeout(uint8_t sock, uint32_t time, long timeout)
{
	bool ret = false;
	char timeoutBuf[MAX_TEMP_BUF_SIZE];
	char timeBuf[MAX_TEMP_BUF_SIZE];

	sprintf(timeBuf, "%u", time);
	sprintf(timeoutBuf, at_timeout[sock], timeBuf);

	clearSerialRxData();
	INFO1(timeoutBuf);
	serialPort[0].print(timeoutBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(timeoutBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set timeout");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setNetIp(uint32_t ip, uint32_t mask, uint32_t gateway, long timeout)
{
	bool ret = false;
	char netIpBuf[MAX_DHCPD_IP_BUF_SIZE];
	char tempBuf[MAX_DHCPD_IP_BUF_SIZE];
	char ipBuf[MAX_IP_BUF_SIZE];
	char maskBuf[MAX_IP_BUF_SIZE];
	char gatewayBuf[MAX_IP_BUF_SIZE];

 	uint8_t *pIp = (uint8_t*) &ip;
	sprintf(ipBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

 	pIp = (uint8_t*) &mask;
	sprintf(maskBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

 	pIp = (uint8_t*) &gateway;
	sprintf(gatewayBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);

	sprintf(tempBuf, "%s,%s,%s", ipBuf, maskBuf, gatewayBuf);

	sprintf(netIpBuf, at_net_ip, tempBuf);

	clearSerialRxData();
	INFO1(netIpBuf);
	serialPort[0].print(netIpBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(netIpBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set net ip");
		goto end;
	}

	ret = true;		
end:
	return ret;
}

bool AtDrv::setNetDns(uint32_t dns1, uint32_t dns2, long timeout)
{
	bool ret = false;
	char netDnsBuf[MAX_DHCPD_IP_BUF_SIZE];
	char tempBuf[MAX_DHCPD_IP_BUF_SIZE];
	char dns1Buf[MAX_IP_BUF_SIZE];
	char dns2Buf[MAX_IP_BUF_SIZE];

 	uint8_t *pIp = (uint8_t*) &dns1;
	sprintf(dns1Buf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);
  
 	pIp = (uint8_t*) &dns2;
	sprintf(dns2Buf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);
  
	sprintf(tempBuf, "%s,%s", dns1Buf, dns2Buf);

	sprintf(netDnsBuf, at_net_dns, tempBuf);

	clearSerialRxData();
	INFO1(netDnsBuf);
	serialPort[0].print(netDnsBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(netDnsBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set dhcpd dns");
		goto end;
	}

	ret = true;		
end:
	return ret;
}

bool AtDrv::setApSSID(char *ssid, int type, const char *password)
{
	bool ret = false;
	IPAddress ipStart(192, 168, 43, 100);
	IPAddress ipEnd(192, 168, 43, 200);
	IPAddress mask(255, 255, 255, 0);
	IPAddress gateway(192, 168, 43, 1);
	IPAddress dns1(192, 168, 43, 1);
	IPAddress dns2(8, 8, 8, 8);
	IPAddress netIp(192, 168, 43, 1);

	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}
	
	if(!setNetMode(NET_MODE_WIFI_AP))
		goto end;
	
	if(!setWiFiConfig(ssid, type, password))
		goto end;
    
 	if(!setDhcpd(true))
		goto end;
    
 	if(!setDhcpdIp(ipStart, ipEnd, mask, gateway))
		goto end;
    
 	if(!setDhcpdDns(dns1, dns2))
		goto end;  

 	if(!setDhcpdTime(86400))
		goto end;
    
 	if(!setNetIp(netIp, mask, gateway))
		goto end;
    
 	if(!setNetDns(dns1, dns2))
		goto end;      

	netCommit();

	ret = true;
end:
	return ret;
}

bool AtDrv::setSSID(char *ssid, int type, const char *password)
{
	bool ret = false;
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}
	
	if(!setNetMode(NET_MODE_WIFI_CLIENT))
		goto end;
	
	if(!setWiFiConfig(ssid, type, password))
		goto end;
		
	netCommit();

	ret = true;
end:
	return ret;
}

bool AtDrv::getMAC(uint8_t *mac, long timeout)
{
	bool ret = false;
	char buf[MAX_TEMP_BUF_SIZE];
	int bytes;
	int i = 0;
	char *token;
  
	memset(mac, 0, WL_MAC_ADDR_LENGTH);
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}
	
	clearSerialRxData();
	INFO1(at_Get_MAC);
	serialPort[0].print(at_Get_MAC);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(at_Get_MAC)) {
		INFO1("Fail to get MAC");
		goto end;
	}
  
	bytes = serialPort[0].readBytes(buf, MAX_TEMP_BUF_SIZE);
	
	if(bytes >= MAX_TEMP_BUF_SIZE) {
		INFO1("Buffer is not enough");
		goto end;
	}
	
	buf[bytes] = NULL;	
	token = strtok(buf, " :,\r\n");

	while(token != NULL && i < WL_MAC_ADDR_LENGTH) {
		mac[WL_MAC_ADDR_LENGTH-1-i] = strtol(token, NULL, 16);
		token = strtok(NULL, " :,\r\n");
		i++;
	}
  
	if(i!=6) {
		INFO1("Can't get valid MAC string");
		INFO1(buf);
		memset(mac, 0, WL_MAC_ADDR_LENGTH);
		goto end;	  
	}

	ret = true;
end:
	return ret;
}

bool AtDrv::getMode(uint8_t sock, int *mode, long timeout)
{
	bool ret = false;
	int bytes;
	char modeBuf[MAX_TEMP_BUF_SIZE];

	sprintf(modeBuf, at_mode[sock], "?");
	
	clearSerialRxData();
	INFO1(modeBuf);
	serialPort[0].print(modeBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(modeBuf)) {
		INFO1("Fail to get mode");
		goto end;
	}

	bytes = serialPort[0].readBytes(modeBuf, MAX_TEMP_BUF_SIZE);

	if(bytes >= MAX_TEMP_BUF_SIZE) {
		INFO1("Buffer is not enough");
		goto end;
	}
	
	modeBuf[bytes] = NULL;
	
	if(sock == 0) {
		if(strstr(modeBuf, "server")) {
			*mode = MODE_SERVER;
			ret = true;		

		}
		else if(strstr(modeBuf, "client")) {
			*mode = MODE_CLIENT;
			ret = true;		
		}
	}
	else {
		if(strstr(modeBuf, "1")) {
			*mode = MODE_SERVER;
			ret = true;		

		}
		else if(strstr(modeBuf, "2")) {
			*mode = MODE_CLIENT;
			ret = true;		
		}
	}
		
end:
	return ret;
}

bool AtDrv::setMode(uint8_t sock, int mode, long timeout)
{
	bool ret = false;
	char modeBuf[MAX_TEMP_BUF_SIZE];

	if(mode <= MODE_NONE || mode > MODE_SERVER) {
		INFO1("Invalid mode");
		goto end;
	}
	
	if(mode == MODE_SERVER) {
		if(sock == 0)
			sprintf(modeBuf, at_mode[sock], "server");
		else
			sprintf(modeBuf, at_mode[sock], "1");
	}
	else {
		if(sock == 0)
			sprintf(modeBuf, at_mode[sock], "client");
		else
			sprintf(modeBuf, at_mode[sock], "2");
	}
	
	clearSerialRxData();
	INFO1(modeBuf);
	serialPort[0].print(modeBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(modeBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set mode");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::getProtocol(uint8_t sock, uint8_t *protocol, long timeout)
{
	bool ret = false;
	int bytes;
	char protModeBuf[MAX_TEMP_BUF_SIZE];

	sprintf(protModeBuf, at_remotepro[sock], "?");
	
	clearSerialRxData();
	INFO1(protModeBuf);
	serialPort[0].print(protModeBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(protModeBuf)) {
		INFO1("Fail to get protocol");
		goto end;
	}

	bytes = serialPort[0].readBytes(protModeBuf, MAX_TEMP_BUF_SIZE);

	if(bytes >= MAX_TEMP_BUF_SIZE) {
		INFO1("Buffer is not enough");
		goto end;
	}
	
	protModeBuf[bytes] = NULL;
	
	if(sock == 0) {
		if(strstr(protModeBuf, "tcp")) {
			*protocol = PROTO_MODE_TCP;
			ret = true;		

		}
		else if(strstr(protModeBuf, "udp")) {
			*protocol = PROTO_MODE_UDP;
			ret = true;		
		}
	}
	else {
		if(strstr(protModeBuf, "1")) {
			*protocol = PROTO_MODE_TCP;
			ret = true;		

		}
		else if(strstr(protModeBuf, "2")) {
			*protocol = PROTO_MODE_UDP;
			ret = true;		
		}
	}

end:
	return ret;
}

bool AtDrv::setProtocol(uint8_t sock, uint8_t protocol, long timeout)
{
	bool ret = false;
	char protModeBuf[MAX_TEMP_BUF_SIZE];

	if(protocol != PROTO_MODE_TCP && protocol != PROTO_MODE_UDP) {
		INFO1("Invalid protocol");
		goto end;
	}
	
	if(protocol == PROTO_MODE_TCP) {
		if(sock == 0)
			sprintf(protModeBuf, at_remotepro[sock], "tcp");
		else
			sprintf(protModeBuf, at_remotepro[sock], "1");
	}
	else {
		if(sock == 0)
			sprintf(protModeBuf, at_remotepro[sock], "udp");
		else
			sprintf(protModeBuf, at_remotepro[sock], "2");
	}
	clearSerialRxData();
	INFO1(protModeBuf);
	serialPort[0].print(protModeBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(protModeBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set protocol");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setLocalPort(uint8_t sock, uint16_t port, long timeout)
{
	bool ret = false;
	char localPortBuf[MAX_TEMP_BUF_SIZE];
	char portBuf[MAX_TEMP_BUF_SIZE];

	sprintf(portBuf, "%u", port);
	sprintf(localPortBuf, at_CLport[sock], portBuf);
	
	clearSerialRxData();
	INFO1(localPortBuf);
	serialPort[0].print(localPortBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(localPortBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set local port");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setPort(uint8_t sock, uint16_t port, long timeout)
{
	bool ret = false;
	char remotePortBuf[MAX_TEMP_BUF_SIZE];
	char portBuf[MAX_TEMP_BUF_SIZE];

	sprintf(portBuf, "%u", port);
	sprintf(remotePortBuf, at_remoteport[sock], portBuf);
	
	clearSerialRxData();
	INFO1(remotePortBuf);
	serialPort[0].print(remotePortBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(remotePortBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set port");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::setRemoteIp(uint8_t sock, uint32_t ip, long timeout)
{
	bool ret = false;
	char remoteIpBuf[MAX_TEMP_BUF_SIZE];
	char ipBuf[MAX_TEMP_BUF_SIZE];
	uint8_t *pIp = (uint8_t*) &ip;
	sprintf(ipBuf, "%u.%u.%u.%u", pIp[0], pIp[1], pIp[2], pIp[3]);
	sprintf(remoteIpBuf, at_remoteip[sock], ipBuf);
	
	clearSerialRxData();
	INFO1(remoteIpBuf);
	serialPort[0].print(remoteIpBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(remoteIpBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set port");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

bool AtDrv::getRemoteHost(uint8_t sock, char *host, long timeout)
{
	bool ret = false;
	int bytes;
	char remoteHostBuf[MAX_HOST_NAME_BUF_SIZE];
	char *tok;

	sprintf(remoteHostBuf, at_remoteip[sock], "?");
	
	clearSerialRxData();
	INFO1(remoteHostBuf);
	serialPort[0].print(remoteHostBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(remoteHostBuf)) {
		INFO1("Fail to set port");
		goto end;
	}

	bytes = serialPort[0].readBytes(remoteHostBuf, MAX_HOST_NAME_BUF_SIZE);

	if(bytes >= MAX_HOST_NAME_BUF_SIZE) {
		INFO1("Buffer is not enough");
		goto end;
	}
	
	remoteHostBuf[bytes] = NULL;
	
	tok = strtok(remoteHostBuf, " \r\n");
	if(tok) {
		ret = true;
		strcpy(host, tok);
	}
end:
	return ret;
}

bool AtDrv::setRemoteHost(uint8_t sock, const char *host, long timeout)
{
	bool ret = false;
	char remoteHostBuf[MAX_HOST_NAME_BUF_SIZE];

	sprintf(remoteHostBuf, at_remoteip[sock], host);
	
	clearSerialRxData();
	INFO1(remoteHostBuf);
	serialPort[0].print(remoteHostBuf);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	if(!serialPort[0].find(remoteHostBuf) || !serialPort[0].find(at_ok)) {
		INFO1("Fail to set port");
		goto end;
	}
		
	ret = true;		
end:
	return ret;
}

void AtDrv::startServer(uint8_t sock, uint16_t port, uint8_t protMode)
{
	bool needReConn = false;
	int curMode;
	uint8_t curProtocol;
	uint16_t curPort;
	uint32_t curTimeout;

	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}
	
	if(!getMode(sock, &curMode) || curMode != MODE_SERVER) {
		needReConn = true;
		INFO1("curMode != MODE_SERVER");
		if(!setMode(sock, MODE_SERVER)) {
			INFO1("Can't set mode");
			goto end;			
		}
	}
	
	if(!getProtocol(sock, &curProtocol) || curProtocol != protMode) {
		needReConn = true;
		INFO1("curProtocol != protMode");
		if(!setProtocol(sock, protMode)) {
			INFO1("Can't set protocol");
			goto end;	
		}
	}

	if(!getRemotePort(sock, &curPort) || curPort != port) {
		needReConn = true;
		INFO1("curPort != port");
		if(!setPort(sock, port)) {
			INFO1("Can't set port");
			goto end;	
		}
	}

	if(!getNetworkTimeout(sock, &curTimeout) || curTimeout != 0) {
		needReConn = true;
		INFO1("curTimeout != 0");	
		if(!setNetworkTimeout(sock, 0)) {
			INFO1("Can't set timeout");
			goto end;	
		}
	}

	if(needReConn) {
		if(!reConnect()) {
			INFO1("Can't reconnect");
			goto end;	
		}
	}

	sockPort[sock] = port;
	sockConnected[sock] = true;

end:
	return;
}

void AtDrv::startClient(uint8_t sock, uint32_t ipAddress, uint16_t port, uint8_t protMode)
{
	// if we enable CHECK_TCP_STATE feature, always call reConnect(), or we won't get right
	// tcp port status, since we disable tcp auto reconnect.
#ifndef CHECK_TCP_STATE
	bool needReConn = false;
#else
	bool needReConn = true;
#endif
	int curMode;
	uint32_t curIp;
	uint8_t curProtocol;
	uint16_t curPort;
	uint16_t curLocalPort;
	uint32_t curTimeout;
	bool curTcpAuto;

	// clear uart buffer first
	stopClient(sock);
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}
	
	if(!getMode(sock, &curMode) || curMode != MODE_CLIENT) {
		needReConn = true;
		INFO1("curMode != MODE_CLIENT");
		if(!setMode(sock, MODE_CLIENT)) {
			INFO1("Can't set mode");
			goto end;			
		}
	}
	
	if(!getRemoteIp(sock, (uint8_t *)&curIp) || curIp != ipAddress) {
		needReConn = true;
		INFO1("curIp != ipAddress");
		if(!setRemoteIp(sock, ipAddress)) {
			INFO1("Can't set ip");
			goto end;	
		}
	}
	
	if(!getProtocol(sock, &curProtocol) || curProtocol != protMode) {
		needReConn = true;
		INFO1("curProtocol != protMode");
		if(!setProtocol(sock, protMode)) {
			INFO1("Can't set protocol");
			goto end;	
		}
	}
	
	if(!getRemotePort(sock, &curPort) || curPort != port) {
		needReConn = true;
		INFO1("curPort != port");
		if(!setPort(sock, port)) {
			INFO1("Can't set port");
			goto end;	
		}
	}

	if(!getTcpAuto(sock, &curTcpAuto) || curTcpAuto != false) {
		needReConn = true;
		INFO1("curTcpAuto != false");	
		if(!setTcpAuto(sock, false)) {
			INFO1("Can't set tcp auto");
			goto end;	
		}
	}
	
	if(!getLocalPort(sock, &curLocalPort) || curLocalPort != localSockPort[sock]) {
		needReConn = true;
		INFO1("curLocalPort != port");
		if(!setLocalPort(sock, localSockPort[sock])) {
			INFO1("Can't set port");
			goto end;	
		}
	}
	
	if(needReConn) {
		if(!reConnect()) {
			INFO1("Can't reconnect");
			goto end;	
		}
	}

	sockPort[sock] = localSockPort[sock];
	sockConnected[sock] = true;

end:
	return;
}

void AtDrv::startClient(uint8_t sock, const char *host, uint16_t port, uint8_t protMode)
{
	// if we enable CHECK_TCP_STATE feature, always call reConnect(), or we won't get right
	// tcp port status, since we disable tcp auto reconnect.
#ifndef CHECK_TCP_STATE
	bool needReConn = false;
#else
	bool needReConn = true;
#endif
	int curMode;
	char curHostBuf[MAX_HOST_NAME_BUF_SIZE];
	uint8_t curProtocol;
	uint16_t curPort;
	uint16_t curLocalPort;
	uint32_t curTimeout;
	bool curTcpAuto;

	// clear uart buffer first
	stopClient(sock);

	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}
	
	if(!getMode(sock, &curMode) || curMode != MODE_CLIENT) {
		needReConn = true;
		INFO1("curMode != MODE_CLIENT");
		if(!setMode(sock, MODE_CLIENT)) {
			INFO1("Can't set mode");
			goto end;			
		}
	}

	if(!getRemoteHost(sock, curHostBuf) || (strcmp(curHostBuf,  host) != 0)) {
		needReConn = true;
		INFO1("curHostBuf != host");
		if(!setRemoteHost(sock, host)) {
			INFO1("Can't set host");
			goto end;	
		}
	}
	
	if(!getProtocol(sock, &curProtocol) || curProtocol != protMode) {
		needReConn = true;
		INFO1("curProtocol != protMode");
		if(!setProtocol(sock, protMode)) {
			INFO1("Can't set protocol");
			goto end;	
		}
	}
	
	if(!getRemotePort(sock, &curPort) || curPort != port) {
		needReConn = true;
		INFO1("curPort != port");
		if(!setPort(sock, port)) {
			INFO1("Can't set port");
			goto end;	
		}
	}
	
	if(!getTcpAuto(sock, &curTcpAuto) || curTcpAuto != false) {
		needReConn = true;
		INFO1("curTcpAuto != false");	
		if(!setTcpAuto(sock, false)) {
			INFO1("Can't set tcp auto");
			goto end;	
		}
	}	
	
	if(!getLocalPort(sock, &curLocalPort) || curLocalPort != localSockPort[sock]) {
		needReConn = true;
		INFO1("curLocalPort != port");
		if(!setLocalPort(sock, localSockPort[sock])) {
			INFO1("Can't set port");
			goto end;	
		}
	}	
	
	if(needReConn) {
		if(!reConnect()) {
			INFO1("Can't reconnect");
			goto end;	
		}
	}	

	sockPort[sock] = localSockPort[sock];
	sockConnected[sock] = true;

end:
	return;
}

uint16_t AtDrv::availData(uint8_t sock)
{
	uint16_t len = 0;
	
	// sock1 (second uart port) is always in data mode, so don't need to switch mode
	if(sock == 0 && isAtMode()) {
		if(!switchToDataMode()) {
			INFO1("Can't switch to data mode");
			goto end;
		}
	}
	
	// sock0's available data len should plus the data in sock0DataQueue
	if(sock == 0)
		len = sock0DataQueue.count();
	
	len += serialPort[sock].available();
end:
	return len;
}

int AtDrv::peek(uint8_t sock)
{
	int c = -1;
	
	// sock1 (second uart port) is always in data mode, so don't need to switch mode
	if(sock == 0 && isAtMode()) {
		if(!switchToDataMode()) {
			INFO1("Can't switch to data mode");
			goto end;
		}
	}

	// if sock0 && sock0DataQueue has data, then peek data in sock0DataQueue
	if(sock == 0 && !sock0DataQueue.isEmpty())
		c = sock0DataQueue.peek();
	else
		c = serialPort[sock].peek();
end:
	return c;
}

int AtDrv::read(uint8_t sock)
{
	int c = -1;
	
	// sock1 (second uart port) is always in data mode, so don't need to switch mode
	if(sock == 0 && isAtMode()) {
		if(!switchToDataMode()) {
			INFO1("Can't switch to data mode");
			goto end;
		}
	}

	// if sock0 && sock0DataQueue has data, then read data in sock0DataQueue
	if(sock == 0 && !sock0DataQueue.isEmpty())
		c = sock0DataQueue.pop();
	else
		c = serialPort[sock].read();
end:
	return c;
}

uint16_t AtDrv::readBytes(uint8_t sock, uint8_t *_data, uint16_t *_dataLen)
{
	uint16_t len = 0;
	
	// sock1 (second uart port) is always in data mode, so don't need to switch mode
	if(sock == 0 && isAtMode()) {
		if(!switchToDataMode()) {
			INFO1("Can't switch to data mode");
			goto end;
		}
	}

	// if sock0 && sock0DataQueue has data, then read data in sock0DataQueue first
	if(sock == 0) {
		while(!sock0DataQueue.isEmpty()) {
			_data[len++] = sock0DataQueue.pop();
			// full? then exit
			if(len == *_dataLen)
				goto end;
		}
		// substrate data length we've filled
		*_dataLen -= len;
	}

	len += serialPort[sock].readBytes((char *)_data+len, *_dataLen);
	*_dataLen = len;
end:
	return len;
}

uint16_t AtDrv::write(uint8_t sock, const uint8_t *data, uint16_t _len)
{
	uint16_t len = 0;
	
	// sock1 (second uart port) is always in data mode, so don't need to switch mode
	if(sock == 0 && isAtMode()) {
		if(!switchToDataMode()) {
			INFO1("Can't switch to data mode");
			goto end;
		}
	}
	
	len = serialPort[sock].write(data, _len);
end:
	return len;
}

uint8_t AtDrv::portStateMapping(LinuxTcpState linuxPortState)
{
	switch (linuxPortState) {
	case TCP_ESTABLISHED:
		return ESTABLISHED;
	
	case TCP_SYN_SENT:
		return SYN_SENT;
	
	case TCP_SYN_RECV:
		return SYN_RCVD;
	
	case TCP_FIN_WAIT1:
		return FIN_WAIT_1;
	
	case TCP_FIN_WAIT2:
		return FIN_WAIT_2;
	
	case TCP_TIME_WAIT:
		return TIME_WAIT;
	
	case TCP_CLOSE:
		return CLOSED;
	
	case TCP_CLOSE_WAIT:
		return CLOSE_WAIT;
	
	case TCP_LAST_ACK:
		return LAST_ACK;
	
	case TCP_LISTEN:
		return LISTEN;
	
	case TCP_CLOSING:
		return CLOSING;
	
	default:
		return CLOSED;
	}
}

uint8_t AtDrv::getClientState(uint8_t sock, long timeout)
{
#ifndef CHECK_TCP_STATE
	// Since RM04 can't get current connection status, we always return ESTABLISHED
	sockConnected[sock] = true;
	return ESTABLISHED;
#else
	// Try to use at+excxxx to get & parse tcp port state
	bool found = false;
	uint8_t ret = CLOSED;
	int bytes;
	LinuxTcpState portState;
	int start;
	char lineBuf[MAX_LINE_BUF_SIZE];
	char cmdBuf[MAX_TEMP_BUF_SIZE];
	char *token;

	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}

	clearSerialRxData();
	sprintf(cmdBuf, at_tcp_port_stat, sockPort[sock]);
	INFO1(cmdBuf);
	serialPort[0].print(cmdBuf);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	
	// skip cmd response
	serialPort[0].find(cmdBuf);
	
	for(int i=0;;i++) {

		found = serialPort[0].find(":");

		if(found)
		{
			bytes = serialPort[0].readBytesUntil('\n', lineBuf, MAX_LINE_BUF_SIZE);
			if(bytes >= MAX_LINE_BUF_SIZE) {
				INFO1("Buffer is not enough");
				goto end;
			}
			lineBuf[bytes] = NULL;	
			//INFO1(lineBuf);
			
			token = strtok(lineBuf, " :\r\n");
			token = strtok(NULL, " :\r\n");
			if(token == NULL) {
				INFO1("failed to parse tcp info");
				goto end;		
			}
			//INFO1("TCP port: ");
			//INFO1(token);

			if(strtol(token, NULL, 16) == sockPort[sock]) {
				//INFO1("Found port");
				token = strtok(NULL, " :\r\n");
				token = strtok(NULL, " :\r\n");
				token = strtok(NULL, " :\r\n");
				if(token == NULL) {
					//INFO1("failed to parse tcp info");
					goto end;		
				}
				//INFO1("Port state: ");
				//INFO1(token);				
				portState = (LinuxTcpState) strtol(token, NULL, 16);
				ret = portStateMapping(portState);
				// if port state is ESTABLISHED, return immediately. otherwise, keep parsing
				if(ret == ESTABLISHED) {
					sockConnected[sock] = true;
					goto end;
				}
			}
		}
		else {
			INFO1("Not Found");
		}
		
		serialPort[0].setTimeout(SERIAL_TIMEOUT);

		if(!serialPort[0].available()) {
			INFO1("No data");
			break;
		}
	}


end:
	clearSerialRxData();
	return ret;
#endif
}

void AtDrv::stopClient(uint8_t sock)
{
	if(sock == 0 && isAtMode()) {
		if(!switchToDataMode()) {
			INFO1("Can't switch to data mode");
			goto end;
		}
	}
	
	if(sock == 0) {
		while(!sock0DataQueue.isEmpty())
			sock0DataQueue.pop();
	}

	// clear uart buffer, since we won't use it
	while(serialPort[sock].available())
		serialPort[sock].read();

end:
	sockConnected[sock] = false;
	return;
}

bool AtDrv::disconnect()
{
	bool ret = false;
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}
	
	// set netmode as eth mode will disable wifi
	if(!setNetMode(NET_MODE_ETH))
		goto end;
		
	netCommit();

	ret = true;
end:
	return ret;	
}

uint8_t AtDrv::getScanNetworks(char _networkSsid[][WL_SSID_MAX_LENGTH+1], int32_t _networkRssi[], uint8_t _networkEncr[], uint8_t maxItems, long timeout)
{
	uint8_t numItems = 0;
	int bytes;
	char lineBuf[MAX_LINE_BUF_SIZE];
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}

	clearSerialRxData();
	INFO1(at_wifi_Scan);
	serialPort[0].print(at_wifi_Scan);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	
	// skip cmd response
	serialPort[0].find(at_wifi_Scan);
	serialPort[0].readBytesUntil('\n', lineBuf, MAX_LINE_BUF_SIZE);
	
	// find item
	do {
		bytes = serialPort[0].readBytesUntil('\n', lineBuf, MAX_LINE_BUF_SIZE);
		if(bytes >= MAX_LINE_BUF_SIZE) {
			INFO1("Buffer is not enough");
			goto end;
		}
		else if(bytes == 0) {
			INFO1("No more items");
			goto end;		
		}
		lineBuf[bytes] = NULL;
		// a valid item line should be channel num (a number) and contains 100 characters above
		if(lineBuf[0] >= '0' && lineBuf[1] <= '9' && bytes > 100) {
			INFO1("Found a item");
			// parse ssid
			// ssid should starts at lineBuf[4]
			if(lineBuf[4] == ' ') {
				INFO1("A hidden ssid, skip");
				continue;
			}
			memcpy(_networkSsid[numItems], &lineBuf[4], WL_SSID_MAX_LENGTH);
			// truncate trail spaces of ssid
			int i = WL_SSID_MAX_LENGTH-1;
			_networkSsid[numItems][WL_SSID_MAX_LENGTH] = NULL;
			while(_networkSsid[numItems][i--] == ' ' && i > 0);
			
			_networkSsid[numItems][i+2] = NULL;
			INFO1(_networkSsid[numItems]);
			
			// parse enc type
			// security type should starts at lineBuf[57] and ends at lineBuf[78]
			char *enc = &lineBuf[57];
			lineBuf[79] = NULL;
			if(strstr(enc, "/TKIPAES") != NULL) {
				INFO1("AUTO");
				_networkEncr[numItems] = ENC_TYPE_AUTO;
			}
			else if(strstr(enc, "NONE") != NULL) {
				INFO1("NONE");
				_networkEncr[numItems] = ENC_TYPE_NONE;
			}
			else if(strstr(enc, "/AES") != NULL) {
				INFO1("AES");
				_networkEncr[numItems] = ENC_TYPE_CCMP;
			}
			else if(strstr(enc, "/TKIP") != NULL) {
				INFO1("TKIP");
				_networkEncr[numItems] = ENC_TYPE_TKIP;
			}
			else if(strstr(enc, "WEP") != NULL) {
				INFO1("WEP");
				_networkEncr[numItems] = ENC_TYPE_WEP;
			}
			else {
				INFO1("Unknow, treat as error");
				continue;
			}
			
			// parse signal strength
			// signal strength should starts at lineBuf[80] and ends at lineBuf[82]
			char *rssi = &lineBuf[80];
			lineBuf[83] = NULL;
			// this is link qualit value (0~100)
			_networkRssi[numItems] = strtoul(rssi, NULL, 10);
			// convert link quality to dbm
			_networkRssi[numItems] = (_networkRssi[numItems]/2) - 100;
			
			numItems++;
		}
	}while(numItems < maxItems);

end:
	clearSerialRxData();
	return numItems;
}

void AtDrv::getCurrentSSID(char _ssid[], long timeout)
{
	int bytes;
	char buf[MAX_TEMP_BUF_SIZE];
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}

	clearSerialRxData();
	INFO1(at_get_ssid);
	serialPort[0].print(at_get_ssid);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);

	// skip cmd response
	if(!serialPort[0].find(at_get_ssid)) {
		INFO1("fail to get ssid");
		_ssid[0] = NULL;
		goto end;
	}
	
	bytes = serialPort[0].readBytesUntil('\n', buf, MAX_TEMP_BUF_SIZE);
	if(bytes >= MAX_TEMP_BUF_SIZE || bytes > WL_SSID_MAX_LENGTH) {
		INFO1("Buffer is not enough");
		_ssid[0] = NULL;
		goto end;
	}

	buf[bytes] = NULL;
	
	strncpy(_ssid, buf, WL_SSID_MAX_LENGTH+1);
end:
	clearSerialRxData();
	return;
}

void AtDrv::getCurrentBSSID(uint8_t _bssid[], long timeout)
{
	int bytes;
	int i = 0;
	char *token;
	char buf[MAX_TEMP_BUF_SIZE];
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}

	clearSerialRxData();
	INFO1(at_get_bssid);
	serialPort[0].print(at_get_bssid);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);

	// skip cmd response
	if(!serialPort[0].find(at_get_bssid)) {
		INFO1("fail to get bssid");
		memset(_bssid, 0, WL_MAC_ADDR_LENGTH);
		goto end;
	}
	
	bytes = serialPort[0].readBytes(buf, MAX_TEMP_BUF_SIZE);
	
	if(bytes >= MAX_TEMP_BUF_SIZE) {
		INFO1("Buffer is not enough");
		memset(_bssid, 0, WL_MAC_ADDR_LENGTH);
		goto end;
	}
	
	buf[bytes] = NULL;	
	
	token = strtok(buf, " :");

	while(token != NULL && i < WL_MAC_ADDR_LENGTH) {
		_bssid[WL_MAC_ADDR_LENGTH-1-i] = strtol(token, NULL, 16);
		token = strtok(NULL, " :,\r\n");
		i++;
	}
  
	if(i!=6) {
		INFO1("Can't get valid MAC string");
		INFO1(buf);
		memset(_bssid, 0, WL_MAC_ADDR_LENGTH);
		goto end;	  
	}

end:
	clearSerialRxData();
	return;
}

int32_t AtDrv::getCurrentRSSI(long timeout)
{
	int32_t rssi = 0;
	clearSerialRxData();
	INFO1(at_get_rssi);
	serialPort[0].print(at_get_rssi);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);
	
	// skip cmd response
	if(!serialPort[0].find(at_get_rssi)) {
		INFO1("fail to get rssi");
		goto end;
	}

	// this is link qualit value (0~100)
	rssi = serialPort[0].parseInt();
	// convert link quality to dbm
	rssi = (rssi/2) - 100;

	INFO("RSSI: %ld dbm", rssi);
	
end:
	clearSerialRxData();
	return rssi;	
}

uint8_t AtDrv::getCurrentEncryptionType(long timeout)
{
	uint8_t enc = 0;
	int bytes;
	char buf[MAX_TEMP_BUF_SIZE];
	
	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}

	clearSerialRxData();
	INFO1(at_get_enc);
	serialPort[0].print(at_get_enc);
	serialPort[0].print("\r\r");
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);

	// skip cmd response
	if(!serialPort[0].find(at_get_enc)) {
		INFO1("fail to get enc type");
		goto end;
	}
	
	bytes = serialPort[0].readBytesUntil('\n', buf, MAX_TEMP_BUF_SIZE);
	if(bytes >= MAX_TEMP_BUF_SIZE) {
		INFO1("Buffer is not enough");
		goto end;
	}
	
	buf[bytes] = NULL;

	if(strstr(buf, "/TKIPAES") != NULL) {
		enc = ENC_TYPE_AUTO;
	}
	else if(strstr(buf, "NONE") != NULL) {
		enc = ENC_TYPE_NONE;
	}
	else if(strstr(buf, "/AES") != NULL) {
		enc = ENC_TYPE_CCMP;
	}
	else if(strstr(buf, "/TKIP") != NULL) {
		enc = ENC_TYPE_TKIP;
	}
	else if(strstr(buf, "WEP") != NULL) {
		enc = ENC_TYPE_WEP;
	}
	else {
		INFO1("Unknow, treat as error");
	}

end:
	clearSerialRxData();
	return enc;
}

void AtDrv::getFwVersion(char fwVersion[], uint8_t bufLength, long timeout)
{
	int bytes;

	if(!isAtMode()) {
		if(!switchToAtMode()) {
			INFO1("Can't switch to at mode");
			goto end;
		}
	}

	clearSerialRxData();
	INFO1(at_ver);
	serialPort[0].print(at_ver);
	serialPort[0].flush();
	serialPort[0].setTimeout(timeout);

	// skip cmd response and '\n'
	if(!serialPort[0].find(at_ver) || !serialPort[0].find("\n")) {
		INFO1("fail to get firmware version");
		goto end;
	}
	
	bytes = serialPort[0].readBytesUntil('\r', fwVersion, bufLength);
	fwVersion[bytes] = NULL;

end:
	clearSerialRxData();
	return;
}

uint8_t AtDrv::checkDataSent(uint8_t sock)
{
	// Since RM04 can't check data sent status, jsut flush our UART buffer instead
	serialPort[sock].flush();
	return 1;
}

AtDrv::AtDrv()
{
#ifdef USE_ESCAPE_PIN
	// need change pin mode and output it to inactive state in the very beginning, or
	// might put module in facotry rest mode
	pinMode(ESCAPE_PIN, OUTPUT);
	digitalWrite(ESCAPE_PIN, !ESCAPE_PIN_ACTIVE);
#endif
	sock0DataQueue.setPrinter(Serial);
}

AtDrv atDrv;

HardwareSerial AtDrv::serialPort[] = {AT_DRV_SERIAL, AT_DRV_SERIAL1};
bool AtDrv::atMode = false;
uint16_t AtDrv::sockPort[2] = {0};
QueueList<uint8_t> AtDrv::sock0DataQueue;
bool AtDrv::sockConnected[2] = {false, false};
