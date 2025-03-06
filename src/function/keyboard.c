#include "def.h"

#include <coredef.h>
#include <poslib.h>

#include <string.h>
#include <struct.h>

/* ---------------------------------------------------------------------
 *  0  |  9   |  .   |  F1  |  Fn  |   x  |  ->  |  +   | Enter |  *   |
 * ---------------------------------------------------------------------
 * 0x30| 0x39 | 0x0b | 0x14 | 0x16 | 0x1b | 0x08 | 0x2b | 0x0d  | 0x0a |
 * ---------------------------------------------------------------------
 *  48 |  57  |  11  |  20  |  22  |  27  |   8  |  43  |  13   |  10  |
 * ---------------------------------------------------------------------
 */
void KeyBoardThread(void)
{
	unsigned char key;
	while (1)
	{
		key = GetKey_Api();

		char info[12] = "";
		sprintf(info, "KEY = 0x%x", key);
		LogPrintInfo(info);

		Delay_Api(200);
	}
}

int keyTransNum(int key)
{
	int num = -1;

	switch (key) {
		case 48: num = 0; break;
		case 49: num = 1; break;
		case 50: num = 2; break;
		case 51: num = 3; break;
		case 52: num = 4; break;
		case 53: num = 5; break;
		case 54: num = 6; break;
		case 55: num = 7; break;
		case 56: num = 8; break;
		case 57: num = 9; break;
		default: break;
	}

	return num;
}
