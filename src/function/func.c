#include <string.h>
#include "def.h"
#include "EmvCommon.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

#include <stdint.h>
#include "sha256.h"

#include <ctype.h>
#define		MAX_LCDWIDTH				21

struct _CtrlParam gCtrlParam;
const char BASE_CODECODE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void filter_printable_chars(const char *input, char *output) {
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        if (isprint((unsigned char)input[i])) {  // Check if character is printable
            output[j++] = input[i];
        }
    }
    output[j] = '\0';  // Null-terminate the output string
}


int _IS_PORT_OPEN_ = 0;

int GetScanf(u32 mode, int Min, int Max, char *outBuf, u32 TimeOut, u8 StartRow, u8 EndRow, int howstr)
{
	return GetScanfEx_Api(mode, Min, Max, outBuf, TimeOut, StartRow, EndRow, howstr, MMI_NUMBER);
}

int GetAmount(u8 *pAmt, int currency)
{
    int ret;
    char buf[32], temp[32];
    int amt, minAmount;

    // Set minimum amount based on currency
    // For KHR (currency 1): 10000 (100.00)
    // For USD (currency 2): 2 (0.02)
    minAmount = (currency == 1) ? 10000 : 2;

    memset(buf, 0, sizeof(buf));
    memset(temp, 0, sizeof(temp));

    while(1)
    {
        memset(buf, 0, sizeof(buf));

        // Use MMI_POINT for both currencies since both can have decimal places
        ret = GetScanf(MMI_POINT, 1, 9, buf, 60, LINE4, LINE4, MAX_LCDWIDTH);

        if(ret == ENTER)
        {
            // Format input to BCD
            memset(temp, 0x30, 12-buf[0]);
            strcpy(&temp[12-buf[0]], &buf[1]);
            AscToBcd_Api(pAmt, temp, 12);

            // Convert to long to check minimum amount
            amt = BcdToLong_Api(pAmt, 6);

            // Check if amount meets minimum requirement
            if (amt < minAmount)
            {
                Beep_Api(1); // Error beep

                ScrClrLine_Api(LINE5, LINE6);  // Clear any previous error messages


                // Display appropriate error message
                if (currency == 1) {
                    ScrDisp_Api(LINE5, 0, "Amount must greater", CDISP);
                    ScrDisp_Api(LINE6, 0, "then 100.00 KHR", CDISP);
                } else {
                    ScrDisp_Api(LINE5, 0, "Amount must greater", CDISP);
                    ScrDisp_Api(LINE6, 0, "then 0.02 USD", CDISP);
                }

                // Wait a moment to show error message
                WaitAnyKey_Api(3);

                // Clear the error message and let user try again
                ScrClrLine_Api(LINE5, LINE6);
                ScrSetColor_Api(0x0000, 0xFFFF);

                // Continue the loop to get input again
                continue;
            }

            return 0;  // Valid amount entered
        }
        else
        {
            return -1;  // User canceled or error
        }
    }
}

void GetPanNumber()
{
	int tlvLen = 0, i,ret;
	u8 buf[64];
	u8 TEM[64];

	memset(buf, 0, sizeof(buf));
	memset(TEM, 0, sizeof(TEM));
	ret = Common_GetTLV_Api(0x5A, buf, &tlvLen);
	if(ret == 0) //
	{
		BcdToAsc_Api(PosCom.stTrans.MainAcc, buf, tlvLen * 2);
		for(i = tlvLen * 2 - 1; i >= 0; --i)
		{
			if(PosCom.stTrans.MainAcc[i] == 'F' || PosCom.stTrans.MainAcc[i] == 'f')
				PosCom.stTrans.MainAcc[i] = 0;
			else
				break;
		}
	}
	else
	{
		ret = Common_GetTLV_Api(0x57, buf, &tlvLen);
		if (ret == 0)
		{

			BcdToAsc_Api(TEM, buf, tlvLen * 2);
			for(i = tlvLen * 2 - 1; i >= 0; --i)
			{
				if(TEM[i] == 'D'){
					TEM[i] = 0;
					memcpy(PosCom.stTrans.MainAcc,TEM, i);
					break;
				}
			}
		}
	}

}

