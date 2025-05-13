#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <coredef.h>
#define EXTERN extern
#include <struct.h>
#include <poslib.h>
#include "def.h"
#include "httpDownload.h"
#include "tms_md5.h"
#include "tms_tms.h"

static unsigned char *t_sBuf=NULL;
static unsigned char *t_rBuf=NULL;
int Gfunflag = 0; //0-tms log not output     1-tms log output    2-unzip mp3 file
int _tms_G_dbgOutDestApp = 0;

struct __TmsTrade__ TmsTrade;

int TmsConnect_Api()
{
    int errCode = 0;

    // Use the global TMS socket if available
    if (g_tms_socket != NULL && g_tms_socket->valid) {
        MAINLOG_L1("Using existing TMS socket connection");
        return 0;
    }

    // Otherwise establish a new connection and store it
    g_tms_socket = net_connect(NULL, tmsEntry.domain, tmsEntry.port,
                             CONNECT_TIMEOUT * 1000,
                             tmsEntry.protocol == PROTOCOL_HTTPS ? 1 : 0,
                             &errCode);

    if (g_tms_socket != NULL && errCode == 0) {
        MAINLOG_L1("Stored new TMS socket connection to %s:%s",
                  tmsEntry.domain, tmsEntry.port);
        return 0;
    }

    MAINLOG_L1("Failed to establish TMS connection (error: %d)", errCode);
    return errCode ? errCode : -1;
}

int TmsReceive(unsigned char *buf, int max, int timeout)
{
    if (g_tms_socket == NULL) {
        MAINLOG_L1("TMS socket not connected when trying to receive");
        return RECEIVE_ERROR;
    }

    int ret = net_read(g_tms_socket, buf, max, timeout * 1000);
    if (ret < 0) {
        MAINLOG_L1("TMS receive error: %d", ret);
        return RECEIVE_ERROR;
    }

    return ret;
}

int TmsDisconnect(void)
{
    if (g_tms_socket == NULL)
        return 0;

    MAINLOG_L1("Closing TMS socket connection");
    int ret = net_close(g_tms_socket);
    g_tms_socket = NULL;
    return ret;
}

// Replace TmsSend with this improved version
int TmsSend(unsigned char *buf, int length)
{
    if (g_tms_socket == NULL) {
        MAINLOG_L1("TmsSend: TMS socket not connected");
        return SEND_ERROR;
    }

    MAINLOG_L1("TmsSend: Sending %d bytes to server", length);
    int ret = net_write(g_tms_socket, buf, length, 10000); // 10 second timeout

    if (ret < 0) {
        MAINLOG_L1("TmsSend: net_write error: %d", ret);
        return SEND_ERROR;
    }
    else if (ret != length) {
        MAINLOG_L1("TmsSend: Incomplete send - sent %d of %d bytes", ret, length);
        return SEND_ERROR;
    }

    MAINLOG_L1("TmsSend: Successfully sent %d bytes", length);
    return 0;
}

#ifdef JUMPER_EXIST
void Tms_DispJumpSec(void)
{
#ifdef DISP_STH
	char DispBuf[20];
	memset(DispBuf, 0, sizeof(DispBuf));
	_tms_g_MyJumpSec.Sec++;
	sprintf(DispBuf, "%d", _tms_g_MyJumpSec.Sec);
	ScrDisp_Api(LINE5, 0, DispBuf, CDISP);
#endif
}
#endif

int Tms_StartJumpSec(void)
{
#ifdef JUMPER_EXIST
	_tms_g_MyJumpSec.Timer = CommStartJumpSec_Api(Tms_DispJumpSec);
#endif
	return 0;
}

void Tms_StopJumpSec(void)
{
#ifdef JUMPER_EXIST
	_tms_g_MyJumpSec.Sec = 0;
	CommStopJumpSec_Api(_tms_g_MyJumpSec.Timer);
#endif
}

