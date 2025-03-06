/*
 * main.c
 *
 * Created on: Dec 08, 2024
 * Author: kimlean
 */

/*------ HEADER ------*/
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
    "Demo", "Demo-App", "V1.0", "VANSTONE",
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

/* Global data structures */
NFCTAPINFO NFC_INFO;
SmartPay SmartPay_Info;

void InitSys(void) {
    int ret;

    /* Create audio directory if it doesn't exist */
    ret = GetFileSize_Api("/ext/audios/");
    if (ret <= 0) {
        fileMkdir_lib("/ext/audios/");
    }

    /* Initialize system components */
    initParam();
    initDeviceType();
    net_init();
    initMqttOs();
    initLib();

    /* Read hardware information */
    unsigned char bp[32];
    memset(bp, 0, sizeof(bp));
    sysReadBPVersion_lib(bp);
    memset(bp, 0, sizeof(bp));
    sysReadVerInfo_lib(4, bp);

    /* Download configuration */
    download_config_file();
    download_config_dB();

    TmsTest();

    /* Initialize data structures */
    memset(&NFC_INFO, 0, sizeof(NFCTAPINFO));
    memset(&SmartPay_Info, 0, sizeof(SmartPay));

    /* System preparation */
    AppPlayTip("Connect to the bank");
    GetPublicKey_Service();

    /* Create threads */
    fibo_thread_create(MenuThread, "MenuThread", 14 * 1024, NULL, 24);
    fibo_thread_create(mQTTMainThread, "MQTT_Thread", 14 * 1024, NULL, 24);

    /* Create synchronization semaphore */
    GsemaRef = fibo_sem_new(1);
}

int AppMain(int argc, char **argv) {
    /* Initialize core system */
    SystemInit_Api(argc, argv);

    /* Configure logging (disable in production) */
    SetApiCoreLogLevel(1); // 0-turn Off log
    EnableLogPortPrint(); // Enable USB Output

    /* Display startup message */
    ScrDispRam_Api(LINE5, 0, "Please Wait", CDISP);
    ScrDispRam_Api(LINE6, 0, "Connecting", CDISP);
    ScrBrush_Api();

    /* Initialize communications */
    InitConnection();

    /* Initialize system parameters and components */
    InitSys();

    /* Wait for internet connection */
    int connectionCheckCount = 0;
    const int MAX_CONNECTION_ATTEMPTS = 30; // Limit max attempts

    while (!checkInternetStatus()) {
        Delay_Api(2000); // 2 second delay
    }
    return 0;
}
