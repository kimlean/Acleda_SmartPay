#ifndef _tms_HX_TMS_H_
#define _tms_HX_TMS_H_

#include <coredef.h>
#include <string.h>
#include <struct.h>
#include <cJSON.h>
#include "def.h"
#include "tms_md5.h"

#define STATUS_DLCOMPLETE   1  // download complete
#define STATUS_UPDATE       2  // already updated

//#define __Q181L__  		//defined in "def.h"
#define TMSFILE_EXTPATH   //save tms file into external path or internalpath  open-external  close-internal
//#define DISP_STH  // display something //for terminals without display, close it
//#define JUMPER_EXIST

#ifdef TMSFILE_EXTPATH
#define TMS_FILE_DIR  "/ext/tms/"
#define _DOWN_STATUS_ "/ext/tms/_tms_dinfo_"
#define PARAM_FILE_DIR  "/ext/"
#else
#define TMS_FILE_DIR  "/tms/"
#define _DOWN_STATUS_ "/tms/_tms_dinfo_"
#endif

//extern char _TMS_HOST_IP_[64];
//extern char _TMS_HOST_DOMAIN_[128];
//extern char _TMS_HOST_PORT_[64];

//#define _TMS_HOST_IP_		"103.235.231.19"
//#define _TMS_HOST_DOMAIN_	"ipos-os.jiewen.com.cn"
//#define _TMS_HOST_PORT_		"80"

#define SENDPACKLEN 1024
#define RECVPACKLEN 1024*7
#define EXFCONTENT_LEN 6000

#define	PROTOCOL_HTTP	0
#define	PROTOCOL_HTTPS	1

#define TYPE_APP	"APP" //application
#define TYPE_LIB	"DD" //dynamic library --so
#define TYPE_FONT	"WS" //font 
#define TYPE_PARAM  "PF" //common file/parameters file   //CK-�������
#define TYPE_FIRMWARE  "UF" //  UF-firmware 
#define TYPE_QR_STR    "QR"

#define MACHINE_Q181L	"Q181L"
#define MACHINE_Q181E	"Q181E"
#define MACHINE_Q161	"Q161"

#define _TMS_MAX_APPLIB		25

/*****************ERROR CODE****************************/
#define  _TMS_E_TRANS_FAIL			2		//not 200 in response package  or Response code not "00" , or not "200"/"206" while download files
#define  _TMS_E_ERR_CONNECT			5		//connect server failed //unuseful
#define  _TMS_E_SEND_PACKET			6		//send package failed
#define  _TMS_E_RECV_PACKET			7		//receive response failed
#define  _TMS_E_RESOLVE_PACKET		8		//resolve package failed
#define  _TMS_E_PREADDFILE			9		//pre-add files failed
#define  _TMS_E_TOO_MANY_FILES		10		//support less than 20 files
#define  _TMS_E_PACKAGE_WRONG		11		//package received not for this terminal
#define  _TMS_E_FILE_WRITE			13		//write file failed
#define  _TMS_E_MD5_ERR				14		//MD5 error
#define  _TMS_E_DOWNUNCOMPLETE		15		//files download uncomplete
#define  _TMS_E_FILE_SEEK			16		//file seek error
#define	 _TMS_E_FILESIZE			25		//file size error
#define  _TMS_E_NOFILES_D			28    //no files necessary to download for this terminal , or host app version is not higher than app in terminal
#define  _TMS_E_MALLOC_NOTENOUGH	29

#define STATUS_DLUNCOMPLETE		0		//dowload uncomplete
#define STATUS_DLCOMPLETE		1		//download complete
#define STATUS_UPDATE			2		//already update

#define _TMS_FILE_LINUX_		1
#define _TMS_FILE_VOS_			2
#define _TMS_FILE_OTHRE_		0xff


enum TRADE_TYPE
{
	TYPE_CHECKVERSION = 1,
	TYPE_URLGETFILE, 
	TYPE_NOTIFY,
	TYPE_UPDATE,
	TYPE_QR
}; 