int Tms_RecvPacket(u8 *Packet, int *PacketLen, int WaitTime)
{
    int Ret;
    unsigned short tlen = 0;
    int clen, stlen;

    while(1)
    {
        Tms_StartJumpSec();
        Ret = TmsReceive(Packet+tlen, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
        Tms_StopJumpSec();

        if (Ret <= 0) {
            Ret = RECEIVE_ERROR;
            break;
        }
        tlen += Ret;
        clen = 0;
        stlen = 0;
        Ret = 0;
        if(Tms_getContentLen(Packet, &clen, &stlen) == 0)
        {
            if(tlen > stlen)
            {
                Ret = _TMS_E_RECV_PACKET;
                break;
            }
            else if(tlen == stlen)
            {
                *PacketLen = tlen;
                Ret = 0;
                break;
            }
        }
    }
    if(Ret != 0)
        TmsDisconnect();
    return Ret;
}

int Tms_UrlRecvPacket(u8 *Packet, int *PacketLen, int WaitTime)
{
    unsigned short len;
    int Ret;
    u8 *p = NULL;
    int tlen = 0, curfidx;
    int clen = 0, stlen=0, hflag = 0, floc = 0;

    curfidx = TmsTrade.curfindex;
    floc = Tms_GetFileSize(&(TmsTrade.file[TmsTrade.curfindex]));

    while(1)
    {
        Tms_StartJumpSec();
        if(hflag == 0)
            Ret = TmsReceive(Packet+tlen, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
        else
            Ret = TmsReceive(Packet, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
        Tms_StopJumpSec();
        if (Ret <= 0) {
            Ret = RECEIVE_ERROR;
            break;
        }

#ifdef DISP_STH
        memset(tmp, 0, sizeof(tmp));
        ScrClrLine_Api(LINE3, LINE4);
        sprintf(tmp, "%d%% %d/%d", (TmsTrade.file[TmsTrade.curfindex].startPosi*100)/TmsTrade.file[TmsTrade.curfindex].fsize, TmsTrade.file[TmsTrade.curfindex].startPosi, TmsTrade.file[TmsTrade.curfindex].fsize);
        ScrDisp_Api(LINE3, 0, TmsTrade.file[TmsTrade.curfindex].name, FDISP|CDISP);
        ScrDisp_Api(LINE4, 0, tmp,FDISP|CDISP);
#endif
        len = Ret;
        Ret = 0;
        tlen += len;
        if(hflag == 0)
        {
            if(Tms_getContentLen(Packet, &clen, &stlen) != 0)
                continue;

            if(floc+clen != TmsTrade.file[TmsTrade.curfindex].fsize)
            {
                Ret = _TMS_E_RECV_PACKET;
                Tms_LstDbgOutApp("flocfloc:", (u8 *)&floc, sizeof(floc), 0);
                Tms_LstDbgOutApp("clenclen:", (u8 *)&clen, sizeof(clen), 0);
                Tms_LstDbgOutApp("fsize:", (u8 *)&(TmsTrade.file[TmsTrade.curfindex].fsize), sizeof(TmsTrade.file[TmsTrade.curfindex].fsize), 0);
                break;
            }
            p = (u8 *)strstr((char *)Packet, "\r\n\r\n");
            if(tlen-(p+4-Packet) > 0)
            {
                Ret = Tms_WriteFile(&TmsTrade.file[TmsTrade.curfindex], p+4, tlen-(p+4-Packet));
                if(Ret != 0)
                {
                    Ret = _TMS_E_FILE_WRITE;
                    Tms_LstDbgOutApp("Tms_WriteFile:", (u8 *)&Ret, sizeof(Ret), 0);
                    break;
                }

                floc += (tlen-(p+4-Packet));
            }
            hflag = 1;
        }
        else
        {
            Ret = Tms_WriteFile(&TmsTrade.file[TmsTrade.curfindex], Packet, len);
            if(Ret != 0)
            {
                Ret = _TMS_E_FILE_WRITE;
                Tms_LstDbgOutApp("Retwr1:", (u8 *)&Ret, sizeof(Ret), 0);
                break;
            }

            floc += len;
        }

        if(tlen > stlen)
        {
            Ret = _TMS_E_RECV_PACKET;
            Tms_LstDbgOutApp("tlen tlen:", (u8 *)&tlen, sizeof(tlen), 0);
            Tms_LstDbgOutApp("stlen stlen:", (u8 *)&stlen, sizeof(stlen), 0);
            break;
        }
        else if(tlen == stlen)
        {
            Ret = 0;
            *PacketLen = tlen;
            Tms_LstDbgOutApp((TmsTrade.file[curfidx].name), (unsigned char*)" down over!", strlen(" down over!"), 1);
            break;
        }
    }
    if(Ret != 0)
        TmsDisconnect();
    return Ret;
}

int Tms_SendRecvPacket(u8 *SendBuf, int Senlen, u8 *RecvBuf, int *pRecvLen)
{
    int ret = Tms_SendRecvData(SendBuf, Senlen, RecvBuf, pRecvLen, TmsStruct.tradeTimeoutValue);
    return ret;
}


int Tms_SendRecvData(unsigned char *SendBuf, int Senlen, unsigned char *RecvBuf, int *RecvLen, int psWaitTime)
{
    int ret=0;

    ret = TmsSend(SendBuf, Senlen);
    Tms_DbgOutApp("send:", SendBuf, Senlen);
    if(ret != 0)
    {
        Tms_LstDbgOutApp("Tms_SendRecvData CommTxd_Api:", (u8 *)&ret, sizeof(ret), 0);
        if(ret != 0xff)
            return _TMS_E_SEND_PACKET;

        ret = Tms_ReSend(SendBuf, Senlen);
        if(ret != 0)
            return _TMS_E_SEND_PACKET;
    }

    *RecvLen = 0;
    if(TmsTrade.trade_type == TYPE_URLGETFILE || TmsTrade.trade_type == TYPE_QR)
    {
        ret = Tms_UrlRecvPacket(RecvBuf, RecvLen, psWaitTime);
    }
    else
    {
        ret = Tms_RecvPacket(RecvBuf, RecvLen, psWaitTime);
        Tms_DbgOutApp("Tms_RecvPacket:", RecvBuf, *RecvLen); //ascii
    }
    return ret;
}

///Connect to host/send receive data  end


//Create Resolve package start
int Tms_CreatePacket(u8 * packData, int * packLen) //ck
{
	cJSON *root = NULL;
	char *out = NULL;
	int dataLen, i;
	u8 buf[64], tmp[64], timestamp[16], urlPath[128], md5src[1024];

	memset(urlPath, 0, sizeof(urlPath));
	root = cJSON_CreateObject();

	cJSON_AddStringToObject((cJSON *)root, "version", TmsStruct.version);   //version is unuseful
	cJSON_AddStringToObject((cJSON *)root, "sn", TmsStruct.sn);
	cJSON_AddStringToObject((cJSON *)root, "manufacturer", TmsStruct.manufacturer);
	cJSON_AddStringToObject((cJSON *)root, "deviceType", TmsStruct.deviceType);

	switch(TmsTrade.trade_type)
	{
		case TYPE_CHECKVERSION:   //http://aipos-s.jiewen.com.cn/tms/checkVersion
			sprintf((char *)urlPath, "POST /tms/checkVersion"); //POST /tms/checkVersion
			break;
		case TYPE_NOTIFY:
			sprintf((char *)urlPath, "POST /tms/resultNotify");  //POST /spp/tms/resultNotify
			if(Tms_CheckDownloadOver() == 0)
				cJSON_AddStringToObject(root, "upgradeResult", "00");
			else
				cJSON_AddStringToObject(root, "upgradeResult", "99");
			break;
		case TYPE_QR:
			sprintf((char *)urlPath,  "GET /tms/Vanstone/QR/%s/%s_KHR.zip",
					G_sys_param.sn,
					G_sys_param.sn);
			break;
		default:
			break;
	}

	out = cJSON_PrintUnformatted(root);
	dataLen = strlen(out);
	memcpy((char *)packData+strlen((char *)packData), urlPath, strlen((char *)urlPath));
	strcat((char *)packData, " HTTP/1.1\r\n");

	memset(timestamp, 0, sizeof(timestamp));
	GetSysTime_Api(tmp);
	BcdToAsc_Api((char *)timestamp, (unsigned char *)tmp, 14);
	memset(tmp, 0x20, sizeof(tmp));
	memset(buf, 0x20, sizeof(buf));
	memcpy(tmp, TmsStruct.sn, strlen(TmsStruct.sn));
	memcpy(buf, timestamp, strlen((char *)timestamp));

	for(i=0; i<50; i++)
		tmp[i] ^= buf[i];

	memset(buf, 0x20, sizeof(buf));
	memcpy(buf, "998876511QQQWWeerASDHGJKL", strlen("998876511QQQWWeerASDHGJKL"));

	for(i=0; i<50; i++)
		tmp[i] ^= buf[i];

	tmp[50] = 0;
	memset(buf, 0, sizeof(buf));
	memset(md5src, 0, sizeof(md5src));
	memcpy(md5src, out, dataLen);

	BcdToAsc_Api((char *)(md5src+dataLen), (unsigned char *)tmp, 100);
	_tms_MDString((char *)md5src,(unsigned char *)buf);
	memset(tmp, 0, sizeof(tmp));
	BcdToAsc_Api((char *)tmp, (unsigned char *)buf, 32);
	sprintf((char *)packData+strlen((char *)packData), "sign: %s\r\n", tmp);

	sprintf((char *)packData+strlen((char *)packData), "timestamp: %s\r\n", timestamp);
	sprintf((char *)packData+strlen((char *)packData), "Content-Length: %d\r\n", strlen((char *)out));
	strcat((char *)packData, "Content-Type: application/json;charset=UTF-8\r\n");
	sprintf((char *)packData+strlen((char *)packData), "Host: %s\r\n", TmsStruct.hostDomainName);
	strcat((char *)packData, "Connection: Keep-Alive\r\n");
	strcat((char *)packData, "User-Agent: Apache-HttpClient/4.3.5 (java 1.5)\r\n");
	strcat((char *)packData, "Accept-Encoding: gzip,deflate\r\n\r\n");
	memcpy(packData+strlen((char *)packData), out, dataLen);
	*packLen = strlen((char *)packData);
	free(out);
	cJSON_Delete(root);

	return 0;
}

//return 0:sucess other:failed
u8 Tms_HttpGetTranResult(u8 * data) //ck
{
	u8 *p = NULL, tmp[64];

	if((p = (u8 *)strstr((char *)data, "\n")) == NULL)  //\r\n
		return _TMS_E_RESOLVE_PACKET;
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, data, p-data);
	if((p = (u8 *)strstr((char *)tmp, "200")) == NULL)
	{
		memset(TmsTrade.respMsg, 0, sizeof(TmsTrade.respMsg));
		if(strlen((char *)tmp) > (sizeof(TmsTrade.respMsg)-1))
			memcpy(TmsTrade.respMsg, tmp, sizeof(TmsTrade.respMsg)-1);
		else
			memcpy(TmsTrade.respMsg, tmp, strlen((char *)tmp));
		return _TMS_E_TRANS_FAIL;    //transaction failed if not "HTTP/1.1 200 OK"
	}
	return 0;
}


//not for TYPE_URLGETFILE
int  Tms_ResolveHTTPPacket(u8 *recvPack) //ck
{
	char *ptr = NULL;
	int ret;

	ret = Tms_HttpGetTranResult(recvPack);
	if(ret != 0)
	{
		Tms_LstDbgOutApp("Tms_HttpGetTranResult:", (u8 *)&ret, sizeof(ret), 0);
		return ret;
	}

	if((ptr = strstr((char *)recvPack, "{")) == NULL)
	{
		if(ret == 0)
			return _TMS_E_RESOLVE_PACKET;
		else
			return ret;
	}

	ret = Tms_UnPackPacket(ptr, ret);
	if(pRoot != NULL)
	{
		cJSON_Delete(pRoot);
		pRoot = NULL;
	}
	return ret;
}

//return :  E_RESOLVE_PACKET - resolve failed     0-resolve success
int Tms_UnPackPacket(char * data, int reslt) //ck
{
	cJSON * pItem = NULL, * pitem = NULL;
	int len;

	if(pRoot != NULL)
	{
		cJSON_Delete(pRoot);
		pRoot = NULL;
	}
	pRoot = cJSON_Parse(data);
	// MAINLOG_L1("Tms_hort11111:%s", pRoot);
	if(pRoot == NULL)
		return _TMS_E_RESOLVE_PACKET;

	pItem = cJSON_GetObjectItem(pRoot, "version");
	if((pItem != NULL) && (pItem->type != cJSON_NULL))
	{
		if(strcmp(pItem->valuestring, TmsStruct.version))
			return _TMS_E_PACKAGE_WRONG;
	}

	pItem = cJSON_GetObjectItem(pRoot, "sn");
	if((pItem != NULL) && (pItem->type != cJSON_NULL))
	{
		if(strcmp(pItem->valuestring, TmsStruct.sn) != 0)
			return _TMS_E_PACKAGE_WRONG;
	}

	pItem = cJSON_GetObjectItem(pRoot, "manufacturer");
	if((pItem != NULL) && (pItem->type != cJSON_NULL))
	{
		if(strcmp(pItem->valuestring , TmsStruct.manufacturer) != 0)
			return _TMS_E_PACKAGE_WRONG;
	}

	pItem = cJSON_GetObjectItem(pRoot, "deviceType");
	if((pItem != NULL) && (pItem->type != cJSON_NULL))
	{
		if(strcmp(pItem->valuestring , TmsStruct.deviceType) != 0)
			return _TMS_E_PACKAGE_WRONG;
	}

	pItem = cJSON_GetObjectItem(pRoot, "respCode");
	if((pItem != NULL) && (pItem->type != cJSON_NULL))
		memcpy(TmsTrade.respCode, pItem->valuestring, strlen(pItem->valuestring));

	pItem = cJSON_GetObjectItem(pRoot, "respMsg");
	if((pItem != NULL) && (pItem->type != cJSON_NULL))
		memcpy(TmsTrade.respMsg, pItem->valuestring, strlen(pItem->valuestring));

	if((strcmp(TmsTrade.respCode, "00") != 0) || (reslt != 0))
		return _TMS_E_TRANS_FAIL;

	switch(TmsTrade.trade_type)
	{
	case TYPE_CHECKVERSION:
		pItem = cJSON_GetObjectItem(pRoot, "fileList");

		if((pItem == NULL) && (pItem->type != cJSON_Array))
			break;

		pItem = pItem->child;
		if(pItem == NULL)
			return reslt;
		while(pItem != NULL)
		{
			pitem = cJSON_GetObjectItem(pItem, "fileName");
			if((pitem != NULL) && (pitem->type != cJSON_NULL))
			{
				len = sizeof(TmsTrade.file[TmsTrade.fnum].name)-1;
				memcpy(TmsTrade.file[TmsTrade.fnum].name, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
			}

			pitem = cJSON_GetObjectItem(pItem, "fileType");
			if((pitem != NULL) && (pitem->type != cJSON_NULL))
			{
				len = sizeof(TmsTrade.file[TmsTrade.fnum].type)-1;
				memcpy(TmsTrade.file[TmsTrade.fnum].type, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
			}

			pitem = cJSON_GetObjectItem(pItem, "fileVersion");
			if((pitem != NULL) && (pitem->type != cJSON_NULL))
			{
				len = sizeof(TmsTrade.file[TmsTrade.fnum].version)-1;
				memcpy(TmsTrade.file[TmsTrade.fnum].version, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
			}

			pitem = cJSON_GetObjectItem(pItem, "fileSize");
			if((pitem != NULL) && (pitem->type != cJSON_NULL))
				TmsTrade.file[TmsTrade.fnum].fsize = (int)AscToLong_Api((unsigned char *)(pitem->valuestring), strlen(pitem->valuestring));

			pitem = cJSON_GetObjectItem(pItem, "filePath");
			if((pitem != NULL) && (pitem->type != cJSON_NULL))
			{
				len = sizeof(TmsTrade.file[TmsTrade.fnum].filePath)-1;
				memcpy(TmsTrade.file[TmsTrade.fnum].filePath, pitem->valuestring, strlen(pitem->valuestring) > len ? len:strlen(pitem->valuestring));
			}

			pitem = cJSON_GetObjectItem(pItem, "md5");
			if((pitem != NULL) && (pitem->type != cJSON_NULL))
			{
				AscToBcd_Api(TmsTrade.file[TmsTrade.fnum].md5, pitem->valuestring, 32);
			}

			if(Tms_CheckNeedDownFile(&TmsTrade.file[TmsTrade.fnum]) == 0) //necessary to download
			{
				TmsTrade.fnum++;
				Tms_LstDbgOutApp("fnum will be loaded:", (unsigned char *)&(TmsTrade.fnum), sizeof(TmsTrade.fnum), 0);
			}
			else
				memset(&TmsTrade.file[TmsTrade.fnum], 0, sizeof(TmsTrade.file[TmsTrade.fnum]));

			pItem = pItem->next;
			if(TmsTrade.fnum > _TMS_MAX_APPLIB)
				return _TMS_E_TOO_MANY_FILES;
		}
		break;
	case TYPE_NOTIFY:
		break;
	default:
		return 1;
	}
	return 0;
}

//Build Resolve package end

//Tms update start
//hostDomainName : necessary
int TmsParamSet_Api(char *hostIp, char *hostPort, char *hostDomainName) //ck
{
	memset(&TmsTrade, 0, sizeof(struct __TmsTrade__));
	memset(&TmsStruct, 0, sizeof(struct __TmsStruct__));

	if((hostDomainName == NULL) || (strlen(hostDomainName) == 0))
		return -1;

	strcpy(TmsStruct.hostDomainName, hostDomainName);

	if((hostPort != NULL) && (strlen(hostPort) != 0))
		strcpy(TmsStruct.hostPort, hostPort);

	if((hostIp != NULL) && (strlen(hostIp) != 0))
		strcpy(TmsStruct.hostIP, hostIp);

	return 0;
}


void Tms_SetTermParam(u8 *oldAppVer) //ck
{
	char tmp[64], ret;

	pRoot = NULL;
	strcpy(TmsStruct.manufacturer, "Vanstone"); ////Vanstone
	strcpy(TmsStruct.version, "1.0");
	TmsStruct.tradeTimeoutValue = 60;
	strcpy(TmsStruct.oldAppVer, (char *)oldAppVer);  //App_Msg.Version

	//deviceType
	//ret1 = sysGetTermType_lib(tmp);
	//if ((ret1 >= 0) && ((memcmp(tmp, "Q180D", 5) == 0) || (memcmp(tmp, "Q190", 4) == 0)))

	strcpy(TmsStruct.deviceType, MACHINE_Q161);

	strcpy(TmsTrade.deviceType, TmsStruct.deviceType);

	//SN
	memset(tmp, 0, sizeof(tmp));
	ret = PEDReadPinPadSn_Api((unsigned char*)tmp);
	if(ret == 0)
	{
		int len = (int)AscToLong_Api((unsigned char*)tmp, 2);
		memcpy(TmsStruct.sn, tmp+2, len);
	}
}

int Tms_Request(void) //ck
{
	int PackLen=0;
	int ret=0, flen;

	memset(TmsTrade.respCode, 0, sizeof(TmsTrade.respCode));
	memset(TmsTrade.respMsg, 0, sizeof(TmsTrade.respMsg));

	if(TmsTrade.trade_type != TYPE_QR)
		TmsTrade.trade_type = TYPE_CHECKVERSION;
	else
		TmsConnect_Api();

	MAINLOG_L1("TmsTrade.trade_type %d", TmsTrade.trade_type);

	memset(t_sBuf, 0, SENDPACKLEN);
	ret = Tms_CreatePacket(t_sBuf, &PackLen);
	if(ret != 0)
		return ret;

	memset(t_rBuf, 0, RECVPACKLEN);
	ret = Tms_SendRecvPacket(t_sBuf, PackLen, t_rBuf, &PackLen);
	if(ret != 0)
	{
		Tms_LstDbgOutApp("Tms_SendRecvPacket:", (u8 *)&ret, sizeof(ret), 0);
		return ret;
	}

	ret = Tms_ResolveHTTPPacket(t_rBuf);
	if(ret != 0)
	{
		Tms_LstDbgOutApp("Tms_ResolveHTTPPacket:", (u8 *)&ret, sizeof(ret), 0);
		return ret;
	}

	if(TmsTrade.fnum <= 0)
	{
		memset(TmsTrade.respCode, 0, sizeof(TmsTrade.respCode));
		memset(TmsTrade.respMsg, 0, sizeof(TmsTrade.respMsg));
		return _TMS_E_NOFILES_D;
	}

	flen = GetFileSize_Api(_DOWN_STATUS_);
	if(flen > 0)
	{
		Tms_CompareVersions(&TmsTrade);
	}

	return ret;
}

void Tms_DelAllDownloadInfo(struct __TmsTrade__ *GFfile) //ck
{
	int i;
	char tmp[64], buf[64];

	for(i = 0; i <= GFfile->curfindex; i++)
	{
		if((strcmp(GFfile->file[i].type, TYPE_FIRMWARE) == 0) || (strcmp(GFfile->file[i].type, TYPE_APP) == 0))
		{
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "%s%s", TMS_FILE_DIR, GFfile->file[i].name);
			DelFile_Api(tmp);
		}
		else if(strcmp(GFfile->file[i].type, TYPE_PARAM) == 0)
		{
			memset(tmp, 0, sizeof(tmp));
			memset(buf, 0, sizeof(buf));
			sprintf(tmp, "%s%s", TMS_FILE_DIR, GFfile->file[i].name);
			strcat(buf, ".bak");
			DelFile_Api(tmp);
			DelFile_Api(buf);
		}
	}
	DelFile_Api(_DOWN_STATUS_);
}

//return : 1-not match     0-match     //Curr:current     GFfile:get from file
int Tms_CompareVersions(struct __TmsTrade__ *Curr) //ck
{
	int i, flag = 0, ret, len;
	struct __TmsTrade__ GFfile;

	len = sizeof(struct __TmsTrade__);
	ret = ReadFile_Api(_DOWN_STATUS_, (unsigned char*)&GFfile, 0, (unsigned int *)&len);
	if(ret != 0)
		goto _TMS_DEL_ALL;

	if((Curr->fnum != GFfile.fnum) )  //file exist // need to compare all files and versions
		goto _TMS_DEL_ALL;

	for(i = 0; i <= GFfile.fnum; i++)
	{
		if((strcmp(GFfile.file[i].name, Curr->file[i].name) == 0) && (strcmp(GFfile.file[i].type, Curr->file[i].type) == 0)
			&& (strcmp(GFfile.file[i].version, Curr->file[i].version) == 0) && (GFfile.file[i].fsize == Curr->file[i].fsize))
		{
				continue;
		}
		else
		{
			flag = 1;
			break;
		}
	}
	if(flag == 0)  //all same, use information in the files
	{
		memcpy(Curr, &GFfile, sizeof(GFfile));
		return 0;
	}

_TMS_DEL_ALL :
	Tms_DelAllDownloadInfo(&GFfile);
	return 1;
}


int Tms_Notify(void) //ck
{
	int PackLen=0;
	int ret=0, i, flag = 0;

	for(i=0; i<TmsTrade.fnum; i++)
	{
		if(TmsTrade.file[i].status == STATUS_DLUNCOMPLETE)
		{
			flag = 1;
			break;
		}
	}

	if(flag == 1)
		return _TMS_E_DOWNUNCOMPLETE;

	memset(TmsTrade.respCode, 0, sizeof(TmsTrade.respCode));
	memset(TmsTrade.respMsg, 0, sizeof(TmsTrade.respMsg));
	TmsTrade.trade_type = TYPE_NOTIFY;

	memset(t_sBuf, 0, SENDPACKLEN);
	ret = Tms_CreatePacket(t_sBuf, &PackLen);
	if(ret != 0)
		return ret;

	memset(t_rBuf, 0, RECVPACKLEN);
	ret = Tms_SendRecvPacket(t_sBuf, PackLen, t_rBuf, &PackLen);
	if(ret != 0)
	{
		Tms_LstDbgOutApp("notify:Tms_SendRecvPacket:", (u8 *)&ret, sizeof(ret), 0);
		return ret;
	}

	ret = Tms_ResolveHTTPPacket(t_rBuf);
	return ret;
}

int Tms_CommProcess() //ck
{
    int ret = 0;

    MAINLOG_L1("Tms_CommProcess started");

    MAINLOG_L1("Calling Tms_Request()");
    ret = Tms_Request();
    MAINLOG_L1("Tms_Request returned: %d", ret);

    Tms_LstDbgOutApp("Tms_Request:", (u8 *)&ret, sizeof(ret), 0);
    if (ret != 0)
    {
        MAINLOG_L1("Tms_Request failed, exiting with ret: %d", ret);
        return ret;
    }

    MAINLOG_L1("trade_type: %d", TmsTrade.trade_type);
    Tms_LstDbgOutApp("trade_type:", (u8 *)&TmsTrade.trade_type, sizeof(TmsTrade.trade_type), 0);

    if (TmsTrade.trade_type == TYPE_UPDATE)
    {
        MAINLOG_L1("Trade type is TYPE_UPDATE, skipping download and notify");
        return 0;
    }

    if (TmsTrade.trade_type == TYPE_NOTIFY)
    {
        MAINLOG_L1("Trade type is TYPE_NOTIFY, jumping to notification step");
        goto _TMS_NOTIFY_;
    }

    MAINLOG_L1("Trade type requires download. Calling AppPlayTip()");
    AppPlayTip("Downloading");

    MAINLOG_L1("Calling Tms_DownloadUrlFilesOneByOne()");
    ret = Tms_DownloadUrlFilesOneByOne();
    MAINLOG_L1("Tms_DownloadUrlFilesOneByOne returned: %d", ret);

    Tms_LstDbgOutApp("Tms_DownloadUrlFilesOneByOne:", (u8 *)&ret, sizeof(ret), 0);
    if (ret != 0)
    {
        MAINLOG_L1("Download failed, exiting with ret: %d", ret);
        return ret;
    }

_TMS_NOTIFY_:

    MAINLOG_L1("Calling Tms_Notify()");
    ret = Tms_Notify();
    MAINLOG_L1("Tms_Notify returned: %d", ret);

    if (ret == 0)
    {
        MAINLOG_L1("Notification success. Setting trade_type to TYPE_UPDATE and saving to file");
        TmsTrade.trade_type = TYPE_UPDATE;
        WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
    }
    else
    {
        MAINLOG_L1("Notification failed with ret: %d", ret);
    }

    MAINLOG_L1("Tms_CommProcess completed with ret: %d", ret);
    return ret;
}


int TmsDownload_Api(u8 *appCurrVer) //ck
{
    int ret;

    MAINLOG_L1("Starting TmsDownload_Api with version: %s", appCurrVer);

    t_sBuf = malloc(SENDPACKLEN);
    if (t_sBuf == NULL)
    {
        MAINLOG_L1("t_sBuf malloc failed!");
        Tms_LstDbgOutApp("Tms_CommProcess:", (unsigned char *)"t_sBuf malloc failed!", strlen("t_sBuf malloc failed!"), 1);
        return _TMS_E_MALLOC_NOTENOUGH;
    }
    MAINLOG_L1("t_sBuf malloc success");

    t_rBuf = malloc(RECVPACKLEN);
    if (t_rBuf == NULL)
    {
        MAINLOG_L1("t_rBuf malloc failed!");
        free(t_sBuf);
        Tms_LstDbgOutApp("Tms_CommProcess:", (unsigned char *)"t_rBuf malloc failed!", strlen("t_rBuf malloc failed!"), 1);
        return _TMS_E_MALLOC_NOTENOUGH;
    }
    MAINLOG_L1("t_rBuf malloc success");

    MAINLOG_L1("Calling Tms_SetTermParam()");
    Tms_SetTermParam(appCurrVer);  // Set terminal parameters according to device

    MAINLOG_L1("Calling Tms_CommProcess()");
    ret = Tms_CommProcess();
    MAINLOG_L1("Tms_CommProcess returned: %d", ret);

    Tms_LstDbgOutApp("Tms_CommProcess:", (u8 *)&ret, sizeof(ret), 0);

    MAINLOG_L1("Calling dev_disconnect()");
    TmsDisconnect();

    MAINLOG_L1("Freeing t_sBuf and t_rBuf");
    free(t_sBuf);
    free(t_rBuf);

    MAINLOG_L1("Returning from TmsDownload_Api with ret: %d", ret);
    return ret;
}


//0:all updated   -1:no config file   -2:download uncommplete  -3:font/param download over but not update       -4:application download over but not update   -5:LIB download over but not update
// 0 -1 :unnecessary to do anything           -2:download again       -3/-4/-5:call TmsUpdate_Api again
int TmsStatusCheck_Api(u8 *appCurrVer) //ck
{
	int i, len, ret;
	unsigned char tmp[64];

	struct __TmsTrade__ GFfile;

	len = sizeof(struct __TmsTrade__);
	ret = ReadFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, (unsigned int*)&len);
	if(ret != 0)
	{
		Tms_LstDbgOutApp("TmsStatusCheck_Api 1:", (u8 *)&ret, sizeof(ret), 0);
		return -1;
	}

	for(i = 0; i < GFfile.fnum; i++)
	{
		if(GFfile.file[i].status == STATUS_DLUNCOMPLETE)//downloading not finished
		{
			Tms_LstDbgOutApp("TmsStatusCheck_Api 2:", (u8 *)&ret, sizeof(ret), 0);
			return -2;
		}
	}

	for(i = 0; i < GFfile.fnum; i++)
	{
		if(GFfile.file[i].status == STATUS_DLCOMPLETE)//downloading finished but not replaced
		{
			if(strcmp(GFfile.file[i].type, TYPE_PARAM) == 0)
			{
				Tms_LstDbgOutApp("TmsStatusCheck_Api 3:", (unsigned char*)"font param download finished but not repaced", strlen("font param download finished but not repaced"), 1);
				return -3;
			}
			else if(strcmp(GFfile.file[i].type, TYPE_FIRMWARE) == 0)
			{
				memset(tmp, 0, sizeof(tmp));
				if(Tms_GetFirmwareVerSion((char *)tmp) == 0)
				{
					if(strcmp(GFfile.file[i].version, (char *)tmp) != 0)  //lib not latest
					{
						Tms_LstDbgOutApp("TmsStatusCheck_Api 51:", (unsigned char*)GFfile.file[i].version, strlen(GFfile.file[i].version), 1);
						Tms_LstDbgOutApp("TmsStatusCheck_Api 52:", tmp, strlen((char *)tmp), 1);
						return -5;
					}
				}
			}
			else if (strcmp(GFfile.file[i].type, TYPE_QR_STR) == 0)
			{
				char srcPath[256];
				/* downloaded into your standard dir (e.g. TMS_FILE_DIR) */
				sprintf(srcPath, "%s%s", TMS_FILE_DIR, GFfile.file[i].name);
				/* unzip into /ext/tms/ as requested */
				fileunZip_lib((unsigned char*)srcPath, "/ext/tms/");
				/* optionally delete the zip */
				DelFile_Api(srcPath);
			}
		}
	}

	for(i = 0; i < GFfile.fnum; i++)
	{
		if(GFfile.file[i].status == STATUS_DLCOMPLETE)//downloading finished but not replaced
		{
			if(strcmp(GFfile.file[i].type, TYPE_APP) == 0)
			{
				if(strcmp(GFfile.file[i].version, (char *)appCurrVer) != 0)  //APP not latest
				{
					Tms_LstDbgOutApp("TmsStatusCheck_Api 41:", (unsigned char*)GFfile.file[i].version, strlen(GFfile.file[i].version), 1);
					Tms_LstDbgOutApp("TmsStatusCheck_Api 42:", (unsigned char*)appCurrVer, strlen((char *)appCurrVer), 1);
					return -4;
				}
			}
		}
	}

	//all already updated
	DelFile_Api(_DOWN_STATUS_);
	return 0;
}


//0:success  -1:no config file   -2:download uncomplete  -3:update font/param files failed
int TmsUpdate_Api(u8 *appCurrVer)
{
	int i, len, ret;
	char tmp[64], buf[64], *p = NULL;

	struct __TmsTrade__ GFfile;

	len = sizeof(struct __TmsTrade__);
	ret = ReadFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, (unsigned int*)&len);
	if(ret != 0)
		return -1;

	for(i = 0; i < GFfile.fnum; i++)
	{
		if(GFfile.file[i].status == STATUS_DLUNCOMPLETE)//downloading not finished
			return -2;
	}

	//replace font and parameter files

	for(i = 0; i < GFfile.fnum; i++)
	{
		if(GFfile.file[i].status == STATUS_DLCOMPLETE) //downloading finished but not replaced
		{
			if(strcmp(GFfile.file[i].type, TYPE_PARAM) == 0)
			{
				memset(tmp, 0, sizeof(tmp));  //.bak name
				memset(buf, 0, sizeof(buf)); //old name
				sprintf(buf, "%s%s", TMS_FILE_DIR, GFfile.file[i].name);
				sprintf(tmp, "%s.bak", buf);
				DelFile_Api(buf);

				ret = ReNameFile_Api(tmp, buf);
				if(ret != 0)
					ret = ReNameFile_Api(tmp, buf);

				if(ret == 0)
				{
					char *p = strstr(GFfile.file[i].name, ".zip");
					if(p != NULL)
					{
						ret = fileunZip_lib((unsigned char*)buf, PARAM_FILE_DIR);
						// MAINLOG_L1("fileunZip_lib 11 :%d  file:%s  dest:%s", ret, buf, TMS_FILE_DIR);
						DelFile_Api(buf);
					}
				}

				if(ret == 0)
				{
					GFfile.file[i].status = STATUS_UPDATE;
					WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, sizeof(GFfile));
				}

				if(ret != 0)
				{
					Tms_LstDbgOutApp("TmsUpdate_Api rename:", (unsigned char *)tmp, strlen(tmp), 1);
					Tms_LstDbgOutApp("TmsUpdate_Api rename:", (unsigned char *)buf, strlen(buf), 1);
					Tms_LstDbgOutApp("TmsUpdate_Api rename ret:", (unsigned char *)&ret, sizeof(ret), 0);
					Tms_DelAllDownloadInfo(&GFfile);
					return -3;
				}
			}
		}
	}

	//update firmware
	for(i = 0; i < GFfile.fnum; i++)
	{
		if(GFfile.file[i].status == STATUS_DLCOMPLETE) //downloading finished but not replaced
		{
			if(strcmp(GFfile.file[i].type, TYPE_FIRMWARE) == 0)
			{
				ret = 0;
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "%s%s",  TMS_FILE_DIR, GFfile.file[i].name);
				p = strstr(GFfile.file[i].name, ".zip");
				if(p != NULL)
				{
					ret = fileunZip_lib((unsigned char*)tmp, TMS_FILE_DIR);
					// MAINLOG_L1("fileunZip_lib firmware :%d", ret);
					DelFile_Api(tmp);
					if(ret == 0)
					{
						memcpy(p, ".img", 4);  // bin change to .img
						WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, sizeof(GFfile));
						memset(tmp, 0, sizeof(tmp));
						sprintf(tmp, "%s%s",  TMS_FILE_DIR, GFfile.file[i].name);
						//// MAINLOG_L1("TagTms_lib name %s", tmp);
					}
				}

				if(ret == 0)
				{
					ret = tmsUpdateFile_lib(TMS_FLAG_DIFFOS, tmp, NULL);
					// MAINLOG_L1("TagTms_tmsUpdateFile_lib fw %s = %d", tmp, ret);
				}

				if(ret != 0)
				{
					Tms_DelAllDownloadInfo(&GFfile);
					return -4;
				}
			}
		}
	}


	//replace APP  //firmware must be updating before app because new lib may not work with old firmware
	for(i = 0; i < GFfile.fnum; i++)
	{
		if(GFfile.file[i].status == STATUS_DLCOMPLETE) //downloading finished but not replaced
		{
			if(strcmp(GFfile.file[i].type, TYPE_APP) == 0)
			{
				ret = 0;
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "%s%s",  TMS_FILE_DIR, GFfile.file[i].name);
				p = strstr(GFfile.file[i].name, ".zip");
				if(p != NULL)
				{
					ret = fileunZip_lib((unsigned char*)tmp, TMS_FILE_DIR);
					// MAINLOG_L1("fileunZip_lib app:%d", ret);

                    DelFile_Api(tmp);
					if(ret == 0)
					{
//#ifdef __Q181L__
#ifdef __Q161__
						memcpy(p, ".img", 4);
                        // MAINLOG_L1("Tms 1:%d", ret);
#else
						memcpy(p, ".img", 4); // bin to img
                        // MAINLOG_L1("Tms 2:%d", ret);

#endif
						WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&GFfile, 0, sizeof(GFfile));
						memset(tmp, 0, sizeof(tmp));
						sprintf(tmp, "%s%s",  TMS_FILE_DIR, GFfile.file[i].name);
                        // MAINLOG_L1("Tms 3:%d", ret);
					}
				}

				if(ret == 0)
				{
					// MAINLOG_L1("tmsUpdateFile_lib app name :%s", tmp);
					ret = tmsUpdateFile_lib(TMS_FLAG_APP, tmp, NULL);
					// MAINLOG_L1("TagTms_tmsUpdateFile_lib app = %d", ret);

				}
				else
				{
					Tms_DelAllDownloadInfo(&GFfile);
					return -5;
				}
			}
		}
	}

	DelFile_Api(_DOWN_STATUS_);
	return 0;
}


