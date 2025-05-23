#ifndef __DEF_H__
#define	__DEF_H__

#define EXTERN extern
#define MQTT_PACKET_LEN 640
#define	MAX_PATH_LENGTH		128
#define BATER_WARM	   15
#define BATER_POWR_OFF 3

extern char _TMS_HOST_IP_[64];
extern char _TMS_HOST_DOMAIN_[128];
extern char _TMS_HOST_PORT_[64];

EXTERN int fileFilter;
EXTERN int needUpdate;
EXTERN int Device_Type;
EXTERN unsigned char updateAppName[32];
extern char global_dB[10];
extern unsigned char tms_ip[32];
extern unsigned char tms_domain[32];

extern char KHQRBMP[50];
extern char USDQRBMP[50];
extern char DUEQRBMP[50];
extern int DISPLAY_QR_TYPE;
extern int g_khrQrExists;
extern int g_usdQrExists;

#define __DISPLAY__   //terminal with display or not ,   open:yes      close:no

enum tms_flag {
 TMS_FLAG_BOOT = 0,
 TMS_FLAG_VOS,
 TMS_FLAG_L1,
 TMS_FLAG_L2,
 TMS_FLAG_FONT,
 TMS_FLAG_MAPP,
 TMS_FLAG_ACQUIRER,
 TMS_FLAG_APP_PK,
 TMS_FLAG_APP,
 TMS_FLAG_VOS_NOFILE,
 TMS_FLAG_APP_NOFILE,
 TMS_FLAG_DIFFOS,
 TMS_FLAG_MAX,
};

typedef struct _LCDINFO{
 int Wigtgh;            //actual width
 int Heigth;            //actual height
 int UseWigtgh;           //available width
 int UseHeigth;           //available width
 unsigned char ColorValue;        // 1:monochromatic 2:RGB332 3:RGB565 4:RGB888
}LCDINFO;

typedef struct {
	char FromBank[7];
	unsigned char EncodeMobile[256];
}NFCTAPINFO;
extern NFCTAPINFO NFC_INFO;

// Define maximum domain length if not already defined
#ifndef MAX_DOMAIN_LENGTH
#define MAX_DOMAIN_LENGTH 128
#endif

// Enhanced socket structure with reference counting
typedef struct {
    int valid;              // 1 if slot is in use, 0 if free
    int socket;             // Socket handle
    int ssl;                // SSL mode (0=none, 1=SSL, 2=SSL with cert)
    int ref_count;          // Reference counter to track usage
    char host[MAX_DOMAIN_LENGTH]; // Host to track connection target
    char port[8];           // Port to track connection target
} NET_SOCKET_PRI;
//
// Maximum number of socket slots
#define MAX_SOCKETS 10
#define CERTI_LEN  1024*7

typedef struct {
	int protocol;
	char domain[MAX_DOMAIN_LENGTH];
	char path[MAX_PATH_LENGTH];
	char port[8];
} URL_ENTRY;

extern int parseURL(char *url, URL_ENTRY *entry);
// Global socket slots for connection reuse
extern NET_SOCKET_PRI sockets[MAX_SOCKETS];

// Global named socket references for common services
extern NET_SOCKET_PRI *g_tms_socket;
extern NET_SOCKET_PRI *g_mqtt_socket;
extern NET_SOCKET_PRI *g_api_socket;

/* Missed system header definitions */
int tmsUpdateFile_lib(enum tms_flag flag, char *pcFileName, char *signFileName);
int LedTwinkle_Api(int ledNum, int type, int count);
void SetApiCoreLogLevel(int level);		// Not for sure
void ApiCoreLog(char *Tag, const char *func, unsigned int level, char *format, ...);
int NetLinkCheck_Api(int type);
int NetModuleOper_Api(int type, int onOrOff);
void SysPowerReBoot_Api(void);

int wirelessGetSingnal_lib(void);
/*rssi[out]:
		0  		-113 dBm or less
 		1  		-111 dBm
 		2...30 	-109..-53 dBm
 		31  	-51 dBm or greater
 		99 		Not known or not detectable
  ber[out]:  0-success    <0-failed
  return  :
 * */
int wirelessGetCSQ_lib(int *rssi, int *ber);

/* Get network registered status
 * status[out]:
		0/2/3/4  		unregistered
 		others  		registered
  return  : 0-success    <0-failed
 * */
