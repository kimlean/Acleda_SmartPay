/*
 * internet_config.c
 *
 *  Created on: Feb 10, 2025
 *      Author: kimlean
 */
#include "def.h"
#include <coredef.h>
#include <poslib.h>

int _IS_GPRS_READY_ = -1;
int _IS_WIFI_READY_ = -1;

int _IS_WIFI_ENABLED_ = 0;
int _IS_GPRS_ENABLED_ = 0;

void EnableWIFI()
{
	int ret;

	ret = NetModuleOper_Api(WIFI, 1);
	if (ret != 0)
	{
		MAINLOG_L1("!!! NetModuleOper_Api(WIFI, 1) failed(%d) !!!", ret);
		return;
	}

	ret = wifiOpen_lib();
	if (ret != 0)
	{
		MAINLOG_L1("!!! wifiOpen_lib() failed(%d) !!!", ret);
		return;
	}

	int file_size = GetFileSize_Api(WIFI_CONFIG_FILE);
	if (file_size == 0)
	{
		AppPlayTip("Please config Wifi");
		WifiConfig();
	}
	else
	{
		char buf[2] = {0};
		ret = ReadFile_Api(WIFI_CONFIG_FILE, buf, 0, &file_size);

		if (ret != 0)
		{
			MAINLOG_L1("!!! ReadFile_Api('wifi_config.dat') failed(%d) !!!", ret);
			AppPlayTip("Read Wifi configuration failed, please config Wifi again");

			WifiConfig();
		}
		else
		{
			int flag = atoi(buf);
			MAINLOG_L1("WIFI_CONFIG_FLAG = %d", flag);

			if (flag == 1)
			{
				int check_count = 0;

			WIFI_CONNECT_CHECK:
				MAINLOG_L1("WIFI_CONNECT_CHECK PROCESS = %d", check_count);
				Delay_Api(5000);
				ret = wifiGetLinkStatus_lib();
				MAINLOG_L1("WIFI_LINK_STATUS = %d", ret);

				if (ret == 2)
				{
					_IS_WIFI_ENABLED_ = 1;

					// CLOSE THE INTERNET
					_IS_GPRS_ENABLED_ = 0;
					AppPlayTip("Wifi connected");
				}
				else
				{
					if (check_count >= 1)
					{
						AppPlayTip("Can not connect to your wifi.");
						_IS_WIFI_ENABLED_ = 0;
						EnableGPRS();
						return;
					}
					else
					{
						check_count++;

						//							MAINLOG_L1( "WIFI_CONNECT_CHECK PROCESS = %d", check_count);
						//							MAINLOG_L1("START REBOOT");
						wifiReboot();
						//							MAINLOG_L1("START REBOOT");
						goto WIFI_CONNECT_CHECK;
					}
				}
			}
			else
			{
				AppPlayTip("Read Wifi configuration failed, please config Wifi again");
				WifiConfig();
			}
		}
	}
}

void EnableGPRS()
{
	int ret = NetModuleOper_Api(GPRS, 1);
	int current_connection_type = 0;

	if (ret < 0 || ret == 2)
	{
		MAINLOG_L1("!!! NetModuleOper_Api(GPRS, 1) failed(%d) !!!", ret);
		return;
	}

	ret = NetLinkCheck_Api(GPRS);
	// GPRS is ready
	if (ret != 2 && ret != 1)
	{
		_IS_GPRS_READY_ = 1;
		current_connection_type = 1;
	}
	else
	{
		_IS_GPRS_READY_ = 0;

		// Handle no SIM case
		if (ret == 2)
		{
			static int no_sim_play_count = 0;
			no_sim_play_count++;

			if (no_sim_play_count == 1)
			{
				AppPlayTip("Please insert sim card and restart device");
			}

			if (no_sim_play_count >= 2)
			{
				AppPlayTip("No sim card, power off");
				SysPowerOff_Api();
			}
		}
	}

	AppPlayTip("Mobile network connected");
	_IS_GPRS_ENABLED_ = 1;
	MAINLOG_L1("!!! GPRS ENABLED(%d) !!!", ret);

	// CLOSE THE WIFI
	_IS_WIFI_ENABLED_ = 0;
}

void TimeOutChoiseConnection()
{
	AppPlayTip("Network not connected. Smart Pay Shutdown");
	SysPowerOff_Api();
}