int Tms_DownloadUrlFilesOneByOne()
{
    char tmp[32], dname[64];
    int i, Ret, curfidx;
    int clen, stlen=0, thisLen,tlen = 0, len=0, mlen=0, sidx=0;
    int PackLen=0;
    char *p = NULL;

    while((TmsTrade.file[TmsTrade.curfindex].status == STATUS_DLUNCOMPLETE) && (TmsTrade.curfindex < TmsTrade.fnum))
    {
        curfidx = TmsTrade.curfindex;
        if(TmsTrade.file[TmsTrade.curfindex].startPosi > TmsTrade.file[TmsTrade.curfindex].fsize)
        {
            Tms_LstDbgOutApp("Dfile size wrong startPosi:", (u8 *)&TmsTrade.file[TmsTrade.curfindex].startPosi, sizeof(TmsTrade.file[TmsTrade.curfindex].startPosi), 0);
            Tms_LstDbgOutApp("Dfile size wrong fsize:", (u8 *)&TmsTrade.file[TmsTrade.curfindex].fsize, sizeof(TmsTrade.file[TmsTrade.curfindex].fsize), 0);
            Ret = _TMS_E_FILESIZE;
            break;
        }

        if(TmsTrade.file[TmsTrade.curfindex].startPosi == 0)
        {
            memset(&TmsTrade.context, 0, sizeof(TmsTrade.context));
            _tms_MD5Init (&TmsTrade.context);
            Tms_LstDbgOutApp("_tms_MD5Init:", NULL, 0, 1);
        }

        memset(dname, 0, sizeof(dname));
        Tms_getDomainName(TmsTrade.file[TmsTrade.curfindex].filePath, dname);

        for(i=TmsTrade.file[TmsTrade.curfindex].startPosi; i<TmsTrade.file[TmsTrade.curfindex].fsize; i+=EXFCONTENT_LEN)
        {
            memset(t_sBuf, 0, SENDPACKLEN);

            thisLen = (TmsTrade.file[TmsTrade.curfindex].fsize-TmsTrade.file[TmsTrade.curfindex].startPosi)>EXFCONTENT_LEN?EXFCONTENT_LEN:(TmsTrade.file[TmsTrade.curfindex].fsize-TmsTrade.file[TmsTrade.curfindex].startPosi);

            sprintf((char *)t_sBuf, "GET %s HTTP/1.1\r\n", TmsTrade.file[TmsTrade.curfindex].filePath);
            strcat((char *)t_sBuf, "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n");
            strcat((char *)t_sBuf, "User-Agent: Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)\r\n");
            sprintf((char *)t_sBuf+strlen((char *)t_sBuf), "Range: bytes=%d-%d\r\n", TmsTrade.file[TmsTrade.curfindex].startPosi, TmsTrade.file[TmsTrade.curfindex].startPosi+thisLen-1);
            sprintf((char *)t_sBuf+strlen((char *)t_sBuf), "Host: %s\r\n\r\n", dname);
            PackLen = strlen((char *)t_sBuf);
            Tms_LstDbgOutApp("request:", t_sBuf, PackLen, 1);

            // Use TmsSend instead of dev_send
            Ret = TmsSend(t_sBuf, PackLen);
            if(Ret != 0)
            {
                Tms_LstDbgOutApp("DUF CommTxd_Api:", (u8 *)&Ret, sizeof(Ret), 0);
                if(Ret != 0xff)
                {
                    Ret = _TMS_E_SEND_PACKET;
                    break;
                }

                Ret = Tms_ReSend(t_sBuf, PackLen);
                if(Ret != 0)
                {
                    Ret = _TMS_E_SEND_PACKET;
                    break;
                }
            }

            tlen = 0;
            memset(t_rBuf, 0, RECVPACKLEN);
            while(1)
            {
                Tms_StartJumpSec();
                // Use TmsReceive instead of dev_recv
                Ret = TmsReceive(t_rBuf+tlen, RECEIVE_BUF_SIZE - 1 - tlen, RECEIVE_TIMEOUT);
                Tms_StopJumpSec();
                if (Ret <= 0) {
                    Ret = _TMS_E_RECV_PACKET;
                    break;
                }

                len = Ret;
                Ret = 0;
                //Tms_LstDbgOutApp("receive:", t_rBuf+tlen, len, 0);
                tlen += len;
                if(Tms_getContentLen(t_rBuf, &clen, &stlen) != 0)
                    continue;

                if((p = strstr((char *)t_rBuf, "\n")) == NULL)  //\r\n
                    return _TMS_E_RESOLVE_PACKET;
                memset(tmp, 0, sizeof(tmp));
                memcpy(tmp, t_rBuf, ((unsigned char*)p)-t_rBuf);
                if((strstr(tmp, "200") == NULL) && (strstr(tmp, "206") == NULL))
                {
                    memset(TmsTrade.respMsg, 0, sizeof(TmsTrade.respMsg));
                    if(strlen(tmp) > (sizeof(TmsTrade.respMsg)-1))
                        memcpy(TmsTrade.respMsg, tmp, sizeof(TmsTrade.respMsg)-1);
                    else
                        memcpy(TmsTrade.respMsg, tmp, strlen(tmp));
                    return _TMS_E_TRANS_FAIL;
                }

                p = strstr((char *)t_rBuf, "\r\n\r\n");
                if(tlen > stlen)
                {
                    Ret = _TMS_E_RECV_PACKET;
                    break;
                }
                else if(tlen == stlen)
                {
                    if((tlen-(p+4-(char *)t_rBuf)) > 0)
                    {
#ifdef DISP_STH
                        ScrClrLine_Api(LINE3, LINE4);
                        memset(tmp, 0, sizeof(tmp));
                        sprintf(tmp, "%d%% %d/%d", (TmsTrade.file[TmsTrade.curfindex].startPosi*100)/TmsTrade.file[TmsTrade.curfindex].fsize, TmsTrade.file[TmsTrade.curfindex].startPosi, TmsTrade.file[TmsTrade.curfindex].fsize);
                        ScrDisp_Api(LINE3, 0, TmsTrade.file[TmsTrade.curfindex].name, FDISP|CDISP);
                        ScrDisp_Api(LINE4, 0, tmp,FDISP|CDISP);
#endif
                        Ret = Tms_WriteFile(&TmsTrade.file[TmsTrade.curfindex], (unsigned char*)(p+4), thisLen);
                        if(Ret != 0)
                        {
                            Ret = _TMS_E_FILE_WRITE;
                            Tms_LstDbgOutApp("Retwr2:", (u8 *)&Ret, sizeof(Ret), 0);
                            break;
                        }
                    }

                    if(Tms_NeedReConnect(t_rBuf) == 0) //reconnect
                    {
                        TmsDisconnect();
                        Ret = TmsConnect_Api();
                        Tms_LstDbgOutApp("TmsConnect_Api:", (u8 *)&Ret, sizeof(Ret), 0);
                        if(Ret != 0)
                            Ret = _TMS_E_ERR_CONNECT;
                    }
                    break;
                }
            }//while(1)
            if(Ret != 0)
            {
                Tms_LstDbgOutApp("in while:", (u8 *)&Ret, sizeof(Ret), 0);
                break;
            }

            if(TmsTrade.file[curfidx].startPosi > TmsTrade.file[curfidx].fsize)
            {
                Ret = _TMS_E_FILESIZE;
                Tms_LstDbgOutApp("recv>real recv:", (u8 *)&TmsTrade.file[curfidx].startPosi, sizeof(TmsTrade.file[curfidx].startPosi), 0);
                Tms_LstDbgOutApp("recv>real real:", (u8 *)&TmsTrade.file[curfidx].fsize, sizeof(TmsTrade.file[curfidx].fsize), 0);
                break;
            }
            if(TmsTrade.file[curfidx].status == STATUS_DLCOMPLETE)
            {
#ifdef DISP_STH
                ScrClrLine_Api(LINE3, LINE4);
                memset(tmp, 0, sizeof(tmp));
                sprintf(tmp, "100%% %d/%d", TmsTrade.file[curfidx].startPosi, TmsTrade.file[curfidx].fsize);
                ScrDisp_Api(LINE3, 0, TmsTrade.file[curfidx].name, FDISP|CDISP);
                ScrDisp_Api(LINE4, 0, tmp,FDISP|CDISP);
#endif
                break;
            }
        }//for - one file
        if(Ret != 0)
            break;
    }//while --all files
    if(Ret != 0)
        TmsDisconnect();
    return Ret;
}

