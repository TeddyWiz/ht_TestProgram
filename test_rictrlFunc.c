#include <stdio.h>
#include <string.h>

#define uint8 	unsigned char
#define uint16	unsigned chort
#define BOOL	unsigned char

#define RI_CTRL_MAX_THRESHOLD 3 // 최종 반영 횟수
#define RI_CTRL_DEC_SIG_QUAL 105 // 보고주기 감소 기준 값
#define RI_CTRL_INC_SIG_QUAL 110 // 보고주기 증가 기준 값
#define FALSE 0
#define TRUE 1

typedef struct {
	uint8 meterType;
	uint8 dataSkipMode;
	uint8 sleepMode;
	uint8 riCtrlMode; // 0 : no ctrl, 1 : ctrl
	uint8 riCtrlChgCount;
	uint8 riCtrlValue;
	uint8 isShortInterval;
	uint8 intervalBaseTime;
	uint8 meterInterval;
	uint8 reportInterval;
	uint8 reportRange;
	uint8 reportMin;
	uint8 reportSec;
	uint8 debugPrint;
	uint8 termModel;
	uint8 bslModel;

} Config_t;

Config_t conf;


void ctrlReportInterval(int sigQual)
{
	char incNeed = FALSE;
	if (sigQual >= RI_CTRL_INC_SIG_QUAL) {
		conf.riCtrlChgCount++;
		incNeed = TRUE;
	} else {
		conf.riCtrlChgCount = 0;
	}
	if (conf.riCtrlChgCount >= RI_CTRL_MAX_THRESHOLD) {
		//printf("over count %d\r\n", incNeed);
		conf.riCtrlChgCount = 0;
		if (incNeed) {
			conf.riCtrlValue *= 2;
			//printf("conf.riCtrlValue=%2d\n", conf.riCtrlValue);
			if (conf.riCtrlValue > 24)
				conf.riCtrlValue = 24;
		} else {
			conf.riCtrlValue /= 2;
			if (conf.riCtrlValue < conf.reportInterval)
				conf.riCtrlValue = conf.reportInterval;
		}
	}
}

void ctrlReportInterval1(int sigQual)
{
	BOOL isIncCtrl = (conf.riCtrlValue > conf.reportInterval) ? TRUE : FALSE;
	if (sigQual <= RI_CTRL_DEC_SIG_QUAL) {
		conf.riCtrlChgCount = (isIncCtrl) ? (conf.riCtrlChgCount + 1) : 0;
	} else if (sigQual >= RI_CTRL_INC_SIG_QUAL) {
		conf.riCtrlChgCount = (isIncCtrl) ? 0 : (conf.riCtrlChgCount + 1);
	} else {
		conf.riCtrlChgCount = 0;
	}
	if (conf.riCtrlChgCount >= RI_CTRL_MAX_THRESHOLD) {
		conf.riCtrlChgCount = 0;
		if (isIncCtrl) {
			conf.riCtrlValue = conf.reportInterval;
		} else {
			conf.riCtrlValue = conf.reportInterval * 2;
			if (conf.riCtrlValue > 24)
				conf.riCtrlValue = 24;
		}
	}
}
int main(void)
{
	int i=0;
	conf.riCtrlChgCount = 0;
	conf.reportInterval = 6;
	conf.riCtrlValue = 6;

	//               0   1   2   3    4    5    6    7    8   9 
	int s_data[20]={90, 95, 97, 105, 110, 112, 115, 117, 118, 119,\
					115, 116, 117, 95, 96, 94, 100, 101, 102, 101};
	
	for(i=0; i<20; i++)
	{
		//ctrlReportInterval(s_data[i]);
		ctrlReportInterval1(s_data[i]);
		printf("%2d:[%3d] count:%2d, ri:%2d\r\n", i, s_data[i], conf.riCtrlChgCount, conf.riCtrlValue);
	}
	return 0;
}
