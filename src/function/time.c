#include <MQTTClient.h>

long currentTimeStr;
long time_unix(){
	time_t currentTime;
	// Get the current system time
	currentTime = time(NULL);
	// Sufficient size to hold the string representation of currentTime
	sprintf(currentTimeStr, "%ld", (long) currentTime);
	return currentTimeStr;
}

char* getTimeOutASecound() {
    static char timeout_str[14];  // 13 digits + 1 null terminator
	time_t current_time = time(NULL);
	long long timeout_ms = ((long long)current_time + 60LL) * 1000LL;  // Add 1 hour and convert to ms

	snprintf(timeout_str, sizeof(timeout_str), "%lld", timeout_ms);

	return timeout_str;
}

// char* getTimeOutAHour() {
//     static char timeout_str[14];  // 13 digits + 1 null terminator
//     time_t current_time = time(NULL);
//     long long timeout_ms = ((long long)current_time + 3600LL) * 1000LL;  // Add 1 hour and convert to ms

//     snprintf(timeout_str, sizeof(timeout_str), "%lld", timeout_ms);

//     return timeout_str;
// }