int wirelessGetRegInfo_lib(unsigned char *status);
/*
	pvTaskCode : task function
	pcName : task name
	usStackDepth : task size, maximum is 250K
	pvParameters:  input parameters
	uxPriority : priority ;  OSI_PRIORITY_NORMAL is suggested, and highest priority for APP thread is OSI_PRIORITY_NORMAL ;

	return :0-success  <0-failed
*/
int fibo_thread_create(void *pvTaskCode, char *pcName, int usStackDepth, void *pvParameters, int uxPriority);
unsigned int fibo_sem_new(unsigned char inivalue);
void fibo_sem_free(unsigned int semid);
void fibo_sem_wait(unsigned int semid);
void fibo_sem_signal(unsigned int semid);
unsigned char fibo_sem_try_wait(unsigned int semid, unsigned int timeout);

void PlayMP3File(char *audioFileName);

/*
	Set APN:
	Parameters:
		apn: APN
		username: username; set NULL if unnecessary
		password: password; set NULL if unnecessary
	Return : 0-success   <0-failed
	Noted :  parameters effect after terminal is restarted
			wirelessPdpWriteParam_lib(NULL, "", ""); -- will clear APN
*/
int wirelessPdpWriteParam_lib(char *apn, char *username, char *password);


// Update code here
#define KH_QRBMP_PATH "/ext/tms/%s_KHR.jpg"
#define USD_QRBMP_PATH "/ext/tms/%s_USD.jpg"
#define DUE_QRBMP_PATH "/ext/tms/%s_QR.jpg"


#define MAINLOG_L1(format, ...) ApiCoreLog("MAINAPI", __FUNCTION__, 1, format, ##__VA_ARGS__)

#define	SYS_PARAM_NAME		"sys_param.dat"

#define SYS_BANK 			"acleda"

#define TMS_DOWN_FILE		"/ext/UpdateData.zip"
#define BATER_WARM		 	15
#define BATER_POWR_OFF  	3
#define TMSPLAYONCETIME 	5							//When don't broadcast a trans, time it every five seconds
#define TMSMAXTIME (30+TMSPLAYONCETIME)     			//broadcast 30%10, 3 times
#define SAVE_UPDATE_FILE_NAME	"/ext/ifUpdate"		//record if the updated file, delete the file after update success

//file
void folderFileDisplay(unsigned char *filePath);
int unzipDownFile(unsigned char *fileName);

// mqtt
void initMqttOs(void);
void mQTTMainThread(void);
void TMSConnection(void);
void QRKeyInputThread(void);

// sound
void AppPlayTip(char *tip);

// monitor
void MonitorThread(void);

// tms
void set_tms_download_flag(int type);
void TMSThread(void);
void tms_TMSThread(void);

// network
void net_init(void);
void *net_connect(void* attch, const char *host,const char *port, int timerOutMs, int ssl, int *errCode);
int net_close(void *netContext);
int net_read(void *netContext, unsigned char* recvBuf, int needLen, int timeOutMs);
int net_write(void *netContext, unsigned char* sendBuf, int sendLen, int timeOutMs);

// param
typedef struct {
	char mqtt_server[128];
	char mqtt_port[8];
	int mqtt_ssl;		// 0-TCP, 1-SSL without cert, 2-SSL with cert
	int mqtt_qos;		// 0-QOS0, 1-QOS1, 2-QOS2
	int mqtt_keepalive;	// in seconds

	char mqtt_topic[32];
	char mqtt_client_id[32];
	char sn[32];
	int sound_level;
} SYS_PARAM;
extern SYS_PARAM G_sys_param;

typedef struct
{
	unsigned char public_key_modulus[512]; // Public Modulus
	unsigned char public_key_exponent[6];  // Public Exponent
} SYS_RSA_PARAM;
extern SYS_RSA_PARAM G_sys_rsa_param;

void initParam(void);
void saveParam(void);


void DispMainFace(void);
int WaitEvent(void);
void SelectMainMenu(void);


int tms_tms_flag;
void TransTapCard();

// ENCYPT

#define AES_BLOCK_SIZE_16 16
#define AES_BLOCK_SIZE_32 32

#define AES_ECB 0
#define AES_CBC 1
#define AES_IV_LEN 16
#define AES_KEY_LEN 32

extern unsigned int GsemaRef;

extern unsigned char ACLEDA_AES_KEY[AES_KEY_LEN];
extern unsigned char RANDOM_AES_KEY[AES_KEY_LEN];
extern unsigned char RANDOM_AES_KEY_PKCS1[256];

#define DES_ENCRYPTION_ECB_MODE 1
#define DES_DECRYPTION_ECB_MODE 0
#define DES_ENCRYPTION_CBC_MODE 3
#define DES_DECRYPTION_CBC_MODE 2

#define DATA_PACKET_SMA_LEN 256
#define DATA_PACKET_MID_LEN 512
#define DATA_PACKET_BIG_LEN 1024

#define RSA_KEY_LEN_IN_BITS 2048