int EnterPIN(u8 flag)
{
	int ret;
	u8 DesFlag;
	u8 temp[128];
	char dispbuf[32];

	memset(dispbuf,0,sizeof(dispbuf));
	ScrClrLine_Api(LINE2, LINE5);
	if(flag != 0)
		ScrDisp_Api(LINE2, 0, "Password Is Wrong, Please Input Again:", LDISP);
	else
		ScrDisp_Api(LINE2, 0, "Please Input Password:", LDISP);

	if(gCtrlParam.DesType == 1) //Des
		DesFlag = 0x01;
	else
		DesFlag = 0x03;


	ret = PEDGetPwd_Api(gCtrlParam.PinKeyIndes, 4, 8, PosCom.stTrans.MainAcc, PosCom.sPIN, DesFlag);

	if(ret != 0)
		return -1;

	if(memcmp(PosCom.sPIN, "\0\0\0\0\0\0\0\0", 8) == 0)
		PosCom.stTrans.EntryMode[1] = PIN_NOT_INPUT;
	else
		PosCom.stTrans.EntryMode[1] = PIN_HAVE_INPUT;

	return 0;
}

int GetCardNoFromTrack2Data(char *cardNo, u8 *track2Data)
{
	char tmp[MCARDNO_MAX_LEN + 2], *p = NULL ;
	u32 len;

	memset(tmp, 0, sizeof(tmp));

	len = MCARDNO_MAX_LEN+1 < track2Data[0]*2 ? MCARDNO_MAX_LEN+1 : track2Data[0]*2;

	FormBcdToAsc( tmp, track2Data+1, len);
	tmp[len] = '\0';
	p = strchr(tmp, '=');//
	if(p != NULL)
	{
		*p = 0;
		strcpy(cardNo, tmp);
		return TRUE;
	}

	return FALSE;
}

int DispCardNo(void)
{
	int iRet;
	u8 pTrackBuf[256];

	memset (pTrackBuf, 0, sizeof(pTrackBuf));
	iRet = GetEmvTrackData(pTrackBuf);
	if(iRet == 0)
	{
		GetCardNoFromTrack2Data(PosCom.stTrans.MainAcc, pTrackBuf);
		//ScrClrLineRam_Api(LINE2, LINE5);
		//ScrCls_Api(line);
		ScrClrLine_Api(LINE2, LINE10);
		ScrDispRam_Api(LINE3, 0,"PLS confirm:", LDISP);
		ScrDispRam_Api(LINE4, 0,PosCom.stTrans.MainAcc, RDISP);
		ScrDispRam_Api(LINE6, 0,"ENTER to continue", RDISP);
		ScrBrush_Api();
	}
	KBFlush_Api ();
	iRet = WaitEnterAndEscKey_Api(30);
	if(iRet != ENTER)
		return ERR_NOTACCEPT;
	return 0;
}

void RemoveTailChars(char* pString, char cRemove)
{
	int nLen = 0;

	nLen = strlen(pString);
	while(nLen)
	{
		nLen--;
		if(pString[nLen] == cRemove)
			pString[nLen] = 0;
		else
			break;
	};
}

int MatchTrack2AndPan(u8 *pTrack2, u8 *pPan)
{
	int  i = 0;
	char szTemp[19+1], sTrack[256], sPan[256];

	memset(szTemp, 0, sizeof(szTemp));
	memset(sTrack, 0, sizeof(sTrack));
	memset(sPan, 0, sizeof(sPan));

	//track2
	BcdToAsc_Api(sTrack, &pTrack2[1], (u16)(pTrack2[0]*2));
	RemoveTailChars(sTrack, 'F');		// erase padded 'F' chars
	for(i=0; sTrack[i]!='\0'; i++)		// convert 'D' to '='
	{
		if(sTrack[i]=='D' )
		{
			sTrack[i] = '=';
			break;
		}
	}
	for(i=0; i<19 && sTrack[i]!='\0'; i++)
	{
		if(sTrack[i] == '=') break;
		szTemp[i] = sTrack[i];
	}
	szTemp[i] = 0;
	//pan
	BcdToAsc_Api(sPan, &pPan[1], (int)(pPan[0]*2));
	RemoveTailChars(sPan, 'F');         // erase padded 'F' chars

	if(strcmp(szTemp, sPan)==0)
		return 0;
	else
		return 1;
}


