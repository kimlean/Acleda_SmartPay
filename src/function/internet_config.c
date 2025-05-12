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

    // Try to enable WiFi module
    ret = NetModuleOper_Api(WIFI, 1);
    if (ret != 0)
    {
        MAINLOG_L1("!!! NetModuleOper_Api(WIFI, 1) failed(%d) !!!", ret);
        AppPlayTip("WiFi module error");
        return;
    }

    // Open WiFi interface
    ret = wifiOpen_lib();
    if (ret != 0)
    {
        MAINLOG_L1("!!! wifiOpen_lib() failed(%d) !!!", ret);
        AppPlayTip("WiFi interface error");
        return;
    }

    // Check if we have WiFi configuration
    int file_size = GetFileSize_Api(WIFI_CONFIG_FILE);
    if (file_size == 0)
    {
        AppPlayTip("Please config WiFi");
        ConfigWifi();
    }
    else
    {
        char buf[2] = {0};  // Fixed: char buf[2] should be a char array, not a pointer array
        ret = ReadFile_Api(WIFI_CONFIG_FILE, buf, 0, &file_size);

        if (ret != 0)
        {
            MAINLOG_L1("!!! ReadFile_Api('wifi_config.dat') failed(%d) !!!", ret);
            AppPlayTip("Read WiFi configuration failed, please config WiFi again");
            ConfigWifi();
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
					Delay_Api(3000);
					ret = wifiGetLinkStatus_lib();
					MAINLOG_L1("WIFI_LINK_STATUS = %d", ret);
					if (ret == 2)
					{
						_IS_WIFI_ENABLED_ = 1;
						_IS_WIFI_READY_ = 1;  // Added this to indicate WiFi is ready

						// CLOSE THE INTERNET
						_IS_GPRS_ENABLED_ = 0;
						_IS_GPRS_READY_ = 0;  // Added this to ensure GPRS is marked as not ready

						AppPlayTip("WiFi connected");
					}
					else
					{
						if (check_count >= 2)
						{
							AppPlayTip("Cannot connect to your WiFi");
							_IS_WIFI_ENABLED_ = 0;
							_IS_WIFI_READY_ = 0;  // Added this to ensure WiFi is marked as not ready

							MAINLOG_L1("Falling back to GPRS after WiFi failure");
							EnableGPRS();
							return;
						}
						else
						{
							check_count++;
							MAINLOG_L1("Rebooting WiFi connection, attempt %d", check_count);
							wifiReboot();
							goto WIFI_CONNECT_CHECK;
						}
					}
            }
            else
            {
                AppPlayTip("Invalid WiFi configuration, please reconfigure");
                ConfigWifi();
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

        // Only play the success message if we actually connected
        AppPlayTip("Mobile network connected");
        _IS_GPRS_ENABLED_ = 1;
        MAINLOG_L1("!!! GPRS ENABLED(%d) !!!", ret);

        // CLOSE THE WIFI
        _IS_WIFI_ENABLED_ = 0;
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
        else if (ret == 1)
        {
            // Network registration in progress
            AppPlayTip("Mobile network registering");
            MAINLOG_L1("!!! GPRS REGISTRATION IN PROGRESS !!!");
        }
    }
}

void TimeOutChoiseConnection()
{
	AppPlayTip("Network not connected. Smart Pay Shutdown");
	SysPowerOff_Api();
}

void ResetWifi()
{
	ScrCls_Api();
	ScrDisp_Api(LINE5, 0, "Please wait", CDISP);

	int ret = wifiClose_lib();
	MAINLOG_L1("WI-FI CLOSE STATUS %d", ret);
	ret = wifiReset_lib();
	MAINLOG_L1("WI-FI RESET STATUS %d", ret);
//	ret = wifiRestore_lib();
//	MAINLOG_L1("WI-FI RESTORE STATUS %d", ret);
	if(ret == 0)
	{
		// DELETE WIFI PASSWORD
		DelFile_Api(WIFI_CONFIG_FILE);
		DelFile_Api(WIFI_PARAM_FILE);

		// DELETE INTERNET CONFIG FILE
		DelFile_Api(INTERNET_CONFIG_FILE);

		AppPlayTip("Wifi Reset. Smart Pay Reboot");
		ScrCls_Api();
		ScrDisp_Api(LINE5, 0, "Wifi Reset", CDISP);
		ScrDisp_Api(LINE6, 0, "Smart Pay Reboot", CDISP);
		SysPowerReBoot_Api();
	}
}