//Tms update end

//Other functions start
int Tms_getDomainName(char *url, char *DomainName) //ck
{
	char *p = NULL,  *q = NULL;

	if((p = strstr(url, "//")) == NULL)
		return -1;
	if((q = strstr(p+2, "/")) == NULL)
		return -1;

	memcpy(DomainName, p+2, (q-p-2));
	return (q-p-2);
}

int Tms_Trim(u8 *str, u8 *out) //ck
{
	u8 *p = NULL;
	u8 *q = NULL;

	if((str == NULL) || (strlen((char *)str)<=0))
		return -1;

	p = str;
	q = str+strlen((char *)str)-1;
	while(p<=q)
	{
		if(*p != ' ')
			break;
		p++;
	}

	while(q>=p)
	{
		if(*q != ' ')
			break;
		q--;
	}
	memcpy(out, p, q-p+1);
	return 0;
}

int Tms_GetFirmwareVerSion(char *VerSion) //ck
{
	unsigned char ver[64];
	int ret;

	memset(ver, 0, sizeof(ver));
	ret = sysReadBPVersion_lib(ver);
	// MAINLOG_L1("TagTms_sysReadBPVersion == %d, version: %s", ret, ver);
	if(ret < 0)
		return -1;

	memcpy(VerSion, ver, strlen(ver));

	return 0;
}