// ==================== PKCS ====================
// Helper function to convert Base64 to Hex
typedef unsigned char BYTE;
void base64_to_hex(const char* base64_str, char* hex_output) {
    const char hex_chars[] = "0123456789abcdef";
    int i = 0;
    for (i = 0; base64_str[i] != '\0'; i++) {
        BYTE b = (BYTE) base64_str[i];
        hex_output[2 * i] = hex_chars[(b >> 4) & 0x0F];
        hex_output[2 * i + 1] = hex_chars[b & 0x0F];
    }
    hex_output[2 * i] = '\0';
}

// Main encryption function
void encrypt_with_aes(const char* plaintext, const char* key, char* output) {
    // 1. Hash the key using SHA256 (you should have an SHA256 implementation in your codebase)
    calcSha_lib(key, strlen(key), output, SHA_TYPE_160);
    // MAINLOG_L1("calcSha_lib:\n%s\n", output);

    // 2. Encrypt the plaintext using AES (AES128 or AES256 based on your key length)
    unsigned char CBC_IV[] = {
		0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
		0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66
	};

    calcAesEnc_lib(plaintext, strlen(plaintext), output, output, AES_KEY_LEN * 2, CBC_IV, AES_ECB);
    // MAINLOG_L1("calcAesEnc_lib:\n%s\n", output);

    // 3. Base64 encode the encrypted data
    char base64_encoded[2048]; // Adjust size as needed
    base64Encode(output, base64_encoded, base64_encoded);

    // 4. Convert the Base64 output to hexadecimal
    base64_to_hex(base64_encoded, output);
}

void pkcs7_padding_customize(const char *input, int input_len, char *output, int padded_len) {
    int padding_size = padded_len - input_len;

    // Copy the input data to the output buffer
    memcpy(output, input, input_len);

    // Add padding bytes
    memset(output + input_len, padding_size, padding_size);

    // Debug log to verify padded data
    // MAINLOG_L1("Padded data (hex): %s\n", to_hex_string(output, padded_len));
}

int pkcs7_pad(const uint8_t *data, int data_len,
                     uint8_t *padded_out, int *padded_len)
{
    // Block size for AES is 16
    const int block_size = 16;

    // Number of padding bytes needed
    int pad_size = block_size - (data_len % block_size);
    if (pad_size == 0) {
        pad_size = block_size;
    }

    // The new total length
    int new_len = data_len + pad_size;
    // Copy the original data
    memcpy(padded_out, data, data_len);
    // Fill the padding bytes (each = pad_size)
    for (int i = 0; i < pad_size; i++) {
        padded_out[data_len + i] = (uint8_t)pad_size;
    }

    *padded_len = new_len;
    return 0;
}

