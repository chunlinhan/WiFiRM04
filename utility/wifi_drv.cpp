#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Arduino.h"
#include "at_drv.h"
#include "wifi_drv.h"

#define _DEBUG_

extern "C" {
#include "wifi_spi.h"
#include "wl_types.h"
#include "debug.h"
}

// Array of data to cache the information related to the networks discovered
char 	WiFiDrv::_networkSsid[][WL_SSID_MAX_LENGTH+1] = {{"1"},{"2"},{"3"},{"4"},{"5"}};
int32_t WiFiDrv::_networkRssi[WL_NETWORKS_LIST_MAXNUM] = { 0 };
uint8_t WiFiDrv::_networkEncr[WL_NETWORKS_LIST_MAXNUM] = { 0 };
uint8_t numList = 0;

// Cached values of retrieved data
char 	WiFiDrv::_ssid[] = {0};
uint8_t	WiFiDrv::_bssid[] = {0};
uint8_t WiFiDrv::_mac[] = {0};
uint8_t WiFiDrv::_localIp[] = {0};
uint8_t WiFiDrv::_subnetMask[] = {0};
uint8_t WiFiDrv::_gatewayIp[] = {0};
// Firmware version
char    WiFiDrv::fwVersion[] = {0};


// Private Methods

void WiFiDrv::getNetworkData(uint8_t *ip, uint8_t *mask, uint8_t *gwip)
{
	AtDrv::getNetworkData(ip, mask, gwip);
}

void WiFiDrv::getRemoteData(uint8_t sock, uint8_t *ip, uint8_t *port)
{
    AtDrv::getRemoteData(sock, ip, (uint16_t *)port);
}


// Public Methods


void WiFiDrv::wifiDriverInit()
{
    AtDrv::begin();
}

bool WiFiDrv::wifiSetApMode(char* ssid, uint8_t ssid_len)
{
    return AtDrv::setApSSID(ssid, 0, NULL);
}

int8_t WiFiDrv::wifiSetNetwork(char* ssid, uint8_t ssid_len)
{
    bool ret;

    ret = AtDrv::setSSID(ssid, ENC_TYPE_NONE, NULL);

    return (ret)? WL_SUCCESS : WL_FAILURE;
}

int8_t WiFiDrv::wifiSetPassphrase(char* ssid, uint8_t ssid_len, const char *passphrase, const uint8_t len)
{
    bool ret;

    ret = AtDrv::setSSID(ssid, ENC_TYPE_AUTO, passphrase);

    return (ret)? WL_SUCCESS : WL_FAILURE;
}


int8_t WiFiDrv::wifiSetKey(char* ssid, uint8_t ssid_len, uint8_t key_idx, const void *key, const uint8_t len)
{
    bool ret;

    ret = AtDrv::setSSID(ssid, ENC_TYPE_WEP_OPEN, (char *)key);

    return (ret)? WL_SUCCESS : WL_FAILURE;
}

void WiFiDrv::config(uint8_t validParams, uint32_t local_ip, uint32_t gateway, uint32_t subnet)
{
	// Not support
}

void WiFiDrv::setDNS(uint8_t validParams, uint32_t dns_server1, uint32_t dns_server2)
{
    // Not support
}


                        
int8_t WiFiDrv::disconnect()
{
    bool ret;

    ret = AtDrv::disconnect();

    return (ret)? WL_SUCCESS : WL_FAILURE;
}

uint8_t WiFiDrv::getConnectionStatus()
{
    bool ret;

    ret = AtDrv::isWiFiConnected();

    return (ret)? WL_CONNECTED : WL_IDLE_STATUS;
}

uint8_t* WiFiDrv::getMacAddress()
{
    AtDrv::getMAC(_mac);
    return _mac;
}

void WiFiDrv::getIpAddress(IPAddress& ip)
{
    getNetworkData(_localIp, _subnetMask, _gatewayIp);
    ip = _localIp;
}

 void WiFiDrv::getSubnetMask(IPAddress& mask)
 {
    getNetworkData(_localIp, _subnetMask, _gatewayIp);
    mask = _subnetMask;
 }

 void WiFiDrv::getGatewayIP(IPAddress& ip)
 {
    getNetworkData(_localIp, _subnetMask, _gatewayIp);
    ip = _gatewayIp;
 }

char* WiFiDrv::getCurrentSSID()
{
    AtDrv::getCurrentSSID(_ssid);
    return _ssid;
}

uint8_t* WiFiDrv::getCurrentBSSID()
{
    AtDrv::getCurrentBSSID(_bssid);
    return _bssid;
}

int32_t WiFiDrv::getCurrentRSSI()
{
    return AtDrv::getCurrentRSSI();
}

uint8_t WiFiDrv::getCurrentEncryptionType()
{
    return AtDrv::getCurrentEncryptionType();
}

int8_t WiFiDrv::startScanNetworks()
{
    // do nothing, all things will be done at getSSIDNetoworks()
    return WL_SUCCESS;
}


uint8_t WiFiDrv::getScanNetworks()
{
    numList = AtDrv::getScanNetworks(_networkSsid, _networkRssi, _networkEncr, WL_NETWORKS_LIST_MAXNUM);
    return numList;
}

char* WiFiDrv::getSSIDNetoworks(uint8_t networkItem)
{
    if (networkItem >= WL_NETWORKS_LIST_MAXNUM || networkItem >= numList)
        return NULL;
		
    return _networkSsid[networkItem];
}

uint8_t WiFiDrv::getEncTypeNetowrks(uint8_t networkItem)
{
    if (networkItem >= WL_NETWORKS_LIST_MAXNUM || networkItem >= numList)
        return NULL;

    return _networkEncr[networkItem];
}

int32_t WiFiDrv::getRSSINetoworks(uint8_t networkItem)
{
    if (networkItem >= WL_NETWORKS_LIST_MAXNUM || networkItem >= numList)
        return NULL;

    return _networkRssi[networkItem];
}

uint8_t WiFiDrv::reqHostByName(const char* aHostname)
{
    // Not support
    return 0;
}

int WiFiDrv::getHostByName(IPAddress& aResult)
{
    // Not support
    return 0;
}

int WiFiDrv::getHostByName(const char* aHostname, IPAddress& aResult)
{
    // Not support
    return 0;
}

char*  WiFiDrv::getFwVersion()
{
    AtDrv::getFwVersion(fwVersion, WL_FW_VER_LENGTH);
    return fwVersion;
}

WiFiDrv wiFiDrv;
