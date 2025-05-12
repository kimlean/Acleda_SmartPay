/* File: src/utils/network.c */

#include <stdio.h>
#include <string.h>

#include "def.h"
#include "httpDownload.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

NET_SOCKET_PRI *g_tms_socket = NULL;
NET_SOCKET_PRI *g_mqtt_socket = NULL;
NET_SOCKET_PRI *g_api_socket = NULL;
NET_SOCKET_PRI sockets[MAX_SOCKETS];

void net_init(void)
{
    int i;
    // Initialize all socket slots
    for (i = 0; i < MAX_SOCKETS; i++) {
        memset(&sockets[i], 0, sizeof(NET_SOCKET_PRI));
    }

    // Reset global socket references
    g_tms_socket = NULL;
    g_mqtt_socket = NULL;
    g_api_socket = NULL;

    MAINLOG_L1("Network socket management initialized with %d slots", MAX_SOCKETS);
}

int getCertificate(char *cerName, unsigned char *cer){
    // Existing implementation remains unchanged
    int Cerlen, Ret;
    u8 CerBuf[CERTI_LEN];

    Cerlen = GetFileSize_Api(cerName);
    if(Cerlen <= 0)
    {
        MAINLOG_L1("get certificate err or not exist - %s",cerName);
        return -1;
    }

    memset(CerBuf, 0 , sizeof(CerBuf));
    if(Cerlen > CERTI_LEN)
    {
        MAINLOG_L1("%s is too large", cerName);
        return -2;
    }

    Ret = ReadFile_Api(cerName, CerBuf, 0, (unsigned int *)&Cerlen);
    if(Ret != 0)
    {
        MAINLOG_L1("read %s failed", cerName);
        return -3;
    }
    memcpy(cer,CerBuf,strlen(CerBuf));

    return 0;
}