void pkcs7_padding(char *data, int data_len, char *result, int result_len, int padded_len)
{
	memcpy(result, data, data_len);

	for (int i = data_len; i < result_len; ++i) {
		result[i] = (unsigned char)padded_len;
	}

	char result_data[result_len + 8];
	memset(result_data, 0, sizeof(result_data));
	sprintf(result_data, "%s(%d)", result, result_len);
	// MAINLOG_L1(result_data);

	char result_data_asc[(result_len * 2) + 8];
	memset(result_data_asc, 0, sizeof(result_data_asc));
	BcdToAsc_Api(result_data_asc, result, result_len * 2);
	// MAINLOG_L1(result_data_asc);
}
// pkcs1
void pkcs1_padding(char *result)
{
	int block_size = RSA_KEY_LEN_IN_BITS / 8; // 256

	int padding_size = block_size - AES_KEY_LEN; // 224
	// MAINLOG_L1("padding_size = ", padding_size);

	unsigned char padded_block[padding_size + 8];
	memset(padded_block, 0, sizeof(padded_block));

	/* EB = 00 + BT + PS + 00 + D
	 * result = 0×00(padded_bock_falg) + 0×02(00/01-private_key_encryption, 02-public_key_encryption) + PADDED_BLOCK + 0×00(padded_block_flag) + data
	 *
	 * BT = 0, PS = 00; BT = 01, PS = FF; BT = 02, PS != 00
	 */
	int i;
	padded_block[0] = 0x00;
	padded_block[1] = 0x02;

	srand(time(0));
	for (i = 2; i < padding_size - 1; ++i)
	{
		padded_block[i] = rand() % 256;

		while (padded_block[i] == 0) {
			padded_block[i] = rand() % 256;
		}
	}
	padded_block[padding_size - 1] = 0x00;

	char padded_block_asc[padding_size * 2 + 8];
	memset(padded_block_asc, 0, sizeof(padded_block_asc));
	BcdToAsc_Api(padded_block_asc, padded_block, padding_size * 2);
	// MAINLOG_L1("PADDED_BLOCK(asc): ", padded_block_asc);

	char pkcs1_data[256 + 8] = {0};
	memcpy(pkcs1_data, padded_block, padding_size);

	for (int j = 0, i = padding_size; i < block_size; ++j, ++i) {
		pkcs1_data[i] = RANDOM_AES_KEY[j];
	}
	memcpy(result, pkcs1_data, block_size);
}
// ==================== PKCS ====================

// ==================== Base64 ====================
int base64Encode(char *input, char *output, int inputLen)
{
	register int vLen = 0;
	int ret;

    while (inputLen > 0)
    {
		*output++ = BASE_CODECODE[(input[0] >> 2 ) & 0x3F];

        if (inputLen > 2) {
			*output++ = BASE_CODECODE[((input[0] & 3) << 4) | ((input[1] >> 4) & 0x0f)];
			*output++ = BASE_CODECODE[((input[1] & 0xF) << 2) | ((input[2] >> 6) & 0x03)];
            *output++ = BASE_CODECODE[input[2] & 0x3F];
        }
        else
        {
        	switch (inputLen)
        	{
				case 1:
                    *output ++ = BASE_CODECODE[(input[0] & 3) << 4 ];
                    *output ++ = '=';
                    *output ++ = '=';

                    break;

                case 2:
                    *output ++ = BASE_CODECODE[((input[0] & 3) << 4) | (input[1] >> 4)];
                    *output ++ = BASE_CODECODE[((input[1] & 0x0F) << 2) | (input[2] >> 6)];
                    *output ++ = '=';

                    break;
            }
        }

        input    += 3;
        inputLen -= 3;
        vLen 	 += 4;
    }

    *output = 0;

    return vLen;
}
char getCharIndex(char c)
{
	if ((c >= 'A') && (c <= 'Z')) {
		return c - 'A';
    }
	else if ((c >= 'a') && (c <= 'z')) {
		return c - 'a' + 26;
    }
	else if ((c >= '0') && (c <= '9')) {
		return c - '0' + 52;
    }
	else if(c == '+') {
		return 62;
    }
	else if(c == '/') {
		return 63;
    }
	else if(c == '=') {
    	return 0;
    }

    return 0;
}
int base64Decode(char *output, char *input, int inputLen)
{
	static char lpCode[4];

    register int vLen = 0;
	int su_num = 0;

    if (inputLen % 4)
    {
    	output[0] = '\0';
        return -1;
    }

	if (input[inputLen - 1] == '=')
	{
		su_num++;
		if (input[inputLen - 2] == '=') { su_num++; }
	}

    while (inputLen > 2)
    {
    	lpCode[0] = getCharIndex(input[0]);
        lpCode[1] = getCharIndex(input[1]);
        lpCode[2] = getCharIndex(input[2]);
        lpCode[3] = getCharIndex(input[3]);

        *output++ = (lpCode[0] << 2) | (lpCode[1] >> 4);
        *output++ = (lpCode[1] << 4) | (lpCode[2] >> 2);
        *output++ = (lpCode[2] << 6) | (lpCode[3]);

        input    += 4;
        inputLen -= 4;
        vLen 	 += 3;
    }

	vLen -= su_num;

    return vLen;
}