void SelectedWIFIConnection()
{
	ScrCls_Api();
	ScrClsRam_Api();
	ScrDispRam_Api(5, 0, "WiFi Connecting", CDISP);
	ScrDispRam_Api(6, 0, "Please Wait...", CDISP);

	EnableWIFI();
	saveInternetConnectionType(2);
	return;
}

void SelectedGPRS()
{
	ScrCls_Api();
	ScrClsRam_Api();
	ScrDispRam_Api(5, 0, "Sim Connecting", CDISP);
	ScrDispRam_Api(6, 0, "Please Wait...", CDISP);

	EnableGPRS();
	saveInternetConnectionType(1);
	return;
}

void SelectedNetwork()
{
	int nSelcItem = 1, ret;
	char *pszTitle = "Network Connection";
	const char *pszItems[] = {"1.Wifi 2.4GHz", "2.4G SIM Internet"};
	while (1)
	{
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
								 sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL2, 0, 60);
		switch (nSelcItem)
		{
		case DIGITAL1:
			SelectedWIFIConnection();
			return;
		case DIGITAL2:
			SelectedGPRS();
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
	const char *pszItems[] = {"", "1.Wifi Connect 2.4GHz", "2.4G SIM Internet"};
	while (1)
	{
		// DISPLAY THE PARM MENU AND RETURN THE KEY CLICK AND TIME TO CLOSE
		nSelcItem = ShowMenuItem(pszTitle, pszItems,
								 sizeof(pszItems) / sizeof(char *), DIGITAL1, DIGITAL2, 0, 60);
		switch (nSelcItem)
		{
			case DIGITAL1:
				saveInternetConnectionType(2);
				ConfigWifi();
				return;
			case DIGITAL2:
				EnableGPRS();
				saveInternetConnectionType(1);
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

void InitConnection() {
    int connectionType = getInternetConnectionType();
    int ret;

    MAINLOG_L1("InitConnection: Saved connection type: %d", connectionType);

    // If WiFi was the last selected connection
    if (connectionType == 2) {
        // Check if WiFi is configured
        ret = checkingWifiActivate();
        MAINLOG_L1("checkingWifiActivate %d", ret);

        if (ret == 0) {
            // WiFi is configured, try to connect
            EnableWIFI();
            return;
        }
    }
    // If GPRS was the last selected connection
    else if (connectionType == 1) {
        // Try to connect using GPRS
        EnableGPRS();
        return;
    }

    // No valid saved preference or connection failed, ask user to choose
    AppPlayTip("Please select network.");
    DisplayChoiceNetwork();
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

	    // Track WiFi connection state to detect reconnection
	    static int previous_wifi_state = 1; // 0=disconnected, 1=connected

	    // WiFi is connected
	    if (ret != 4 && ret != 5 && ret != -6300 && ret != 0 && ret != -6302)
	    {
	        _IS_WIFI_READY_ = 1;
	        current_connection_type = 2;

	        // If we were previously disconnected and now reconnected, show alert
	        if (previous_wifi_state == 0)
	        {
	            // Display reconnection message
	            ScrClrLineRam_Api(10, 11);  // Clear previous connection error messages
	            ScrDispRam_Api(10, 0, "WI-FI Reconnected", CDISP);
	            ScrBrush_Api();

	            // Play reconnection sound
	            AppPlayTip("WiFi connection restored");

	            // Log the event
	            MAINLOG_L1("!!! WIFI Reconnected Successfully !!!");

	            // After 2 seconds, clear the reconnection message
	            Delay_Api(3000);
	            ScrClrLineRam_Api(10, 11);
	            ScrBrush_Api();

	            g_RefreshQRDisplay = 1;
	        }

	        previous_wifi_state = 1; // Mark as connected
	    }
	    else
	    {
	    	_IS_WIFI_READY_ = 0;

	        // Only show disconnection message when state changes from connected to disconnected
	        if (previous_wifi_state == 1)
	        {
	            MAINLOG_L1("!!! WIFI Connection Lost !!!");
	        }

	        previous_wifi_state = 0; // Mark as disconnected

	        // Handle WiFi reconnection
	        if (ret == 4 || ret == 5)
	        {
	            ScrDispRam_Api(10, 0, "WI-FI Lost", CDISP);
	            ScrDispRam_Api(11, 0, "Try Reconnect ...", CDISP);
	            ScrBrush_Api();

	            // Always attempt reconnection, no limit on count
	            static int wifi_reconnect_count = 0;
	            wifi_reconnect_count++;

	            // Always attempt to reconnect WiFi when it's disconnected
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
