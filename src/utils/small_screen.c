void small_screen(char* amountStr,char* currencyStr) {
	secscrOpen_lib();
	secscrCls_lib();
//	ScrFontSet_Api(2);
	secscrSetAttrib_lib(4, 1);
	secscrClrLine_lib(1,1);
//	secscrSetAttrib_lib(7, 0);
	char currencys[256];

	snprintf(currencys, sizeof(currencys), "%s %s", amountStr, currencyStr);
	secscrPrint_lib(0, 0, 0, currencys);
//	Delay_Api(5000);
	secscrSetBackLightMode_lib(1,500); //3000 = 30 Seconds
//	secscrCls_lib();
//	secscrClose_lib();
}
