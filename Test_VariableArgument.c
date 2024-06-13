#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>


int printf_ts(const char *format, ...)
{
#define BUF_SIZE 0x200 //DEBUG_TX_BUF_LEN

	//if (IsPrintOn) {
		unsigned int len = 0;
		char buf[BUF_SIZE];
		//Date_t date;

		//RTC_read(&date);
		time_t now = time(NULL);
        struct tm *local = localtime(&now);
        #if 0
		snprintf(buf, 30, "[%04d-%02d-%02d %02d:%02d:%02d]", date.year, date.mon, date.day,
			 date.hour, date.min, date.sec);
		len = strlen(buf);
		#endif
		len = snprintf(buf, 30, "[%04d-%02d-%02d %02d:%02d:%02d]", local->tm_year + 1900, local->tm_mon + 1, local->tm_mday, \
			 local->tm_hour, local->tm_min,  local->tm_sec);

		va_list arg;
		va_start(arg, format);
		// Console 출력 시 New line을 위한 1byte 고려.
		len += vsnprintf(buf + len, BUF_SIZE - 1, format, arg);
		va_end(arg);

		if (buf[len - 1] == '\n') {
			buf[len++] = '\r';
		}
		printf("[%d][%s]",len, buf);
#if 0
		if (conf.debugPrint == 2) {
			UART_send(UART_A3, buf, len, FALSE);
		} else {
			UART_send(UART_A1, buf, len, FALSE);
		}
		#endif
//	}

	return 0;
}
#define TEST "test1"
#define FINISH "finish"
#define SP " "
#define LF_D "\r\n"

int main() {
    // Write C code here
    int cnt = 20;
    char testStr[] = "hello";
    // Write C code here
    printf(TEST SP FINISH" %d : %s " LF_D, cnt, testStr);
    printf_ts(TEST SP FINISH" %d : %s " LF_D, cnt, testStr);
    return 0;
}