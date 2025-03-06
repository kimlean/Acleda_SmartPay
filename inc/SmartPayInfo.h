/*
 * SmartPayInfo.h
 *
 *  Created on: Jan 21, 2025
 *      Author: KIMLEAN
 */

#ifndef SMARTPAYINFO_H_
#define SMARTPAYINFO_H_

typedef struct {
    char regId[50];
    char accIdUsd[50];
    char accIdKhr[50];
    char mctName[50];
    char mmc[50];
    char mbNo[50];
    char counCode[50];
    char cityName[50];
    char mctId[50];
    char adDataTCGUID[100];
} SmartPay;

typedef struct {
	char amount[32]; // DONE
	char ccy[4];  // DONE
	char creditAccount[50]; // DONE
} NFC_PAYMENT;


extern SmartPay SmartPay_Info;

int GetSmartPayInfo_Service();

#endif /* SMARTPAYINFO_H_ */