//return :  0-update necessary      Others-download unnecessary
int Tms_CheckNeedDownFile(struct __FileStruct__ *file) //ck
{
	char	ver[64];

	memset(ver, 0, sizeof(ver));
	if(strcmp(file->type, TYPE_APP) == 0) //APP version
	{
		if(strcmp(file->version, TmsStruct.oldAppVer) > 0)
			return 0;
		else
			return 1;
	}
	else if(strcmp(file->type, TYPE_PARAM) == 0)
	{
		return 0;
	}
	else if(strcmp(file->type, TYPE_FIRMWARE) == 0)
	{
		if(Tms_GetFirmwareVerSion(ver) == 0)//version of lib with same name in terminal is found
		{
			//Tms_LstDbgOutApp("firmware version server:", file->version, strlen(file->version), 1);
			//Tms_LstDbgOutApp("firmware version local:", ver, strlen(ver), 1);
			if(strcmp(file->version, ver) > 0)
				return 0;
			else
			{
				Tms_DbgOutApp("newver:", (unsigned char*)file->version, strlen(file->version));
				Tms_DbgOutApp("oldver:", (unsigned char*)ver, strlen(ver));
				return 1;
			}
		}
		else
		{
			Tms_DbgOutApp("no ver:", (unsigned char*)"not fond lib", strlen("not fond lib"));
			return 0;
		}
	}
	return 1;
}

