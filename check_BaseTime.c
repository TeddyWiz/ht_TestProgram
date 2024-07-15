#include<stdio.h>
#include<string.h>

#define MIN_DIFF    5

#define KT 0
#define GSM 1
#define LG  2
int main(int argc, char *argv[])
{
	int i=0, j=0;
	int serialBase=0, nSpread=0;
	int reportRange = 6;
	unsigned char intervalBaseTime =0, reportMin = 0, reportSec =0;
	unsigned char type = KT;
	char *typestr[3]={"KT", "GSM", "LG"};
	for(j=0; j<3;j++) {
		type = j;
		printf("===== %s DistributingReportTime ========\n", typestr[type]);
		printf(" ReportRange : %2d\n", reportRange);
		printf("num  \tserial\tbase\treMin\treSec\n");
		for(i=1; i<301; i++)
		{
			serialBase = i;
			switch(type)
			{
				case KT:
					nSpread = reportRange * (60/MIN_DIFF);
					serialBase %= nSpread;
					intervalBaseTime = serialBase % reportRange;
					reportMin = ((serialBase / reportRange)*MIN_DIFF) % 60;
					reportSec = 0;
					break;
				case GSM:
					nSpread = reportRange * 50;
					reportSec = ((serialBase / nSpread) % 4) * 15;
					serialBase %= nSpread;
					intervalBaseTime = serialBase / 50;
					reportMin = (serialBase % 50) + 5;
				break;
				case LG:
					nSpread = reportRange * 50;
					reportSec = ((serialBase / nSpread) % 4) * 15;
					serialBase %= nSpread;
					intervalBaseTime = serialBase / 50;
					reportMin = (serialBase % 50) + 5;
				break;
			}
			
			printf("%3d :\t%2d\t%2d\t%2d\t%2d\n", i, serialBase, intervalBaseTime, reportMin, reportSec);
		}
	}
	return 0;
}
