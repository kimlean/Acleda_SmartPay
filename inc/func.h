#ifndef AFX_FUNC_H
#define AFX_FUNC_H

#define		MCARDNO_MAX_LEN 			19

typedef struct {
	unsigned char ucRecFalg;
	int IccOnline;
	unsigned char IccFallBack;
	unsigned short nIccDataLen;
	unsigned char Trans_id;
	char MainAcc[22];
	unsigned char TradeAmount[6+1];//
	unsigned char TradeDate[4];	//
	unsigned char TradeTime[3];    //
	unsigned char OperatorNo;
	unsigned int lTraceNo;
	unsigned int lNowBatchNum;
	char szRespCode[2+1];
	unsigned char ExpDate[4];
	unsigned char EntryMode[4];
	char SysReferNo[13];
	char AuthCode[7];
	char TerminalNo[9];
	char MerchantNo[16];
	char MerchTermNO[5];
	unsigned char SecondAmount[6];
	char SecondAcc[21];
	char HoldCardName[20+1];
	char AddInfo[122+1];
	unsigned int OldTraceNo;
	unsigned int OldBatchNum;
	char OldTransDate[9];
	char OldSysRefNo[13];
	char OldTermNo[9];
	// EMV
	unsigned char	IccData[1+255];
	char szCardUnit[4];
	unsigned char	bPanSeqNoOk;
	unsigned char	ucPanSeqNo;
	unsigned char	sAppCrypto[8];
	unsigned char	sAuthRspCode[2];
	unsigned char	sTVR[5];
	unsigned char	sAIP[2];
	unsigned char  szUnknowNum[4];
	char szAID[32+1];
	char szAppLable[16+1];
	unsigned char	sTSI[2];
	unsigned char	sATC[2];
	unsigned char	szAppPreferName[16+1];
} LOG_STRC;
#define LOG_SIZE  sizeof(LOG_STRC)

typedef struct{
	unsigned char sPIN[9];
	unsigned char BalanceAmount[1 + 40];
	unsigned char Track1[88];
	unsigned char Track2[2 + 37];
	unsigned char Track3[2 + 107];
	unsigned char Track1Len;
	unsigned char Track2Len;
	unsigned char Track3Len;
	LOG_STRC stTrans;
	unsigned char HaveInputAmt;
	unsigned char HaveInputPin;
	unsigned short nRespIccLen;
	unsigned char RespIccData[512];
	int TranKernelType;
} POS_COM;


struct _CtrlParam{
	unsigned char		pinpad_type;
	unsigned char		AKeyIndes;
	unsigned char		MainKeyIdx;
	unsigned char		PinKeyIndes;
	unsigned char		MacKeyIndes;
	int		lTraceNo;
	int		lNowBatchNum;
	unsigned short		iTransNum;
	unsigned char		beepForInput;
	unsigned char		oprTimeoutValue;
	unsigned char		tradeTimeoutValue;
	char	TerminalNo[9];
	char	MerchantNo[16];
	char	MerchantName[41];
	unsigned char		DesType;
	unsigned char		PreDial;
	unsigned char		ShieldPAN;
	unsigned char		SupportICC;
	unsigned char		SupportPICC;
	unsigned char		SupportSignPad;
	unsigned char		SignTimeoutS;
	unsigned short		SingRecNum;
	unsigned char		SignMaxNum;
	unsigned short		SignBagMaxLen;
};
extern struct _CtrlParam gCtrlParam;

enum
{
	PIN_HAVE_INPUT =	0x10,
	PIN_NOT_INPUT =		0x20,
};

#endif