void *net_connect(void* attch, const char *host, const char *port, int timerOutMs, int ssl, int *errCode)
{
    int ret;
    int port_int;
    int sock;
    int i;
    u8 CerBuf[CERTI_LEN];
    int timeid = 0;

    // Step 1: Check if we already have a connection to this host:port
    for (i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].valid &&
            sockets[i].ssl == ssl &&
            strcmp(sockets[i].host, host) == 0 &&
            strcmp(sockets[i].port, port) == 0) {

            // Found an existing connection, increment reference counter
            sockets[i].ref_count++;
            *errCode = 0;

            MAINLOG_L1("Reusing existing socket connection to %s:%s (ref: %d)",
                      host, port, sockets[i].ref_count);

            return &sockets[i];
        }
    }

    // Find an empty socket slot for a new connection
    for (i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].valid == 0)
            break;
    }

    if (i >= MAX_SOCKETS) {
        *errCode = -3;
        MAINLOG_L1("ERROR: All socket slots are in use (%d max)", MAX_SOCKETS);
        return NULL;
    }

    if (!_IS_GPRS_ENABLED_) {
        MAINLOG_L1("Creating new WIFI socket connection to %s:%s (SSL: %d)", host, port, ssl);

        ret = wifiGetLinkStatus_lib();
        MAINLOG_L1("wifiGetLinkStatus_lib:%d", ret);
        if(ret <= 0) {
            *errCode = -3;
            return NULL;
        }

        if(ssl == 0) {
            // >=0 success  , others -failed
            sock = wifiSocketCreate_lib(0); //0-TCP   1-UDP
            MAINLOG_L1("wifiSocketCreate_lib:%d", sock);
            if(sock < 0) {
                *errCode = -4;
                return NULL;
            }

            ret = wifiTCPConnect_lib(sock, host, port, timerOutMs);
            MAINLOG_L1("wifiTCPConnect_lib:%d", ret);
            if(ret != 0)
            {
                *errCode = -5;
                ret =  wifiSocketClose_lib(sock);
                MAINLOG_L1("wifiSocketClose_lib:%d", ret);
                return NULL;
            }
        } else {
            if(ssl == 1) {
                MAINLOG_L1("SSL connection without certificate");
            }
            else if(ssl == 2) {
                memset(CerBuf, 0 , sizeof(CerBuf));
                ret = getCertificate(FILE_CERT_ROOT, CerBuf);
                MAINLOG_L1("getCertificate === %d", ret);
                ret = WifiespDownloadCrt_lib(0, CerBuf, strlen((char *)CerBuf));
                memset(CerBuf, 0 , sizeof(CerBuf));
                ret = getCertificate(FILE_CERT_PRIVATE, CerBuf);
                MAINLOG_L1("getCertificate === %d", ret);
                ret = WifiespDownloadCrt_lib(2, CerBuf, strlen((char *)CerBuf));
                memset(CerBuf, 0 , sizeof(CerBuf));
                ret = getCertificate(FILE_CERT_CHAIN, CerBuf);
                MAINLOG_L1("getCertificate === %d", ret);
                ret = WifiespDownloadCrt_lib(1, CerBuf, strlen((char *)CerBuf));
                MAINLOG_L1("WifiespDownloadCrt_lib = %d" , ret );

                Delay_Api(50);
            }

            sock = wifiSSLSocketCreate_lib(); //>=0 -success
            MAINLOG_L1("wifiSSLSocketCreate_lib:%d", sock);
            if(sock < 0) {
                *errCode = -4;
                return NULL;
            }

            if(ssl == 1) {
                //noted: the second parameter value need to be right for WifiespSetSSLConfig_lib . 0-not verify
                WifiespSetSSLConfig_lib(sock,0);
                MAINLOG_L1("WifiespSetSSLConfig_lib mode:%d", 0);
            }else if(ssl == 2) {
                WifiespSetSSLConfig_lib(sock,3);
                MAINLOG_L1("WifiespSetSSLConfig_lib mode:%d", 3);
            }

            ret = wifiSSLConnect_lib(sock, host, port, timerOutMs);
            MAINLOG_L1("wifiSSLConnect_lib:%d", ret);

            if(ret != 0) {
                *errCode = -5;
                ret = wifiSSLClose_lib(sock);
                MAINLOG_L1("wifiSSLClose_lib:%d", ret);
                if(ret == 0) {
                    ret = wifiSSLSocketClose_lib(sock);
                    MAINLOG_L1("wifiSSLSocketClose_lib:%d", ret);
                }
                return NULL;
            }
        }
    } else {
        MAINLOG_L1("Creating new GPRS socket connection to %s:%s (SSL: %d)", host, port, ssl);

        timeid = TimerSet_Api();
        while(1) {
            ret = wirelessCheckPppDial_lib();
            if(ret == 0)
                break;
            else {
                MAINLOG_L1("timerOutMs = %d  %d" , timerOutMs , ret );
                ret = wirelessPppOpen_lib(NULL, NULL, NULL);
                MAINLOG_L1("wirelessPppOpen_lib = %d" , ret );
                Delay_Api(1000);
            }
            if(TimerCheck_Api(timeid , timerOutMs) == 1) {
                MAINLOG_L1("wirelessCheckPppDial_lib 2 = %d" , ret);
                *errCode = -1;
                return NULL;
            }
        }

        if (ssl == 0) {
            sock = wirelessSocketCreate_lib(0);
            MAINLOG_L1("wirelessSocketCreate_lib = %ld" , sock);
            if (sock < 0) {
                *errCode = -4;
                return NULL;
            }
        } else {
            wirelessSslDefault_lib();
            wirelessSetSslVer_lib(0);

            if (ssl == 2) {
                ret = wirelessSetSslMode_lib(1);
                memset(CerBuf, 0 , sizeof(CerBuf));
                ret = getCertificate(FILE_CERT_ROOT, CerBuf);
                MAINLOG_L1("getCertificate === %d", ret);
                ret = wirelessSendSslFile_lib(2, CerBuf, strlen((char *)CerBuf));
                memset(CerBuf, 0 , sizeof(CerBuf));
                ret = getCertificate(FILE_CERT_PRIVATE, CerBuf);
                MAINLOG_L1("getCertificate === %d", ret);
                ret = wirelessSendSslFile_lib(1, CerBuf, strlen((char *)CerBuf));
                memset(CerBuf, 0 , sizeof(CerBuf));
                ret = getCertificate(FILE_CERT_CHAIN, CerBuf);
                MAINLOG_L1("getCertificate === %d", ret);
                ret = wirelessSendSslFile_lib(0, CerBuf, strlen((char *)CerBuf));
                MAINLOG_L1("wirelessSendSslFile_lib = %d" , ret );
            } else {
                ret = wirelessSetSslMode_lib(0);
                MAINLOG_L1("wirelessSetSslMode_lib = %d" , ret );
            }

            sock = wirelessSslSocketCreate_lib();
            MAINLOG_L1("wirelessSslSocketCreate_lib = %d" , sock );
            if (sock == -1) {
                *errCode = -4;
                return NULL;
            }
        }

        if (ssl == 0)
            ret = wirelessTcpConnect_lib(sock, (char *)host, port, timerOutMs);
        else {
            ret = wirelessSslConnect_lib(sock, (char *)host, port, timerOutMs);
            MAINLOG_L1("wirelessSslConnect_lib ret:%d host:%s  port:%s  ", ret , host, port );
        }

        if (ret != 0) {
            if (ssl == 0)
                wirelessSocketClose_lib(sock);
            else
                wirelessSslSocketClose_lib(sock);

            *errCode = -5;
            ScrDisp_Api(LINE9, 0, "Connect failed", CDISP);
            return NULL;
        }
    }

    // Store connection details for future reuse
    sockets[i].valid = 1;
    sockets[i].ssl = ssl;
    sockets[i].socket = sock;
    sockets[i].ref_count = 1;
    strncpy(sockets[i].host, host, MAX_DOMAIN_LENGTH-1);
    strncpy(sockets[i].port, port, sizeof(sockets[i].port)-1);

    MAINLOG_L1("Created new socket connection to %s:%s (index: %d)", host, port, i);

    *errCode = 0;
    return &sockets[i];
}

