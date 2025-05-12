/*
 * EnvAcleda.h
 *
 *  Created on: Dec 17, 2024
 *      Author: kimlean
 */

#ifndef ENVACLEDA_H
#define ENVACLEDA_H

extern int MerchenID;

#define ENV_AC_APIHOST 		"qruat.acledabank.com.kh"
#define ENV_AC_APIPORT 		"443"


#define ENV_APIHOST 		"220.158.232.58"
#define ENV_APIPORT 		"8000"

// PATH FOR DO TRANSACTION FROM DEVICE
#define ENV_APIPATH "generate-url?"

// PATH FOR READ SOUND TRANSACTION
#define ENV_TRANSACTIONSUCCESS "/ext/tms/SuccessTransaction.mp3"
#define ENV_TRANSACTIONFAIL "/ext/tms/FailTransaction.mp3"

// TESTING DEFINER
#define ENV_TRANSACTIONPATH "/ext/tms/SoundTransaction.mp3"
#define ENV_SOUND "SoundTransaction.mp3"

#define BK_LOGO  "/ext/BK_LOGO.jpg"
#define QR_HEADER  "/ext/QRHeader.jpg"

// ============== CONTROLLER =====================

// PUBLICKEY
#define CONTROLLER_PUBLICKEY    "/NFC/CU06SER2LZ"
#define CONTROLLER_SMARTPAY_INFO    "/NFC/CU01SER2LZ"

#endif /* ENVACLEDA_H */
/* Configuration URLs and paths */
#define CONFIG_BASE_URL       "https://dgdiot.acledabank.com.kh/tms/config/"
#define CONFIG_FILE_NAME      "tms_config.dat"
#define CONFIG_SAVE_PATH      "/ext/tms/"

/* Default TMS values if download fails */
#define DEFAULT_TMS_HOST_IP       "103.235.231.19"
#define DEFAULT_TMS_HOST_DOMAIN   "ipos-os.jiewen.com.cn"
#define DEFAULT_TMS_HOST_PORT     "80"

/* Additional definitions for database configuration */
#define DB_BASE_URL_600      "http://103.25.92.104/tms/Vanstone/600/"
#define DB_BASE_URL_6000     "http://103.25.92.104/tms/Vanstone/6000/"
#define DB_BASE_URL_60000    "http://103.25.92.104/tms/Vanstone/60000/"

#define DB_FILENAME_600      "600.dat"
#define DB_FILENAME_6000     "6000.dat"
#define DB_FILENAME_60000    "60000.dat"

#define DB_SAVE_PATH         "/ext/tms/"

#define ENV_IOT_IP      "103.83.163.69"
#define ENV_IOT_PORT       "1883"

#define ENV_DEV_IOT_IP      "103.25.92.104"
#define ENV_DEV_IOT_PORT       "1883"
