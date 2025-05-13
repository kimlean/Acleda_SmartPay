/*
 * main.c
 *
 * Created on: Dec 08, 2024
 * Author: kimlean
 */

/*------ HEADER ------*/
#include "EnvAcleda.h"
#include <stdio.h>
#include <string.h>

#include "def.h"
#include "SmartPayInfo.h"
#include "PublicKey.h"
#include "tms_tms.h"

#include <struct.h>
#include <poslib.h>

/* Application version and identification information */
const APP_MSG App_Msg = {
    "ACLEDA BANK", "ACLEDA-App", "V1.0", "KIMLEAN",
    __DATE__ " " __TIME__, "", 0, 0, 0, "00001001140616"
};

/* Global crypto keys */
unsigned char RANDOM_AES_KEY[AES_KEY_LEN];
unsigned char ACLEDA_AES_KEY[AES_KEY_LEN] = "d900d6e900081ae0e84ad160c40000fa";
unsigned char PK_ENCODE_AES_IV[AES_KEY_LEN];
unsigned char PK_DECODE_AES_IV[AES_KEY_LEN];

/* Global semaphore reference */
unsigned int GsemaRef;

/* Global state flags */
int _IS_P_2_4_ = 0;
FILE *fptr;

/* Global data structures */
NFCTAPINFO NFC_INFO;
SmartPay SmartPay_Info;

int THANKS_MODE = 0; // Default to enabled (1)
int DISPLAY_QR_TYPE = 1;
int g_RefreshQRDisplay = 0;

char KHQRBMP[50];
char USDQRBMP[50];
//char DUEQRBMP[50];

int g_khrQrExists = 0;
int g_usdQrExists = 0;

void initParmSystem(){
    net_init();
    initParam();
    initDeviceType();
    initMqttOs();
    initLib();
    initThanksMode();

    // SMALL SCREEN INIT
    secscrOpen_lib();
    secscrSetBackLightMode_lib(1,100);
}

void initThanksMode() {
    // Load saved Thanks Mode preference
    THANKS_MODE = getThanksModeType();
}

void initQRImage(){
	// PRODUCTION CODE
	memset(KHQRBMP, 0, sizeof(KHQRBMP));
	sprintf(KHQRBMP, KH_QRBMP_PATH, G_sys_param.sn);
	memset(USDQRBMP, 0, sizeof(USDQRBMP));
	sprintf(USDQRBMP, USD_QRBMP_PATH, G_sys_param.sn);

	// Check if files exist and set global flags
	g_khrQrExists = (GetFileSize_Api(KHQRBMP) > 0) ? 1 : 0;
	g_usdQrExists = (GetFileSize_Api(USDQRBMP) > 0) ? 1 : 0;
}

void InitSys(void) {
    int ret;

    /* Create audio directory if it doesn't exist */
    ret = GetFileSize_Api("/ext/tms/");
    if (ret <= 0) {
        fileMkdir_lib("/ext/tms/");
    }
    
    /* Initialize system components */
    initParmSystem();

    /* Ensure TMS settings are initialized with defaults before any attempt to use them */
    strcpy(_TMS_HOST_IP_, DEFAULT_TMS_HOST_IP);
    strcpy(_TMS_HOST_DOMAIN_, DEFAULT_TMS_HOST_DOMAIN);
    strcpy(_TMS_HOST_PORT_, DEFAULT_TMS_HOST_PORT);

    /* Read hardware information */
    unsigned char bp[32];
    memset(bp, 0, sizeof(bp));
    sysReadBPVersion_lib(bp);
    memset(bp, 0, sizeof(bp));
    sysReadVerInfo_lib(4, bp);

    /* Download configuration */
    download_config_dB();
    check_exist_files();

    initQRImage();

    // CLOSE TEST CODE
    TMSConnection();

    /* Initialize data structures */
    memset(&NFC_INFO, 0, sizeof(NFCTAPINFO));
    memset(&SmartPay_Info, 0, sizeof(SmartPay));

    /* CONNET TO MB SERVER TO GET VALUE */
    GetPublicKey_Service();

    /* Create threads */
    fibo_thread_create(MenuThread, "MenuThread", 14 * 1024, NULL, 24);
    fibo_thread_create(ActionRefresh_Tread, "ActionRefresh_Tread", 8 * 1024, NULL, 24);

    /* Create synchronization semaphore */
    GsemaRef = fibo_sem_new(1);
}

int AppMain(int argc, char **argv)
{
    /* Initialize core system */
    SystemInit_Api(argc, argv);

    /* Initialize communications */
    InitConnection();

    /* Configure logging (disable in production) */
    SetApiCoreLogLevel(1); // 0-turn Off log
    EnableLogPortPrint(); // Enable USB Output

    ScrCls_Api();
    ScrClsRam_Api();

    /* Display startup message */
    ScrDispRam_Api(LINE5, 0, "Please Wait", CDISP);
    ScrDispRam_Api(LINE6, 0, "Connecting", CDISP);
    ScrBrush_Api();

    /* Initialize system parameters and components */
    InitSys();

    while (!checkInternetStatus()) {
    	mQTTMainThread();
        Delay_Api(2000); // 2 second delay
    }
    return 0;
}