extern unsigned char HTTP_RESP_DATA_SMA_PACKET[DATA_PACKET_SMA_LEN];
extern unsigned char HTTP_RESP_DATA_MID_PACKET[DATA_PACKET_MID_LEN];
extern unsigned char HTTP_RESP_DATA_BIG_PACKET[DATA_PACKET_BIG_LEN];
extern unsigned char HTTP_RESP_DATA_SUP_PACKET[DATA_PACKET_BIG_LEN * 2];

#define DUKPT_DAT_KEY_INJECTION_GROUP_INDEX 1
#define DUKPT_PIN_KEY_INJECTION_GROUP_INDEX 2

// ========= SHA_TYPE
#define SHA_TYPE_160 0
#define SHA_TYPE_224 1
#define SHA_TYPE_256 2
#define SHA_TYPE_384 3
#define SHA_TYPE_512 4
// =========

// ===== SERVER
//#define _TEST_SERVER_

#ifdef _TEST_SERVER_
	#define HOST_DOMAIN "115.159.28.147"
#else
	#define HOST_DOMAIN "103.25.92.104"
	// #define HOST_DOMAIN "103.83.163.69"
#endif

#define HOST_DOMAIN_PORT     "2000"
#define HOST_DOMAIN_AES_PORT "8000"

#define HTTP_REQUEST_ENCRYPT_DATA_SUFFIX "/smartpay/receive_data"

void MenuThread(void);
void ActionRefresh_Tread(void);

#define WIFI_HOTSPOT_CONFIG_SSID "Acleda SmartPay"
#define WIFI_HOTSPOT_CONFIG_PWD  "12345678"

#define WIFI_CONFIG_FILE "/ext/app/data/wifi_config.dat"
#define WIFI_PARAM_FILE  "/ext/app/data/wifi_param.dat"

#define INTERNET_CONFIG_FILE "/ext/app/data/internet_config.dat"
#define THANKS_MODE_CONFIG_FILE "/ext/app/data/thanks_mode.dat"

typedef struct
{
	unsigned char ucSsid[32];     /*çƒ­ç‚¹å��å­—*/
	unsigned char ucBssid[6];     /*MACåœ°å�€*/
	int iRssi;      			        /*ä¿¡å�·å¼ºåº¦*/
	int iEcn;      				        /*åŠ å¯†æ–¹å¼�*/
	unsigned char ucPassword[64]; /*è¿žæŽ¥å¯†ç �*/
	unsigned char ucConnect;      /*è¿žæŽ¥çŠ¶æ€�*/
	unsigned char ucRFU[32];      /*é¢„ç•™*/
} WIFI_AP;

typedef struct
{
	int iDHCPEnable; 	    /*DHCPä½¿èƒ½, 0 -- å…³é—­ 1 -- å¼€å�¯*/
	char cIp[20];	        /*é�™æ€�IP---å­—ç¬¦ä¸²å�‚æ•°*/
	char cNetMask[20];	  /*å­�ç½‘æŽ©ç �---å­—ç¬¦ä¸²å�‚æ•°*/
	char cGateWay[20];    /*ç½‘å…³---å­—ç¬¦ä¸²å�‚æ•°*/
	//char cDnsServer0[20]; /*DNSæœ�åŠ¡å™¨1 --- å­—ç¬¦ä¸²å�‚æ•°*/
	//char cDnsServer1[20]; /*DNSæœ�åŠ¡å™¨2 --- å­—ç¬¦ä¸²å�‚æ•°*/
} ST_WIFI_PARAM; // stationæ¨¡å¼�ä¸‹çš„çƒ­ç‚¹WiFiå�‚æ•°

extern int _IS_P_2_4_; // Support Multiple WIFI-SSL

extern int _IS_WIFI_ENABLED_;
extern int _IS_GPRS_ENABLED_;

extern int _IS_GPRS_READY_;
extern int _IS_WIFI_READY_;
extern int MQTT_INITIALIZED;
int readWifiParam();

extern int _NETWORKCONNECTION_;
#define WIFI_CONNECTION "WIFI";
#define SIM_CONNECTION "SIM";

// CERT-FILE
#define FILE_CERT_ROOT	  "/ext/app/data/ca.pem";
#define FILE_CERT_CHAIN   "/ext/app/data/cli.crt";
#define FILE_CERT_PRIVATE "/ext/app/data/pri.key";

/* NETWORK */

#define USB_PORT_NUM 	10 // USB-OUTPUT-PORT-DEFAULT
extern int _IS_PORT_OPEN_;
extern int THANKS_MODE;

extern int g_RefreshQRDisplay;
#endif /* __DEF_H__ */