int Tms_GetFileSize(struct __FileStruct__ *file) //ck
{
	char tmp[64];

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", TMS_FILE_DIR, file->name);
	if((memcmp(file->type, TYPE_APP, 3) == 0) || (memcmp(file->type, TYPE_FIRMWARE, 2) == 0))
		return GetFileSize_Api(tmp);
	else if(memcmp(file->type, TYPE_PARAM, 2) == 0)
	{
		strcat(tmp, ".bak");
		return GetFileSize_Api(tmp);
	}
	return 0;
}

int Tms_WriteFile(struct __FileStruct__ *file, unsigned char *Buf,unsigned int Length) //ck
{
	int ret = -1, flen=0;
	char tmp[128];

	//if((strcmp(TYPE_FIRMWARE, file->type) == 0) || (strcmp(TYPE_APP, file->type) == 0) || (strcmp(TYPE_PARAM, file->type) == 0))

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", TMS_FILE_DIR, file->name);
	if(strcmp(TYPE_PARAM, file->type) == 0)
		strcat(tmp, ".bak");

	flen = GetFileSize_Api(tmp);
	ret = WriteFile_Api(tmp, Buf, file->startPosi, Length);
	if(ret != 0)
		return _TMS_E_FILE_WRITE;

	file->startPosi += Length;
	_tms_MD5Update(&TmsTrade.context, (unsigned char*)Buf, Length);

	//Tms_LstDbgOutApp("state:", TmsTrade.context.state, sizeof(TmsTrade.context.state), 0);
	//Tms_LstDbgOutApp("count:", TmsTrade.context.count, sizeof(TmsTrade.context.count), 0);
	//Tms_LstDbgOutApp("buffer:", TmsTrade.context.buffer, sizeof(TmsTrade.context.buffer), 0);
	if(file->startPosi == file->fsize)
	{
		//check md5
		memset(tmp, 0, sizeof(tmp));
		_tms_MD5Final((unsigned char*)tmp, &TmsTrade.context);
		if(memcmp(file->md5, tmp, 16) != 0)
		{
			Tms_LstDbgOutApp("cal md5:", (unsigned char *)tmp, 16, 0);
			Tms_LstDbgOutApp("host md5:", (unsigned char *)file->md5, 16, 0);
			file->startPosi = 0;
			Tms_DelFile(file);
			WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
			return _TMS_E_MD5_ERR;
		}
		else
		{
			file->status = STATUS_DLCOMPLETE;
			TmsTrade.curfindex++;
		}
	}
	else if(file->startPosi > file->fsize)
	{
		file->startPosi = 0;
		Tms_DelFile(file);
		WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
		return _TMS_E_FILESIZE;
	}

	ret = WriteFile_Api(_DOWN_STATUS_, (unsigned char *)&TmsTrade, 0, sizeof(TmsTrade));
	if(ret != 0)
		return _TMS_E_FILE_WRITE;

	if(Gfunflag == 1)
	{
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "fsizebeforewrite:%d writestart:%d writelen:%d", flen, file->startPosi, Length);
		Tms_LstDbgOutApp("writelen:", (unsigned char *)tmp, strlen(tmp), 1);
	}
	return 0;
}


void Tms_DelFile(struct __FileStruct__ *file) //ck //ck
{
	char tmp[64];

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", TMS_FILE_DIR, file->name);

	if(strcmp(TYPE_PARAM, file->type) == 0)
		strcat(tmp, ".bak");

	DelFile_Api(tmp);
	//Tms_LstDbgOutApp("DelFile_Api:", (u8 *)&ret, sizeof(ret), 0);

	//if((strcmp(TYPE_FIRMWARE, file->type) == 0) || (strcmp(TYPE_APP, file->type) == 0)) //application
}

//return:  0-zip file   1-non zip file
int tms_filezipcheck(char *filename)
{
	if(strstr(filename, ".zip") != NULL)
		return 0;
	return 1;
}

