#include <stdio.h>
#include <string.h>
#define PATTERN_CCLK "+CCLK:"


int main(int argc, char *argv[])
{
	char modem_date[12];
	char date_msg[]="+CCLK:24/06/20,10:31:47+36";
	char *p= NULL, i;
	

	if (p = strstr(date_msg, PATTERN_CCLK)) {
		printf("1: %s\n", p);
		p += strlen(PATTERN_CCLK);
		for(i=0;i<12;i++)
		{
			modem_date[i]=*p++;
			if(((i+1)%2)==0)
			{
				p++;
			}
		}
		modem_date[i] = 0;
		printf("date=%s\n", modem_date);
	}
	printf("end test\n");
	return 0;
}