void ResetWifi()
{
	int ret = wifiReset_lib();
	if(ret == 0)
	{
		AppPlayTip("Wifi Reset. Smart Pay Reboot");
		SysPowerReBoot_Api();
	}
}

void SelectedNetwork()
{
	int nSelcItem = 1, ret;
	char *pszTitle = "Network Connection";
	const char *pszItems[] = {"1. Wifi Connection", "2. 4G SIM Internet"};
	while (1)
	{
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
								 sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL2, 0, 60);
		switch (nSelcItem)
		{
		case DIGITAL1:
			EnableWIFI();
			return;
		case DIGITAL2:
			EnableGPRS();
			return;
		case ESC:
			return;
		default:
			break;
		}
	}
}

void DisplayChoiceNetwork()
{
	int nSelcItem = 1, ret;
	char *pszTitle = "Select Network";
	const char *pszItems[] = {"", "1. Wifi Connection", "2. 4G SIM Internet"};
	while (1)
	{
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
								 sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL2, 0, 60);
		switch (nSelcItem)
		{
		case DIGITAL1:
			EnableWIFI();
			break;
		case DIGITAL2:
			EnableGPRS();
			return;
		case -2:
			TimeOutChoiseConnection();
			break;
		default:
			break;
		}
	}
	return;
}

void InitConnection()
{
	// USING INTERNENT DEVELOP
//	EnableGPRS();

	int ret;
	// CHECKING SMART PAY ALREADYS HAVE WIFI PARAM
	ret = checkingWifiActivate();
	MAINLOG_L1("checkingWifiActivate %d", ret);
	// NO WIFI PARAM CHECK USER TO CHOSES NETWORK CONNECTION
	if (ret != 0)
	{
		AppPlayTip("Please select network.");
		DisplayChoiceNetwork();
	}
	// AUTO CONNECT TO THE WIFI
	else
	{
		EnableWIFI();
	}
}

int checkInternetStatusold()
{
	static int show_net_status_count = 0; // Persistent count across function calls
	static int signal_lost_count = 0;
	static int wifi_reconnect_count = 0;
	static int no_sim_play_count = 0;

	int ret;
	int mobile_network_registered = 0;

	// Check GPRS
	if (_IS_GPRS_ENABLED_)
	{
		ret = NetLinkCheck_Api(GPRS);

		if (show_net_status_count % 6 == 0)
		{
			MAINLOG_L1("----- GPRS_STATUS: [%d] -----", ret);
		}

		if (ret == 2)
		{ // No SIM detected
			_IS_GPRS_READY_ = 0;
			no_sim_play_count++;

			if (no_sim_play_count == 1)
			{
				AppPlayTip("Please insert sim card and restart device");
			}

			if (no_sim_play_count >= 6)
			{
				AppPlayTip("No sim card, power off");
				SysPowerOff_Api();
			}
		}
		else if (ret == 1)
		{ // Network registration in progress
			mobile_network_registered = 0;
			signal_lost_count++;

			if (signal_lost_count == 1)
			{
				AppPlayTip("Mobile network register in progress");
			}

			if (signal_lost_count >= 6)
			{
				_IS_GPRS_READY_ = 0;
				AppPlayTip("Cannot register mobile network, please restart device");
			}
		}
		else
		{ // GPRS Ready
			signal_lost_count = 0;
			mobile_network_registered = 1;
			_IS_WIFI_READY_ = 0;
			_IS_GPRS_READY_ = 1;
		}
	}

	// Check WiFi
	if (_IS_WIFI_ENABLED_)
	{
		fibo_sem_wait(GsemaRef);
		ret = wifiGetLinkStatus_lib();
		fibo_sem_signal(GsemaRef);

		if (show_net_status_count % 6 == 0)
		{
			MAINLOG_L1("----- WIFI_STATUS: [%d] -----", ret);
		}

		if (ret == 4 || ret == 5)
		{ // TCP/UDP disconnect or AP not connected
			wifi_reconnect_count++;

			if (wifi_reconnect_count < 3)
			{
				MAINLOG_L1("!!! WIFI Disconnect or AP Exception, Reconnecting (%d) !!!", wifi_reconnect_count);
				ret = wifiClose_lib();
				if (ret != 0)
					MAINLOG_L1("!!! wifiClose_lib() failed (%d) !!!", ret);

				Delay_Api(100);

				ret = wifiOpen_lib();
				if (ret != 0)
				{
					MAINLOG_L1("!!! wifiOpen_lib() failed (%d) !!!", ret);
					AppPlayTip("Your wifi error, Smart pay will reboot");
				}
			}
			else
			{
				_IS_WIFI_READY_ = 0;
			}
		}
		else if (ret == -6300)
		{ // WiFi module not open
			ret = wifiOpen_lib();
			if (ret != 0)
			{
				_IS_WIFI_READY_ = 0;
				MAINLOG_L1("!!! wifiOpen_lib() failed (%d) !!!", ret);
			}
		}
		else if (ret == 0 || ret == -6302)
		{ // Status check in progress
			if (show_net_status_count % 6 == 0)
			{
				AppPlayTip("Check Wifi Status in Progress...");
			}
		}
		else
		{ // WiFi Ready
			_IS_GPRS_READY_ = 0;
			_IS_WIFI_READY_ = 1;
			wifi_reconnect_count = 0;
		}
	}

	if (show_net_status_count % 6 == 0)
	{
		char info[64];
		snprintf(info, sizeof(info), "[IS_GPRS_READY | IS_WIFI_READY] = [%d | %d]", _IS_GPRS_READY_, _IS_WIFI_READY_);
		MAINLOG_L1(info);
	}

	show_net_status_count++;

	return (_IS_GPRS_READY_ || _IS_WIFI_READY_) ? 1 : 0;
}