int TmsDelFile_Api(int type) //CK
{
	int i, len, ret;
	char tmp[64];
	struct __TmsTrade__ GFfile;

	len = sizeof(struct __TmsTrade__);
	ret = ReadFile_Api(_DOWN_STATUS_, (unsigned char*)&GFfile, 0, (unsigned int *)&len);
	if(ret != 0)
	{
		Tms_LstDbgOutApp("TmsDelFile_Api 1:", (u8 *)&ret, sizeof(ret), 0);
		return 0;
	}

	for(i = 0; i < GFfile.fnum; i++)
	{
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s%s", TMS_FILE_DIR, GFfile.file[i].name);
		switch(type)
		{
		case 1:
			if(strcmp(TYPE_APP, GFfile.file[i].type) == 0) //application
			{
				if(tms_filezipcheck(tmp) == 0)
				{
					fileFilter=2; //delete all .img file
					folderFileDisplay((unsigned char *)TMS_FILE_DIR);
				}
				ret = DelFile_Api(tmp);
				Tms_LstDbgOutApp("TmsDelFile_Api app:", (u8 *)&ret, sizeof(ret), 0);
			}
			break;
		case 2:
			if((strcmp(TYPE_FIRMWARE, GFfile.file[i].type) == 0))
			{
				if(tms_filezipcheck(tmp) == 0)
				{
					fileFilter=5; //delete all .bin file
					folderFileDisplay((unsigned char *)TMS_FILE_DIR);
				}
				ret = DelFile_Api(tmp);
				Tms_LstDbgOutApp("TmsDelFile_Api fw:", (u8 *)&ret, sizeof(ret), 0);
			}
			break;
		case 3: //font
			break;
		case 4:
			if(strcmp(TYPE_PARAM, GFfile.file[i].type) == 0) //param files
			{
				strcat(tmp, ".bak");
				ret = DelFile_Api(tmp);
				Tms_LstDbgOutApp("TmsDelFile_Api2:", (u8 *)&ret, sizeof(ret), 0);
			}
			break;
		default:
			return -1;
		}
	}
	return ret;
}


//check if all files are download completely
//return: 0-all download over    others-not over
int Tms_CheckDownloadOver() //CK
{
	int i;

	for(i=0; i<TmsTrade.fnum; i++)
	{
		if(TmsTrade.file[i].status != STATUS_DLCOMPLETE)
			return -1;
	}
	return 0;
}

#define ERR_NO_CONTENTLEN  -1
#define ERR_CONLEN_FORMAT_WRONG  -2
//Clen:content length      Tlen:total length
int Tms_getContentLen(u8 *packdata, int *Clen, int *Tlen) //ck
{
	u8 *p = NULL;
	char *q = NULL,  *s = NULL, *e = NULL;
	u8 tmp[16], buf[16];

	p = packdata;
	if((q = strstr((char *)packdata, (char *)"\r\n\r\n")) == NULL)
	{
		return ERR_NO_CONTENTLEN;
	}

	if((s = strstr((char *)p, (char *)"Content-Length:")) == NULL)
	{
		return ERR_NO_CONTENTLEN;
	}

	s += strlen("Content-Length:");
	if((e = strstr(s, "\r\n")) == NULL)
	{
		return ERR_CONLEN_FORMAT_WRONG;
	}

	if(e > q)
	{
		return ERR_CONLEN_FORMAT_WRONG;
	}

	memset(buf, 0, sizeof(buf));
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp,	s, e-s);
	if(Tms_Trim(tmp, buf) != 0)
		*Clen = 0;
	else
		*Clen = (int)atoi((char *)buf);

	*Tlen = (q-(char *)packdata+4+(*Clen));
	return 0;
}

//reconnect or not  ,  0:reconnect   others:unnecessary
int Tms_NeedReConnect(u8 *packdata) //ck
{
	u8 *p = NULL,  *q = NULL, *s = NULL, *e = NULL;
	u8 tmp[16], buf[16];

	p = packdata;
	if((q = (u8 *)strstr((char *)packdata, "\r\n\r\n")) == NULL)
		return -1;

	if((s = (u8 *)strstr((char *)p, "Connection:")) == NULL)
		return -1;

	s += strlen("Connection:");
	if((e = (u8 *)strstr((char *)s, "\r\n")) == NULL)
		return -1;

	if(e > q)
		return -1;

	memset(buf, 0, sizeof(buf));
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp,	s, e-s);
	if(Tms_Trim(tmp, buf) != 0)
		return -1;

	if(strcmp((char *)buf, "close") == 0)
		return 0;

	return -1;
}

//return:  0-success    Others-failed
int Tms_ReConnect()
{
    int i, ret;

    for(i=0; i<2; i++)
    {
        TmsDisconnect();
        ret = TmsConnect_Api();
        if(ret == 0)
            break;
    }
    return ret;
}
////only for case 0xff
//return : 0-success   _TMS_E_SEND_PACKET--reconnect or send failed
int Tms_ReSend(u8 *packData, int PackLen) //ck
{
	int ret;
#ifdef DISP_STH
	ScrDisp_Api(LINE2, 0, "ReConnecting..", FDISP|CDISP);
#endif
	ret = Tms_ReConnect();
#ifdef DISP_STH
	ScrClrLine_Api(LINE2, LINE2);
#endif
	if(ret != 0)
		return _TMS_E_SEND_PACKET;

	ret = TmsSend((unsigned char *)packData,  PackLen);
	if(ret != 0)
	{
		Tms_LstDbgOutApp("Tms_ReSend CommTxd_Api:", (u8 *)&ret, sizeof(ret), 0);
		return _TMS_E_SEND_PACKET;
	}
	return ret;
}

//Other functions end

//OnorOff: output or not : 0-off; 1-on;
//logDest: destination that log output :1-log written to file "dbgapp.txt"      2-log output to port
//port : port index
void OutputLogSwitch(int OnorOff, int logDest) //ck
{
	if(OnorOff)
		Gfunflag = 1;
	else
		Gfunflag = 0;
	_tms_G_dbgOutDestApp = logDest;
}

#define LOGSPCS_BYTES 450  //MAINLOG_L1 support 1k at most
#define LOGPRINT_MALLOC 1024
//output logs start
//type 0-BCD  1-ASC 2-%X 3-%d
void Tms_LstDbgOutApp(const char *title, unsigned char *pData, int dLen, u8 type) //ck
{
	if(Gfunflag == 1)
	{
		int i, plen, tlen = dLen, titlen;
		char *buf = malloc(LOGPRINT_MALLOC);

		if(buf == NULL)
		{
			// MAINLOG_L1("TagTms_Tms_LstDbgOutApp malloc failed");
			return ;
		}

		memset(buf, 0, LOGPRINT_MALLOC);
		if((_tms_G_dbgOutDestApp & 0xff) == 2)
		{
			if(title != NULL)
				sprintf(buf+strlen(buf), "%s:", title);
			else
				sprintf(buf+strlen(buf), "%s", ":");

			if((pData == NULL) || (dLen <= 0))
			{
				// MAINLOG_L1("TagTms_%s", buf);
				free(buf) ;
				return ;
			}

			if(type==0)
			{
				titlen = strlen(buf);
				for(i = 0; i < (dLen+LOGSPCS_BYTES-1)/LOGSPCS_BYTES; i++)
				{
					memset(buf+titlen, 0, LOGPRINT_MALLOC-titlen);
					plen = tlen * 2 > LOGSPCS_BYTES*2 ? LOGSPCS_BYTES*2 : tlen * 2;
					BcdToAsc_Api(buf+titlen, pData+i*LOGSPCS_BYTES, plen);
					// MAINLOG_L1("TagTms_%s", buf);
					tlen -= plen/2;
					if(tlen <= 0)
						break;
				}
				free(buf) ;
				return ;
			}
			else if(type==1)
			{
				if(dLen > LOGSPCS_BYTES*2)
				{
					// MAINLOG_L1("TagTms_%s", "print 900 bytes in front");
					memcpy(buf+strlen(buf), pData, LOGSPCS_BYTES*2);
				}
				else
					memcpy(buf+strlen(buf), pData, dLen);
			}
			else if(type==2)
			{
				sprintf(buf+strlen(buf), "0x%x", *pData);
			}
			else if(type == 3)
			{
				if(dLen == 1)
				{
					char aa;
					memcpy(&aa, pData, dLen);
					sprintf(buf+strlen(buf), "%d", aa);
				}
				else if(dLen == 2)
				{
					unsigned short aa;
					memcpy(&aa, pData, dLen);
					sprintf(buf+strlen(buf), "%d", aa);
				}
				else if(dLen == 4)
				{
					int aa;
					memcpy(&aa, pData, dLen);
					sprintf(buf+strlen(buf), "%d", aa);
				}
			}
			// MAINLOG_L1("TagTms_%s", buf);
		}
		free(buf) ;
	}
}

void Tms_DbgOutApp(const char *title, unsigned char *pData, int dLen) //ck
{
	if(Gfunflag == 1)
	{
		char *buf = malloc(LOGPRINT_MALLOC);
		if(buf == NULL)
		{
			// MAINLOG_L1("TagTms_Tms_DbgOutApp malloc failed");
			return ;
		}
		memset(buf, 0, LOGPRINT_MALLOC);
		if((_tms_G_dbgOutDestApp & 0xff) == 2)
		{
			if(title != NULL)
				sprintf(buf+strlen(buf), "%s dlen-%d :", title, dLen);
			else
				sprintf(buf+strlen(buf), " dlen-%d :", dLen);

			if((pData != NULL) && (dLen > 0))
			{
				if(dLen > LOGSPCS_BYTES*2)
					memcpy(buf+strlen(buf), pData, LOGSPCS_BYTES*2);
				else
					memcpy(buf+strlen(buf), pData, dLen);

			}
			// MAINLOG_L1("TagTms_%s", buf);
		}
		free(buf);
	}
}

//int TmsTest();

///tms part

