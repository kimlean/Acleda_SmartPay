void small_screen(char* amountStr,char* currencyStr) {
	secscrCls_lib();
	secscrSetAttrib_lib(4, 1);
	secscrClrLine_lib(1,1);
	char currencys[256];

	snprintf(currencys, sizeof(currencys), "%s %s", amountStr, currencyStr);
	secscrPrint_lib(0, 0, 0, currencys);
	secscrSetBackLightMode_lib(1,500); //3000 = 30 Seconds
}
