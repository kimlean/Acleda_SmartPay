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

    // Input validation
    if (!host || !port || !errCode) {
        MAINLOG_L1("Invalid parameters passed to net_connect");
        *errCode = -1;
        return NULL;
    }

    MAINLOG_L1("Connection requested to %s:%s (SSL: %d)", host, port, ssl);

    // Step 1: Check if we already have a connection to this host:port
    for (i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].valid &&
            sockets[i].ssl == ssl &&
            strcmp(sockets[i].host, host) == 0 &&
            strcmp(sockets[i].port, port) == 0) {

            // Found an existing connection, increment reference counter
            sockets[i].ref_count++;
            *errCode = 0;

            MAINLOG_L1("Reusing existing socket connection to %s:%s (slot: %d, ref: %d)",
                      host, port, i, sockets[i].ref_count);

            return &sockets[i];
        }
    }

    // Find an empty socket slot for a new connection
    for (i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].valid == 0)
            break;
    }

    if (i >= MAX_SOCKETS) {
        // If all slots are used, try to find one with lowest reference count
        int min_refs = 0;
        int min_slot = -1;

        for (i = 0; i < MAX_SOCKETS; i++) {
            // Skip global reference sockets
            if (&sockets[i] == g_tms_socket ||
                &sockets[i] == g_mqtt_socket ||
                &sockets[i] == g_api_socket)
                continue;

            if (sockets[i].ref_count < min_refs) {
                min_refs = sockets[i].ref_count;
                min_slot = i;
            }
        }

        if (min_slot >= 0 && min_refs <= 1) {
            MAINLOG_L1("Reusing socket slot %d with ref count %d", min_slot, min_refs);
            // Close the socket with lowest reference count
            net_close(&sockets[min_slot]);
            i = min_slot;
        } else {
            *errCode = -3;
            MAINLOG_L1("ERROR: All socket slots are in use (%d max) and none can be reused", MAX_SOCKETS);
            return NULL;
        }
    }

    MAINLOG_L1("Using socket slot %d for new connection", i);

    // Create new socket based on connection type
    if (!_IS_GPRS_ENABLED_) {
        MAINLOG_L1("Creating new WIFI socket connection to %s:%s (SSL: %d)", host, port, ssl);

        ret = wifiGetLinkStatus_lib();
        MAINLOG_L1("wifiGetLinkStatus_lib:%d", ret);
        if(ret <= 0) {
            *errCode = -3;
            return NULL;
        }

        if(ssl == 0) {
            // >=0 success, others -failed
            sock = wifiSocketCreate_lib(0); //0-TCP   1-UDP
            MAINLOG_L1("wifiSocketCreate_lib:%d", sock);
            if(sock < 0) {
                *errCode = -4;
                return NULL;
            }

            ret = wifiTCPConnect_lib(sock, host, port, 60 * 1000);
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
                MAINLOG_L1("wirelessCheckPppDial_lib timeout = %d" , ret);
                *errCode = -1;
                return NULL;
            }
        }

        if (ssl == 0) {
            sock = wirelessSocketCreate_lib(0);
            MAINLOG_L1("wirelessSocketCreate_lib = %d" , sock);
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
            MAINLOG_L1("wirelessSslConnect_lib ret:%d host:%s port:%s", ret, host, port);
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
    sockets[i].host[MAX_DOMAIN_LENGTH-1] = '\0'; // Ensure null termination
    strncpy(sockets[i].port, port, sizeof(sockets[i].port)-1);
    sockets[i].port[sizeof(sockets[i].port)-1] = '\0'; // Ensure null termination

    MAINLOG_L1("Created new socket connection to %s:%s (index: %d)", host, port, i);

    *errCode = 0;
    return &sockets[i];
}

int net_close(void *netContext)
{
    int ret = 0;
    NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

    // Validate the socket pointer
    if (sock == NULL) {
        MAINLOG_L1("Warning: Attempted to close NULL socket");
        return 0;  // Not an error to close NULL socket
    }

    // Check if socket is already invalid
    if (sock->valid == 0) {
        MAINLOG_L1("Warning: Attempted to close already invalid socket");
        return 0;  // Not an error to close invalid socket
    }

    // Decrement reference counter
    sock->ref_count--;

    MAINLOG_L1("Decremented socket reference count for %s:%s (ref: %d)",
              sock->host, sock->port, sock->ref_count);

    // Only close the socket when reference count reaches zero
    if (sock->ref_count <= 0) {
        MAINLOG_L1("Closing socket connection to %s:%s (socket: %d)",
                  sock->host, sock->port, sock->socket);

        // Close the actual socket based on connection type
        if (!_IS_GPRS_ENABLED_) {
            if (sock->ssl == 0) {
                // Try to close TCP connection first
                ret = wifiTCPClose_lib(sock->socket);
                MAINLOG_L1("wifiTCPClose_lib:%d", ret);

                // Then close socket regardless of TCP close result
                ret = wifiSocketClose_lib(sock->socket);
                MAINLOG_L1("wifiSocketClose_lib:%d", ret);
            } else {
                // SSL connection - close SSL first
                ret = wifiSSLClose_lib(sock->socket);
                MAINLOG_L1("wifiSSLClose_lib(): %d", ret);
                if (ret != 0) {
                    MAINLOG_L1("Socket Close Error, or Server Shut Down the Connection Actively");
                    // Continue to close the socket even if SSL close had issues
                }

                // Then close the socket itself
                ret = wifiSSLSocketClose_lib(sock->socket);
                MAINLOG_L1("wifiSSLSocketClose_lib(): %d", ret);
                if (ret != 0) {
                    MAINLOG_L1("Socket Release Error.");
                    // Continue with cleanup even if socket close had issues
                }
            }
        } else {
            // GPRS connection
            if (sock->ssl == 0) {
                ret = wirelessSocketClose_lib(sock->socket);
                MAINLOG_L1("wirelessSocketClose_lib(): %d", ret);
            } else {
                ret = wirelessSslSocketClose_lib(sock->socket);
                MAINLOG_L1("wirelessSslSocketClose_lib(): %d", ret);
            }
        }

        // Mark socket as invalid and reset counter
        sock->valid = 0;
        sock->ref_count = 0;
        sock->socket = -1;  // Mark socket as closed

        // Reset global references if they point to this socket
        if (g_tms_socket == sock) {
            MAINLOG_L1("Clearing g_tms_socket reference");
            g_tms_socket = NULL;
        }
        if (g_mqtt_socket == sock) {
            MAINLOG_L1("Clearing g_mqtt_socket reference");
            g_mqtt_socket = NULL;
        }
        if (g_api_socket == sock) {
            MAINLOG_L1("Clearing g_api_socket reference");
            g_api_socket = NULL;
        }
    } else {
        MAINLOG_L1("Socket to %s:%s still has %d references, not closing yet",
                  sock->host, sock->port, sock->ref_count);
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