void tms_TMSThread(void)
{
	int ret;

	while (1) {
		while (tms_tms_flag == 0)
		{
			Delay_Api(1000);
		}

		// MAINLOG_L1("TagTms_start tms download");
//        DelFile_Api("/ext/tms/");
		ret = TmsTest();
		// MAINLOG_L1("TagTms_ret123456789 %d", ret);
        // MAINLOG_L1("TagTms_ret111111111 %d", ret);
//        CustomizedKHQR();
        // MAINLOG_L1("TagTms_ret222222222 %d", ret);
        tms_tms_flag = 0;

	}
}

int fileMkdir_lib(char *pchDirName);

void TMSConnection(void)
{
    int ret;
    char tmp[32];

    memset(tmp, 0, sizeof(tmp));
    sprintf(tmp, "%s%s", TMS_FILE_DIR, "1e");

    ret = GetFileSize_Api(tmp);
    if(ret <= 0)
    {
        ret = WriteFile_Api(tmp, (unsigned char*)"exist", 0, 5);
        if(ret != 0)
            return;
        ret = GetFileSize_Api(tmp);
        if(ret <= 0)
        {
            AppPlayTip("Creating folder failed");
            return;
        }
    }

    // Make sure configuration is downloaded and loaded before proceeding
    download_config_file();

    // Trim whitespace from port value to ensure clean comparison
    char trimmedPort[8] = {0};
    strncpy(trimmedPort, _TMS_HOST_PORT_, sizeof(trimmedPort) - 1);

    // Remove leading and trailing whitespace
    int start = 0, end = strlen(trimmedPort) - 1;
    while(trimmedPort[start] == ' ' && start < end) start++;
    while(trimmedPort[end] == ' ' && end > start) end--;
    trimmedPort[end + 1] = '\0';

    // Log current TMS settings with trimmed port
    MAINLOG_L1("TMS Settings - Domain: %s, Port: '%s', IP: %s",
               _TMS_HOST_DOMAIN_, trimmedPort, _TMS_HOST_IP_);

    memset(&tmsEntry, 0, sizeof(tmsEntry));

    // Check multiple common HTTPS port formats
    if (strcmp(trimmedPort, "443") == 0 ||
        strcmp(_TMS_HOST_PORT_, "443") == 0 ||
        atoi(_TMS_HOST_PORT_) == 443) {
        tmsEntry.protocol = PROTOCOL_HTTPS;
        MAINLOG_L1("Using HTTPS protocol for TMS connection (port 443)");
    } else {
        // Default to HTTP for port 80 or any other port
        tmsEntry.protocol = PROTOCOL_HTTP;
        MAINLOG_L1("Using HTTP protocol for TMS connection (port %s)", _TMS_HOST_PORT_);
    }

    // Copy domain and port values
    strcpy(tmsEntry.domain, _TMS_HOST_DOMAIN_);
    strcpy(tmsEntry.port, trimmedPort);  // Use trimmed port

    ret = TmsParamSet_Api(_TMS_HOST_IP_, trimmedPort, _TMS_HOST_DOMAIN_);
    if(ret != 0)
    {
        AppPlayTip("Set parameters failed");
        return;
    }

    AppPlayTip("Connecting to the server");
    ret = TmsConnect_Api();
    if (ret != 0){
        AppPlayTip("Connect to the server failed");
        TmsDisconnect();
        return;
    }

    ret = TmsDownload_Api((u8 *)App_Msg.Version);
    if(ret == _TMS_E_NOFILES_D)
    {
        AppPlayTip("Already been the latest version");
        TmsDisconnect();
        return;
    }
    else if(ret != 0)
    {
        AppPlayTip("Download failed");
        TmsDisconnect();
        return;
    }

    AppPlayTip("Updating");
    ret = TmsUpdate_Api((u8 *)App_Msg.Version);
    if(ret != 0)
        AppPlayTip("Update failed");

    TmsDisconnect();
    return;
}

//check if update successfully
void CheckTmsStatus()
{
	int ret = 0;

	ret = TmsStatusCheck_Api((u8 *)App_Msg.Version);
	if(ret == 0 || ret == -1){ //update successfully  or no update information
//		// MAINLOG_L1("/tms_download/Min_sound.mp3 size:%d", GetFileSize_Api("/ext/tms/Min_sound.mp3"));
//		// MAINLOG_L1("/tms_download/Max_sound.mp3 size:%d", GetFileSize_Api("/ext/tms/Max_sound.mp3"));
//		// MAINLOG_L1("/tms_download/Decrease.mp3 size:%d", GetFileSize_Api("/ext/tms/Decrease.mp3"));
//		// MAINLOG_L1("/tms_download/Increase.mp3 size:%d", GetFileSize_Api("/ext/tms/Increase.mp3"));
//		// MAINLOG_L1("/tms_download/Insert sim.mp3 size:%d", GetFileSize_Api("/ext/tms/Insert sim.mp3"));
//		// MAINLOG_L1("/tms_download/Internet_connected.mp3 size:%d", GetFileSize_Api("/ext/tms/Internet_connected.mp3"));
//		// MAINLOG_L1("/tms_download/Internet_Disconnected.mp3 size:%d", GetFileSize_Api("/ext/tms/Internet_Disconnected.mp3"));
//		// MAINLOG_L1("/tms_download/IoT_Server_down.mp3 size:%d", GetFileSize_Api("/ext/tms/IoT_Server_down.mp3"));
//		// MAINLOG_L1("/tms_download/IoT_Sucess_connect.mp3 size:%d", GetFileSize_Api("/ext/tms/IoT_Sucess_connect.mp3"));
//		// MAINLOG_L1("/tms_download/Success_01.mp3 size:%d", GetFileSize_Api("/ext/tms/Success_01.mp3"));
//		// MAINLOG_L1("/tms_download/00060006106_KHR.jpg size:%d", GetFileSize_Api("/ext/tms/00060006106_KHR.jpg"));
//		// MAINLOG_L1("/tms_download/00060006106_KHR_adr.jpg size:%d", GetFileSize_Api("/ext/tms/00060006106_KHR_adr.jpg"));
//		// MAINLOG_L1("/tms_download/Q161TMS_VanstoneSign.img size:%d", GetFileSize_Api("/ext/tms/Q161TMS_VanstoneSign.img"));
		return ;
	}else if(ret == -2)
	{
		AppPlayTip("Need to connect and download again");
	}
	else if(ret == -3 || ret == -4 || ret == -5)
	{
		AppPlayTip("Update locally now");
		TmsUpdate_Api((u8 *)App_Msg.Version);
	}
}

/*
void checkUpdateResult()
{
	char tmp[128];
	int ret;

	// MAINLOG_L1("App Current version:%s", App_Msg.Version);

	memset(tmp, 0, sizeof(tmp));
	ret = sysReadBPVersion_lib(tmp);
	// MAINLOG_L1("sysReadBPVersion_lib == %d, version: %s", ret, tmp);

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "mp3.zip");
	// MAINLOG_L1("/tms/mp3.zip size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/mp3/", "param1.txt");
	// MAINLOG_L1("/tms/mp3/param1.txt size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/mp3/", "param2.txt");
	// MAINLOG_L1("/tms/mp3/param2.txt size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "param.zip");
	// MAINLOG_L1("/tms/param.zip size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "param.txt");
	// MAINLOG_L1("/tms/param.txt size:%d", GetFileSize_Api(tmp));


	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "Q181L_0114.zip");
	// MAINLOG_L1("/tms/Q181L_0114.zip size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "Q181L_0114.img");
	// MAINLOG_L1("/tms/Q181L_0114.img size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "Q181E_0113.zip");
	// MAINLOG_L1("/tms/Q181E_0113.zip size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "Q181E_0113.bin");
	// MAINLOG_L1("/tms/Q181E_0113.bin size:%d", GetFileSize_Api(tmp));

	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%s", "/tms/", "mp3Client.zip");
	// MAINLOG_L1("/tms/mp3Client.zip size:%d", GetFileSize_Api(tmp));

}


void DelAllUpdateFiles()
{

	DelFile_Api("/tms/mp3.zip");
	DelFile_Api("/tms/mp3/param1.txt");
	DelFile_Api("/tms/mp3/param2.txt");

	DelFile_Api("/tms/param.zip");
	DelFile_Api("/tms/param.txt");

	DelFile_Api("/tms/Q181L_0114.zip");
	DelFile_Api("/tms/Q181L_0114.bin");

	DelFile_Api("/tms/Q181E_0113.zip");
	DelFile_Api("/tms/Q181E_0113.img");

	DelFile_Api("/tms/mp3Client.zip");

	DelFile_Api(_DOWN_STATUS_);
}*/

void delete(){
	DelFile_Api("/ext/tms/Min_sound.mp3");
	DelFile_Api("/ext/tms/Max_sound.mp3");
	DelFile_Api("/ext/tms/Decrease.mp3");
	DelFile_Api("/ext/tms/Increase.mp3");
	DelFile_Api("/ext/tms/Insert sim.mp3");
	DelFile_Api("/ext/tms/Internet_connected.mp3");
	DelFile_Api("/ext/tms/Internet_Disconnected.mp3");
	DelFile_Api("/ext/tms/IoT_Server_down.mp3");
	DelFile_Api("/ext/tms/IoT_Sucess_connect.mp3");
	DelFile_Api("/ext/tms/Success_01.mp3");
	DelFile_Api("/ext/tms/00060006106_KHR.jpg");
	DelFile_Api("/ext/tms/00060006106_KHR_adr.jpg");
	DelFile_Api("/ext/tms/Q161TMS_VanstoneSign.img");
}

