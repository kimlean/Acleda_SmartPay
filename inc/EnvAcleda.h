/*
 * EnvAcleda.h
 *
 *  Created on: Dec 17, 2024
 *      Author: kimlean
 */

#ifndef ENVACLEDA_H
#define ENVACLEDA_H

extern int MerchenID;

#define ENV_AC_APIHOST 		"chatmbuat.acledabank.com.kh"
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

#define USD_LOGO  "/ext/USD.jpg"
#define KH_LOGO  "/ext/KHR.jpg"
#define BK_LOGO  "/ext/BK_LOGO.jpg"
#define QR_HEADER  "/ext/QRHeader.jpg"

// ============== CONTROLLER =====================

// PUBLICKEY
#define CONTROLLER_PUBLICKEY    "/NFC/CU06SER2LZ"
#define CONTROLLER_SMARTPAY_INFO    "/NFC/CU01SER2LZ"

#endif /* ENVACLEDA_H */