int checkInternetStatus()
{
	static int last_connection_type = 0; // 0=none, 1=GPRS, 2=WiFi
	int current_connection_type = 0;
	int ret;

	// Check GPRS first if enabled
	if (_IS_GPRS_ENABLED_)
	{
		ret = NetLinkCheck_Api(GPRS);

		// GPRS is ready
		if (ret != 2 && ret != 1)
		{
			_IS_GPRS_READY_ = 1;
			current_connection_type = 1;
		}
		else
		{
			_IS_GPRS_READY_ = 0;

			// Handle no SIM case
			if (ret == 2)
			{
				static int no_sim_play_count = 0;
				no_sim_play_count++;

				if (no_sim_play_count == 1)
				{
					AppPlayTip("Please insert sim card and restart device");
				}

				if (no_sim_play_count >= 6)
				{
					AppPlayTip("No sim card, power off");
					SysPowerOff_Api();
				}
			}
		}
	}

	// Then check WiFi if enabled
	if (_IS_WIFI_ENABLED_ && current_connection_type == 0)
	{
		fibo_sem_wait(GsemaRef);
		ret = wifiGetLinkStatus_lib();
		fibo_sem_signal(GsemaRef);

		// WiFi is connected
		if (ret != 4 && ret != 5 && ret != -6300 && ret != 0 && ret != -6302)
		{
			_IS_WIFI_READY_ = 1;
			current_connection_type = 2;
		}
		else
		{
			_IS_WIFI_READY_ = 0;

			// Handle WiFi reconnection
			if (ret == 4 || ret == 5)
			{
				static int wifi_reconnect_count = 0;
				wifi_reconnect_count++;

				if (wifi_reconnect_count < 3)
				{
					MAINLOG_L1("!!! WIFI Disconnect or AP Exception, Reconnecting (%d) !!!", wifi_reconnect_count);
					wifiClose_lib();
					Delay_Api(100);
					ret = wifiOpen_lib();
					if (ret != 0)
					{
						MAINLOG_L1("!!! wifiOpen_lib() failed (%d) !!!", ret);
						AppPlayTip("Your wifi error, Smart pay will reboot");
					}
				}
			}
		}
	}

	// Check if connection type has changed
	if (current_connection_type != last_connection_type)
	{
		MAINLOG_L1("Connection changed from %d to %d", last_connection_type, current_connection_type);
		last_connection_type = current_connection_type;

		// Force MQTT reconnection by returning 0 once
		return 0;
	}

	// Log status periodically (reduced frequency)
	static int show_net_status_count = 0;
	if (show_net_status_count++ % 12 == 0)
	{
		MAINLOG_L1("[IS_GPRS_READY | IS_WIFI_READY] = [%d | %d]", _IS_GPRS_READY_, _IS_WIFI_READY_);
	}

	return (_IS_GPRS_READY_ || _IS_WIFI_READY_) ? 1 : 0;
}