// ==================== Base64 ====================

// ==================== HEX ====================

void bytes_to_hex(const char *bytes, int len, char *hex_output) {
    for (int i = 0; i < len; i++) {
        sprintf(hex_output + (i * 2), "%02x", (unsigned char)bytes[i]);
    }
}

void hex_to_bytes(const char *hex_input, char *bytes_output) {
    int len = strlen(hex_input) / 2;
    for (int i = 0; i < len; i++) {
        sscanf(hex_input + (i * 2), "%02x", (unsigned int *)&bytes_output[i]);
    }
}

// ==================== HEX ====================

void wifiReboot()
{
	int ret;

	ret = wifiClose_lib();
	if (ret != 0) { MAINLOG_L1( "!!! wifiClose_lib() failed(%d) !!!", ret); }

	Delay_Api(200);

	ret = wifiOpen_lib();
	if (ret != 0) {  MAINLOG_L1(  "!!! wifiOpen_lib() failed(%d) !!!", ret); }
}

// ==================== Log Port Print ====================
void EnableLogPortPrint()
{
	int ret = portOpen_lib(USB_PORT_NUM, NULL);

	if (ret != 0) MAINLOG_L1("!!! Port Open Failed(%d) !!!", ret);
	else {
		_IS_PORT_OPEN_ = 1;

		ret = portFlushBuf_lib(USB_PORT_NUM);
		if (ret != 0) MAINLOG_L1("!!! Port Flush Failed(%d) !!!", ret);
	}
}

void LogPrintNoRet(char *data1, char *data2)
{
	int len = strlen(data1) + strlen(data2) + 8;

	char buf[len];
	memset(buf, 0, sizeof(buf));

	sprintf(buf, "%s%s", data1, data2);

	// MAINLOG_L1(buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s%s\n", data1, data2);

	if (_IS_PORT_OPEN_) {
		portSends_lib(USB_PORT_NUM, buf, strlen(buf));
	}
}

void LogPrintWithRet(int log_style, char *data, int ret)
{
	int len = strlen(data) + 8;

	char buf[len];
	memset(buf, 0, sizeof(buf));

	if (log_style == 0) { // Normal, ret at END
		sprintf(buf, "%s%d", data, ret);
		// MAINLOG_L1(buf);

		memset(buf, 0, sizeof(buf));

		sprintf(buf, "%s%d\n", data, ret);

	} else { // UnNormal, ret at OTHER POS.
		sprintf(buf, data, ret);
		// MAINLOG_L1(buf);

		memset(buf, 0, sizeof(buf));

		sprintf(buf, data, ret);
		strcat(buf, "\n");
	}

	if (_IS_PORT_OPEN_) {
		portSends_lib(USB_PORT_NUM, buf, strlen(buf));
	}
}

void LogPrintInfo(char *data)
{
	portFlushBuf_lib(USB_PORT_NUM);

	int len = strlen(data) + 8;

	char buf[len];
	memset(buf, 0, sizeof(buf));

	sprintf(buf, "%s", data);
	// MAINLOG_L1(buf);

	memset(buf, 0, sizeof(buf));

	sprintf(buf, "%s\n", data);

	if (_IS_PORT_OPEN_) {
		portSends_lib(USB_PORT_NUM, buf, strlen(buf));
	}
}
// ==================== Log Port Print ====================

