#include <MQTTClient.h>
#include <time.h>
#include "def.h"

long currentTimeStr;
long time_unix(){
	time_t currentTime;
	// Get the current system time
	currentTime = time(NULL);
	// Sufficient size to hold the string representation of currentTime
	sprintf(currentTimeStr, "%ld", (long) currentTime);
	return currentTimeStr;
}

char* getCurrentTimeMs() {
    static char timeout_str[20]; // Increased buffer size for safety
    struct timeval tv;

	time_t cur_time;
	cur_time = time(NULL);
	sprintf(timeout_str, "%ld", (long)cur_time);

    gettimeofday(&tv, NULL);
    long long timeout_ms = ((long long)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    snprintf(timeout_str, sizeof(timeout_str), "%lld", timeout_ms);

    return timeout_str;
}

char* getTimeOutASecound() {
    static char timeout_str[20];
	time_t current_time = time(NULL);
	long long timeout_ms = ((long long)current_time + 60LL) * 1000LL;  // Add 1 hour and convert to ms

	snprintf(timeout_str, sizeof(timeout_str), "%lld", timeout_ms);

	return timeout_str;
}

char* setTimeOutAMn() {
    static char timeout_str[20];
	time_t current_time = time(NULL);
	long long timeout_ms = ((long long)current_time) * 1000LL;

	timeout_ms += 60 * 1000;

	snprintf(timeout_str, sizeof(timeout_str), "%lld", timeout_ms);

	return timeout_str;
}