enum TRADEHOST_MODE
{
	TMSFLOWWITHONEHOST = 0,
	TMSFLOWWITHTWOHOST
};


struct __FileStruct__
{
	char	name[32];
	char	type[5];
	char	version[26];
	char	filePath[100];
	u8		md5[16];
	int 	fsize;
	int 	startPosi;
	int 	status;  //0-not download completely      1-download completely    2-already update
};

struct __TmsTrade__
{
	int		trade_type;  //trade type
	char	respCode[16];
	char	respMsg[32];
	int		fnum; //the number of file
	int 	curfindex;  //index of current file downloading
	struct __FileStruct__  file[_TMS_MAX_APPLIB]; 
	_tms_MD5_CTX context; 
	u8  	md5data[64];
	int 	mdlen;
	char	deviceType[8];  //M
};

struct __TmsStruct__   //Open for Clients  //M-mandatory   O-Optional
{
	char	version[32];  //   M
	char	sn[18]; //SN   M
	char	manufacturer[15]; //M
	char	deviceType[8];  //M
	char	merNo[32];  //merchent number  M
	char	termNo[32];  //terminal number M
	char	hostIP[16];  //O      one of hostIP and hostDomainName is mandatory
	char	hostPort[8];   //M
	char	hostDomainName[64];  // M    mandatory for http (check version and notify) message
	char    oldAppVer[26];  //old app version
	int		tradeTimeoutValue;
};

#define	MAX_DOMAIN_LENGTH	128
#define	MAX_PATH_LENGTH		128

extern int Tms_logflag;
URL_ENTRY tmsEntry;
struct __TmsTrade__  TmsTrade;
struct __TmsStruct__  TmsStruct;
cJSON * pRoot;
int _tms_G_dbgOutDestApp;

void Tms_CommHangUp();
int Tms_StartJumpSec(void);
void Tms_StopJumpSec(void);						

int Tms_CreatePacket(u8 * packData, int * packLen);
int Tms_RecvPacket(u8 *Packet, int *PacketLen, int WaitTime);
int Tms_UrlRecvPacket(u8 *Packet, int *PacketLen, int WaitTime);
int Tms_SendRecvData( unsigned char *SendBuf, int Senlen, unsigned char *RecvBuf, int *RecvLen,int psWaitTime);
int Tms_SendRecvPacket(u8 *SendBuf, int Senlen, u8 *RecvBuf, int *pRecvLen);

int TmsParamSet_Api(char *hostIp, char *hostPort, char *hostDomainName);
int TmsDownload_Api(u8 *appCurrVer);
int TmsUpdate_Api(u8 *appCurrVer);
int TmsStatusCheck_Api(u8 *appCurrVer);

int Tms_getDomainName(char *url, char *DomainName);
int Tms_getContentLen(u8 *packdata, int *Clen, int *Tlen);
int Tms_CheckNeedDownFile(struct __FileStruct__ *file);
void Tms_DelAllDownloadInfo(struct __TmsTrade__ *GFfile);
int Tms_GetFileSize(struct __FileStruct__ *file);
int Tms_WriteFile(struct __FileStruct__ *file, unsigned char *Buf,unsigned int Length);
void Tms_DelFile(struct __FileStruct__ *file);
int Tms_CheckDownloadOver();
int Tms_NeedReConnect(u8 *packdata);
int Tms_CompareVersions(struct __TmsTrade__ *Curr);
int Tms_DownloadUrlFilesOneByOne();
int TmsDelFile_Api(int type);
int Tms_GetFirmwareVerSion(char *VerSion);
int Tms_ReSend(u8 *packData, int PackLen);
int Tms_UnPackPacket(char * data, int reslt);

void Tms_LstDbgOutApp(const char *title, unsigned char *pData, int dLen, u8 type);
void Tms_DbgOutApp(const char *title, unsigned char *pData, int dLen);
void OutputLogSwitch(int OnorOff, int logDest);

void checkUpdateResult();
void DelAllUpdateFiles();

int sysReadBPVersion_lib(unsigned char *BPinfo);
//int sysGetTermType_lib (char *out_type);

#endif