int net_close(void *netContext)
{
    int ret;
    NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

    if (sock == NULL)
        return -1;

    if (sock->valid == 0)
        return 0;

    // Decrement reference counter
    sock->ref_count--;

    MAINLOG_L1("Decremented socket reference count for %s:%s (ref: %d)",
              sock->host, sock->port, sock->ref_count);

    // Only close the socket when reference count reaches zero
    if (sock->ref_count <= 0) {
        MAINLOG_L1("Closing socket connection to %s:%s (socket: %d)",
                  sock->host, sock->port, sock->socket);

        if (!_IS_GPRS_ENABLED_) {
            if (sock->ssl == 0) {
                ret = wifiTCPClose_lib(sock->socket);
                MAINLOG_L1("wifiTCPClose_lib:%d", ret);

                ret = wifiSocketClose_lib(sock->socket);
                MAINLOG_L1("wifiSocketClose_lib:%d", ret);
            } else {
                ret = wifiSSLClose_lib(sock->socket);
				MAINLOG_L1("wifiSSLClose_lib(): %d", ret);
				if (ret != 0) {
						MAINLOG_L1("Socket Close Error, or Server Shut Down the Connection Actively");
				}

				ret = wifiSSLSocketClose_lib(sock->socket);
				MAINLOG_L1("wifiSSLSocketClose_lib(): %d", ret);
				if (ret != 0) {
						MAINLOG_L1("Socket Release Error.");
				}
            }
        } else {
            if (sock->ssl == 0)
                wirelessSocketClose_lib(sock->socket);
            else
                wirelessSslSocketClose_lib(sock->socket);
        }

        sock->valid = 0;
        sock->ref_count = 0;

        // Reset global references if they point to this socket
        if (g_tms_socket == sock)
            g_tms_socket = NULL;
        if (g_mqtt_socket == sock)
            g_mqtt_socket = NULL;
        if (g_api_socket == sock)
            g_api_socket = NULL;
    }

    return 0;
}

int net_read(void *netContext, unsigned char* recvBuf, int needLen, int timeOutMs)
{
    int ret;
    NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

    if (sock == NULL)
        return -1;

    if (sock->valid == 0)
        return -1;

    if (!_IS_GPRS_ENABLED_) {
        if (sock->ssl == 0)
            ret = wifiRecv_lib(sock->socket, recvBuf, 1, timeOutMs);
        else
            ret = wifiSSLRecv_lib(sock->socket, recvBuf, 1, timeOutMs);

        if(ret == 0) {
            return 0;
        }
        else if(ret < 0) {
            MAINLOG_L1("wifiRecv_lib:%d", ret);
            return -1;
        }

        if (needLen == 1)
            return 1;

        if (sock->ssl == 0)
            ret = wifiRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);
        else
            ret = wifiSSLRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);

        if(ret < 0)
            return -1;

        return ret + 1;
    } else {
        if (sock->ssl == 0)
            ret = wirelessRecv_lib(sock->socket, recvBuf, 1, timeOutMs);
        else {
            ret = wirelessSslRecv_lib(sock->socket, recvBuf, 1, timeOutMs);
        }

        if (ret == 0)
            return 0;

        if (ret != 1) {
            return -1;
        }

        if (needLen == 1)
            return 1;

        if (sock->ssl == 0) {
            ret = wirelessRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);
        } else {
            ret = wirelessSslRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);
        }

        if(ret < 0) {
            return -1;
        }

        return ret + 1;
    }
}

int net_write(void *netContext, unsigned char* sendBuf, int sendLen, int timeOutMs)
{
    int ret;
    NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

    if (sock == NULL)
        return -1;

    if (sock->valid == 0)
        return -1;

    if (!_IS_GPRS_ENABLED_) {
        if (sock->ssl == 0)
            ret = wifiSend_lib(sock->socket, sendBuf, sendLen, timeOutMs);
        else
            ret = wifiSSLSend_lib(sock->socket, sendBuf, sendLen, timeOutMs);
        MAINLOG_L1("wifiSend_lib:%d", ret);
        if(ret == 0)
            return sendLen;
        else
            return -1;
    } else {
        if (sock->ssl == 0)
            return wirelessSend_lib(sock->socket, sendBuf, sendLen);
        else
            return wirelessSslSend_lib(sock->socket, sendBuf, sendLen);
    }
}
