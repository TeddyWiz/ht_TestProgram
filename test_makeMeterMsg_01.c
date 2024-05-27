#include <stdio.h>
#include <string.h>

#define uchar 	unsigned char
#define uint16	unsigned short
#define uint8	unsigned char
#define uint32	unsigned long
#define MAX_NUM_STORED_DATA 24
#define BOOL 	unsigned char
#define FALSE 	0
#define TRUE 	1


// APP Process
#define APP_IDLE 0
#define APP_INITIAL_REPORT 1
#define APP_PERIODIC_REPORT 2
#define APP_IMMEDIATE_REPORT 3
#define APP_SET_CONFIGURATION 4

typedef struct {
	uchar imei[8];
	uchar imsi[8];
} MobileId_t;

typedef struct {
	uchar rssi;
	uchar ber;
	uchar cid[2];
	uchar rsrp[2];
	uchar rsrq[2];
	uchar snr[2];
} NbiotRadioQuality_t;

typedef struct {
	uchar rssi;
	uchar ber;
	uchar cid[4];
	uchar rsrp[2];
	uchar rsrq[2];
	uchar snr[2];
} NbiotRadioQuality1_t; //seoul protocol version 1.7


typedef struct {
	uchar termSerial[5];
	uchar fwVer[2];
	uchar termBatt;
} NbiotDeviceInfo_t;

typedef struct {
	uchar meterSerial[4];
	uchar meterType;
	uchar meterCaliber_dp;
	uchar meterStatus;
} NbiotMeterInfo_t;

typedef struct {
	uchar mi;
	uchar ri;
} NbiotMeteringParam_t;

typedef struct {
	uchar year;
	uchar mon;
	uchar day;
	uchar hour;
	uchar min;
	uchar sec;
} NbiotTimeStamp_t;

typedef struct {
	uchar interval;
	uchar numData;
	uchar refValuePos;
	uchar refValue[4];
	uchar valueDiff[MAX_NUM_STORED_DATA][2];
} NbiotMeterData_t;

typedef struct {
	uchar interval;
	uchar numData;
	uchar refValuePos;
	//uchar refValue[4];
	uchar valueDiff[MAX_NUM_STORED_DATA][4];
} NbiotMeterData1_t; //seoul protocol version 1.7


// EC_RUDY209
typedef struct {
    uchar valueDiff[2];
    uchar temperature[2];
    uchar pressure[2];
} NbiotMeterValueDiffExt_t;

typedef struct {
	uchar interval;
	uchar numData;
	uchar refValuePos;
	uchar refValue[4];
	NbiotMeterValueDiffExt_t valueDiffExt[MAX_NUM_STORED_DATA];
} NbiotMeterDataExt_t;
// end of EC_RUDY209


typedef struct {
	uchar resetCause;
	uchar resetCount[2];
} NbiotReset_t;


//teddy 240509
typedef struct {
	uchar protocol;
	uchar len;
	uchar mtype;
} NbiotMsgHeader_t;

typedef struct {
	uchar protocol;
	uchar len;
	uchar mtype;
	uchar ip[4];
	uchar port[2];
	uchar fwVer[4];
	uchar checksum;
} NbiotCmdFotaReq_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	uchar mi; // 1  - 4
	uchar ri; // 1  - 5
	uchar checksum; // 1  - 6
} NbiotIntervalReq_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	uchar range; // 1  - 4
	uchar checksum; // 1  - 5
} NbiotRepRange_t;

// EC_RUDY203
typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	uchar riMinute; // 1  - 4
	uchar checksum; // 1  - 5
} NbiotIntervalMinuteReq_t;
// end of EC_RUDY203

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	uchar checksum; // 1  - 5
} NbiotAck_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	MobileId_t id; // 16 - 19
	NbiotRadioQuality_t radio; // 10 - 29
	NbiotDeviceInfo_t term; // 8  - 37
	NbiotMeterInfo_t meter; // 7  - 44
	NbiotMeteringParam_t mp; // 2  - 46
	NbiotTimeStamp_t meterTime; // 6  - 52
	NbiotMeterData_t data; // 7 + 2*nData --> 61 ~ 107
	uchar checksum; // 1  - 108
} NbiotDataReport_t;

// EC_RUDY203
typedef struct {
	uchar meterSerial[4];
	uchar meterType;
	uchar meterCaliber_dp;
	uchar meterStatus;
    uchar temperature[2];
    uchar pressure[2];
} NbiotMeterInfoExt_t;

typedef struct {
	uchar miMinute[2];
	uchar riMinute[2];
} NbiotMeteringParamExt_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	MobileId_t id; // 16 - 19
	NbiotRadioQuality_t radio; // 10 - 29
	NbiotDeviceInfo_t term; // 8  - 37
	NbiotMeterInfoExt_t meterExt; // 11  - 48
	NbiotMeteringParamExt_t mpExt; // 4  - 52
	NbiotTimeStamp_t meterTime; // 6  - 58
	NbiotMeterDataExt_t data; // 7 + 6*nData --> 71 ~ 209
	uchar checksum; // 1  - 210
} NbiotDataReportExt_t;


typedef struct {
	uint16 year; // yearH, yearL
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 reserved;
} Date_t;


typedef struct {
	uint8 battery : 2, // terminal battery
		lowBatt : 1, // meter battery
		fArrow : 1, rArrow : 1, m3 : 1, notUsed : 1, leak : 1;
} LcdIcon_t;

typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 isTimeSync;
	uint8 meterData[4];
	uint8 meterStatus;
	uint8 st[2];
	uint8 rt[2];
	uint8 flowData[4];
	LcdIcon_t icon;
    uint8 temperature[2];
    uint8 pressure[2];
} MeterUnitData_t;


typedef struct {
	uint8 nData;
	uint8 saveInterval;
	uint8 caliberDp;
	uint8 dif;
	uint8 vif;
	uint8 meterSerial[4];
	MeterUnitData_t unit[MAX_NUM_STORED_DATA];
	MeterUnitData_t lastDiffUnit;
} MeterStoredData_t;

typedef struct {
	uint8 serialNum[12 + 1];
	uint8 meterType;
	char serviceCode[5]; // using only LG platfom. others all '0'
	char serverIp[16];
	uint16 serverPort;
	char fotaIp[16]; // using for FOTA. It used when not use LG platform.
	uint16 fotaPort; // using for FOTA. It used when not use LG platform.
	uint8 fotaInterval; // using for FOTA. It used when not use LG platform.
	uint8 imei[8];
	uint8 imsi[8];
	uint8 isModemInit;
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
	uint16 resetCount;
	uint8 debugPrint;
	uint8 termModel;
	uint8 bslModel;

// EC_RUDY204
    uint8 extMode;
// end of EC_RUDY204
#if 1 //teddy 24050
	uint8 meterProtoVer;
#endif

	uint8 checksum;
} Config_t;

Config_t conf;

static uchar cal_checksum(uchar *p, int len)
{
	uchar checksum = 0;

	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}

	return checksum;
}

uint8 ascii2Hex(char ch)
{
	uint8 hex = 0;

	if (ch >= 'A' && ch <= 'F') {
		hex = (ch - 'A') + 10;
	} else if (ch >= 'a' & ch <= 'f') {
		hex = (ch - 'a') + 10;
	} else if (ch >= '0' & ch <= '9') {
		hex = ch - '0';
	}
	return hex;
}

uint8 ascii2BCD(char a, char b)
{
	return (ascii2Hex(a) * 0x10 + ascii2Hex(b));
}

static void str2BcdForImeiAndImsi(uchar *pBCD, char *pString)
{
	for (int i = 0; i < 7; i++) {
		*(pBCD + i) = ascii2BCD(*(pString + i * 2 + 0), *(pString + i * 2 + 1));
	}

	*(pBCD + 7) = ascii2BCD(*(pString + 14), 'F');
}
uchar modemIMEI[16] = "866416049616841F";
uchar modemIMSI[16] = "450061235190642F";
void MODEM_copyImei(uchar *imei)
{
	str2BcdForImeiAndImsi(imei, modemIMEI);
}
void MODEM_copyImsi(uchar *imsi)
{
	str2BcdForImeiAndImsi(imsi, modemIMSI);
}

static int insert_id(MobileId_t *p)
{
	MODEM_copyImei(p->imei);
	MODEM_copyImsi(p->imsi);

	return sizeof(MobileId_t);
}


static int insert_termInfo(NbiotDeviceInfo_t *p)
{
	for (uchar i = 0; i < 12 / 2; i++) {
		uchar idx1 = (2 + i * 2 + 0);
		uchar idx2 = (2 + i * 2 + 1);
		p->termSerial[i] = ascii2BCD(conf.serialNum[idx1], conf.serialNum[idx2]);
	}

	// f/w version
	char fw_ver[0x10] = "";
	strcpy(fw_ver, "U331");
	fw_ver[0] = '0';

	p->fwVer[0] = ascii2BCD(fw_ver[0], fw_ver[1]);
	p->fwVer[1] = ascii2BCD(fw_ver[2], fw_ver[3]);

#define LOW_BATTERY_THRESHOLD 31
	p->termBatt = 0x31;//BATT_getVoltage();
	if (p->termBatt <= LOW_BATTERY_THRESHOLD) {
		p->termBatt |= 0x80;
	}

	return sizeof(NbiotDeviceInfo_t);
}

static int insert_radioQuality(NbiotRadioQuality_t *p)
{
	#if 0
	int rssi = MODEM_getLastRssi();
	int rsrp = MODEM_getLastRsrp();
	int rsrq = MODEM_getLastRsrq();
	int snr = MODEM_getLastSNR();

	p->rssi = rssi;
	p->ber = 0; // BER - 사용하지 않음.
	memcpy(p->cid, &modem.modemQuality.cid, 2);
	memcpy(p->rsrp, &rsrp, 2);
	memcpy(p->rsrq, &rsrq, 2);
	memcpy(p->snr, &snr, 2);

	return sizeof(NbiotRadioQuality_t);
	#endif
	int tempdata = 0;
	p->rssi = 101;
	p->ber = 102;
	tempdata = 103;
	printf("p->cid = %d\r\n", tempdata);
	memcpy(p->cid, &tempdata, 2);
	tempdata = 104;
	printf("p->rsrp = %d\r\n", tempdata);
	memcpy(p->rsrp, &tempdata, 2);
	tempdata = 105;
	printf("p->rsrq = %d\r\n", tempdata);
	memcpy(p->rsrq, &tempdata, 2);
	tempdata = 106;
	printf("p->snr = %d\r\n", tempdata);
	memcpy(p->snr, &tempdata, 2);
	printf("p->rssi = %d, p->ber = %d, p->cid = %02x%02x\r\n",p->rssi, p->ber, p->cid[0],  p->cid[1]);
	printf("p->rsrp = %02x%02x, p->rsrq = %02x%02x, p->snr = %02x%02x\r\n",p->rsrp[0],p->rsrp[1], p->rsrq[0], p->rsrq[1], p->snr[0], p->snr[1]);
	return sizeof(NbiotRadioQuality_t);
}

static int insert_radioQuality1(NbiotRadioQuality1_t *p)
{
	#if 0
	int rssi = MODEM_getLastRssi();
	int rsrp = MODEM_getLastRsrp();
	int rsrq = MODEM_getLastRsrq();
	int snr = MODEM_getLastSNR();

	p->rssi = rssi;
	p->ber = 0; // BER - 사용하지 않음.
	memset(p->cid, 0, 4);
	memcpy(p->cid, &modem.modemQuality.cid, 2);
	memcpy(p->rsrp, &rsrp, 2);
	memcpy(p->rsrq, &rsrq, 2);
	memcpy(p->snr, &snr, 2);
	#endif
	return sizeof(NbiotRadioQuality1_t);
	int tempdata = 0;
	p->rssi = 101;
	p->ber = 102;
	tempdata = 103;
	printf("p->cid = %d\r\n", tempdata);
	memcpy(p->cid, &tempdata, 4);
	tempdata = 104;
	printf("p->rsrp = %d\r\n", tempdata);
	memcpy(p->rsrp, &tempdata, 2);
	tempdata = 105;
	printf("p->rsrq = %d\r\n", tempdata);
	memcpy(p->rsrq, &tempdata, 2);
	tempdata = 106;
	printf("p->snr = %d\r\n", tempdata);
	memcpy(p->snr, &tempdata, 2);
	printf("p->rssi = %d, p->ber = %d, p->cid = %02x%02x\r\n",p->rssi, p->ber, p->cid[0],  p->cid[1]);
	printf("p->rsrp = %02x%02x, p->rsrq = %02x%02x, p->snr = %02x%02x\r\n",p->rsrp[0],p->rsrp[1], p->rsrq[0], p->rsrq[1], p->snr[0], p->snr[1]);
	return sizeof(NbiotRadioQuality_t);
}

static int insert_meteringParam(NbiotMeteringParam_t *p)
{
	p->mi = conf.meterInterval;
	p->ri = conf.reportInterval;

	return sizeof(NbiotMeteringParam_t);
}

int AppProcess = APP_INITIAL_REPORT;
static int insert_meterInfo(NbiotMeterInfo_t *p)
{
	
	uint8 pSerial[4] = {0x41, 0x42, 0x43, 0x44};//METER_getSerialNum();
	if (pSerial) {
		memcpy(p->meterSerial, pSerial, 4);
	}
	printf("meter serial = 0x%02x%02x%02x%02x \r\n", pSerial[0], pSerial[1], pSerial[2], pSerial[3]);

	p->meterType = 0x01;
	p->meterCaliber_dp = 0x02;//METER_getMeterCaliber_dp();

	//MeterUnitData_t *pUnitData;

	if (AppProcess == APP_INITIAL_REPORT || AppProcess == APP_IMMEDIATE_REPORT) {
		//pUnitData = METER_getTempData();
		p->meterStatus = 0x34;//pUnitData->meterStatus;
	} else {
		int nData = 1;//METER_getNumberOfStoredData();

		for (int i = 0; i < nData; i++) {
			p->meterStatus = 0x35;//METER_getMeterStatus(i);
			if (p->meterStatus != 0xff) {
				break;
			}
		}
	}

	if (p->meterStatus == 0xff) {
		p->meterStatus = 0;
	}

	return sizeof(NbiotMeterInfo_t);
}

// EC_RUDY203
static int insert_meteringParamExt(NbiotMeteringParamExt_t *p)
{
    int mi = conf.meterInterval;
    int ri = conf.reportInterval;

	mi = conf.isShortInterval? conf.meterInterval:(conf.meterInterval*60);
	ri = conf.isShortInterval? conf.reportInterval:(conf.reportInterval*60);

    memcpy(p->miMinute, &mi, 2);
    memcpy(p->riMinute, &ri, 2);

	return sizeof(NbiotMeteringParamExt_t);
}
#if 0
static int insert_meterInfoExt(NbiotMeterInfoExt_t *p)
{
	uint8 *pSerial = METER_getSerialNum();
	if (pSerial) {
		memcpy(p->meterSerial, pSerial, 4);
	}

	p->meterType = conf.meterType;
	p->meterCaliber_dp = METER_getMeterCaliber_dp();

	MeterUnitData_t *pUnitData;

	if (AppProcess == APP_INITIAL_REPORT || AppProcess == APP_IMMEDIATE_REPORT) {
		pUnitData = METER_getTempData();
		p->meterStatus = pUnitData->meterStatus;

        if (pUnitData->meterStatus == 0xFF)
        {
            memset(p->temperature, 0xFF, 2);
            memset(p->pressure, 0xFF, 2);
        }
        else
        {
            memcpy(p->temperature, pUnitData->temperature, 2);
            memcpy(p->pressure, pUnitData->pressure, 2);
        }
	} else {
		int nData = METER_getNumberOfStoredData();

		for (int i = 0; i < nData; i++) {
			p->meterStatus = METER_getMeterStatus(i);
			if (p->meterStatus != 0xff) {
                pUnitData = METER_getStoredData(i);
                memcpy(p->temperature, pUnitData->temperature, 2);
                memcpy(p->pressure, pUnitData->pressure, 2);
				break;
			}
            else
            {
                memset(p->temperature, 0xFF, 2);
                memset(p->pressure, 0xFF, 2);
            }
		}
	}

	if (p->meterStatus == 0xff) {
		p->meterStatus = 0;
	}

	return sizeof(NbiotMeterInfoExt_t);
}
#endif
// end of EC_RUDY203

static int insert_meteringTime(NbiotTimeStamp_t *p, uchar isJoinMsg)
{
	#if 0
	MeterUnitData_t *pUnit;
	if (isJoinMsg) {
		pUnit = METER_getTempData();
	} else {
		pUnit = METER_getLastData();
	}
	#endif

	if (1) {
		p->year = 24;//pUnit->year;
		p->mon = 5;//pUnit->mon;
		p->day = 13;//pUnit->day;
		p->hour = 01; //pUnit->hour;
		p->min = 50; //pUnit->min;
		p->sec = 40; //pUnit->sec;
		printf("MODEM : Metering time - set from data\n");
	} else {
		// 검침 데이터의 시간이 동기화된 것이 아닐 경우 전송시간에 대한 차이값으로 설정.
		if (isJoinMsg) {
			// Join message 의경우 검침 직후 보고하므로 차이값을 0로 설정.
			p->year = 0xFF;
			p->mon = 0xFF;
			p->day = 0xFF;
			p->hour = 0;
			p->min = 0;
			p->sec = 0;
			printf("MODEM : Metering time - set now\n");
		} else {
			// Join message가 아닐 경우 전송시간에 대한 차이값을 계산하여 설정.
			p->year = 0xFF;
			p->mon = 0xFF;
			p->day = 0xFF;

#if 0
			Date_t now;
			//RTC_read(&now);
			uint8 diff = conf.reportInterval % conf.meterInterval;
			// 1. 주기보고의 경우 Base time 및 interval로 계산된 시간을 설정한다.
			// 1.1. 이때 분단위 주기 동작 여부에 따라 다르게 설정한다.
			// 2. 시간단위 주기동작 에서 검침시간 대비 보고시간은 report min만큼 이후에 보고한다.
			// 2.1. 위의 이유로 분단위는 report min 값을 설정한다.
			if (conf.isShortInterval == 1) {
				p->hour = 0;
				p->min = 0;
			} else {
				p->hour = diff;
				p->min = conf.reportMin;
			}
			p->sec = 0;
			#endif
			printf("MODEM : Metering time - set difference\n");
		}
	}

	return sizeof(NbiotTimeStamp_t);
}
uchar * make_High_SendMsg(uchar *buf)
{
	uchar *pbuf = buf;
	
	//이동통신 ID
	//MobileId_t *p1 = (MobileId_t *)pbuf;
	printf("pbuf addr %ld (%d)\r\n", (long)pbuf, __LINE__);
	pbuf += insert_id((MobileId_t *)pbuf);
	// 무선 품질 정보
	if(conf.meterProtoVer == 0xA4) //protocol version 1.7
	{
		pbuf += insert_radioQuality1((NbiotRadioQuality1_t *)pbuf);
	}
	else //protocol version 1.6
	{
		pbuf += insert_radioQuality((NbiotRadioQuality_t *)pbuf);
	}

	//단말기 정보
	pbuf += insert_termInfo((NbiotDeviceInfo_t *)pbuf);

	//계량기 정보
	//pbuf += insert_meterInfo((NbiotMeterInfo_t *)pbuf);

	return pbuf;
}

uchar * make_Low_SendMsg(uchar *buf, uchar isJoinMsg)
{
	uchar *pbuf = buf;

	//계량기 정보
	pbuf += insert_meterInfo((NbiotMeterInfo_t *)pbuf);

	//검침 및 보고 주기
	pbuf += insert_meteringParam((NbiotMeteringParam_t *)pbuf);

	//검침 시간
	pbuf += insert_meteringTime((NbiotTimeStamp_t *)pbuf, isJoinMsg);
	
	return pbuf;
}

MeterStoredData_t StoredMeterData = { 0 };

uint8 METER_getNumberOfStoredData()
{
	return StoredMeterData.nData;
}

void METER_getMeterData(uint8 index, uint8 *buf)
{
	uint8 nData = METER_getNumberOfStoredData();
	if (index < nData) {
		memcpy(buf, StoredMeterData.unit[index].meterData, 4);
	}
}

BOOL METER_isAllFF(uchar *p, int len)
{
	BOOL result = TRUE;

	for (unsigned int i = 0; i < len; i++) {
		if (*(p + i) != 0xFF) {
			result = FALSE;
			break;
		}
	}
	return result;
}

void bcd2int(uint8 *pBCD, uint32 *pInt, uint8 nDigit)
{
	#if 1
	unsigned char i = 0;
	for(i=0; i<nDigit; i++)
	{
		*pInt *= 100;
		*pInt += ((*pBCD & 0xf0) >> 4) * 10 + (*pBCD & 0x0f);
		pBCD++;
	}
	#else
	uint32 value = 0;
	uint8 temp = 0;

	for (int i = 0; i < nDigit; i++) {
		value *= 100;
		temp = *(pBCD + i);
		temp = ((temp & 0xf0) >> 4) * 10 + (temp & 0x0f);
		value += temp;
	}

	*pInt = value;
	#endif
}

static int insert_multiData1(NbiotMeterData1_t *p)
{
	//METER_collectStoredData();

	uchar nData = 4; //METER_getNumberOfStoredData();
	#if 0
	if (nData > MAX_NUM_STORED_DATA) {
		nData = MAX_NUM_STORED_DATA;
	}
	#endif

	p->interval = 1;//METER_getSaveInterval();
	p->numData = nData;

	//memset(p->refValue, 0xFFFF, 4);
	for (uchar i = 0; i < nData; i++) {
		memset(p->valueDiff[i], 0xFFFF, 4);
	}

	uchar validPos = 0; //METER_getFirstValidPos();
	p->refValuePos = validPos;

	if (validPos != 0xFF) {
		uint8 valueArray[4];

		// 유효한 최신 데이터 채우기
		uint32 prevValue = 0;
		#if 0
		METER_getMeterData(validPos, valueArray);

		
		bcd2int(valueArray, &prevValue, 4);

		memcpy(p->refValue, &prevValue, 4);
		memset(p->valueDiff[validPos], 0, 4);
#endif
		for (uchar i = validPos; i < nData; i++) {
			METER_getMeterData(i, valueArray);

			if (METER_isAllFF(valueArray, 4)) {
				// meter down
				memset(p->valueDiff[i], 0xFFFF, 4);
			} else {
				uint32 currValue = 0;
				bcd2int(valueArray, &currValue, 4);

				#if 1	//teddy 240514
				if (prevValue < currValue) { // 검침값이 줄어든 경우
					//memset(p->valueDiff[i], 0, 4);
					printf("reduce meter pre:%ld cur:%ld\r\n",  prevValue, currValue);
				}
				memcpy(p->valueDiff[i], &currValue, 4);
				prevValue = currValue;
				#else
				if (prevValue < currValue) { // 검침값이 줄어든 경우
					memset(p->valueDiff[i], 0, 4);
				} else {
					//uint16 diff = (uint16)(prevValue - currValue);
					memcpy(p->valueDiff[i], &currValue, 4);
					prevValue = currValue;
				}
				#endif
			}
		}
	}

	return 3 + (nData * 4); // 7 = interval(1), nData(1), validPos(1), //refValue(4)
}


unsigned int MODEM_sendCurrentData(uchar *buf)
{
	
	uchar *pbuf = buf;

	NbiotMsgHeader_t *p = (NbiotMsgHeader_t *)buf;
	p->protocol = conf.meterProtoVer==0 ? 0xA3 : conf.meterProtoVer;
	p->mtype = 0x70;//NBIOT_DATA_REPORT;
	pbuf = buf + sizeof(NbiotMsgHeader_t);
	printf("NbiotMsgHeader_t size = %ld\r\n", sizeof(NbiotMsgHeader_t));
	pbuf = make_High_SendMsg(pbuf);
	pbuf = make_Low_SendMsg(pbuf, 1);
	#if 0
	if(conf.meterProtoVer == 0xA4) //protocol version 1.7
	{
		pbuf += insert_oneData1((NbiotMeterData1_t *)pbuf);
	}
	else
	{
		pbuf += insert_oneData((NbiotMeterData_t *)pbuf);
	}
	#endif
	pbuf += insert_multiData1((NbiotMeterData1_t *)pbuf);
	p->len = (uchar)(pbuf - &p->mtype);
	printf("message len = %d\r\n", (int)p->len);
	*pbuf = cal_checksum(&p->mtype, p->len); // 2 - protocol, len
	return p->len + 3;
}

int main(int argc, char *argv[])
{
	uchar msg[1024] = {0,};
	int msg_len = 0, i = 0;
	
	conf.isShortInterval = 1;
	conf.meterProtoVer = 0xA4;
	conf.meterInterval = 1;
	conf.reportInterval = 6;
	strcpy(conf.serialNum, "NL3323100021");
	
	StoredMeterData.nData = 4;
	
	StoredMeterData.unit[0].year = 24;
	StoredMeterData.unit[0].mon = 5;
	StoredMeterData.unit[0].day = 14;
	StoredMeterData.unit[0].hour = 11;
	StoredMeterData.unit[0].min = 00;
	StoredMeterData.unit[0].sec = 00;
	StoredMeterData.unit[0].meterData[0]=0x00;
	StoredMeterData.unit[0].meterData[1]=0x01;
	StoredMeterData.unit[0].meterData[2]=0x85;
	StoredMeterData.unit[0].meterData[3]=0x83;
	StoredMeterData.unit[0].meterStatus = 0x00;
	
	StoredMeterData.unit[1].year = 24;
	StoredMeterData.unit[1].mon = 5;
	StoredMeterData.unit[1].day = 14;
	StoredMeterData.unit[1].hour = 12;
	StoredMeterData.unit[1].min = 00;
	StoredMeterData.unit[1].sec = 00;
	StoredMeterData.unit[1].meterData[0]=0x00;
	StoredMeterData.unit[1].meterData[1]=0x01;
	StoredMeterData.unit[1].meterData[2]=0x64;
	StoredMeterData.unit[1].meterData[3]=0x17;
	StoredMeterData.unit[1].meterStatus = 0x00;
	
	StoredMeterData.unit[2].year = 24;
	StoredMeterData.unit[2].mon = 5;
	StoredMeterData.unit[2].day = 14;
	StoredMeterData.unit[2].hour = 13;
	StoredMeterData.unit[2].min = 00;
	StoredMeterData.unit[2].sec = 00;
	StoredMeterData.unit[2].meterData[0]=0x00;
	StoredMeterData.unit[2].meterData[1]=0x01;
	StoredMeterData.unit[2].meterData[2]=0x42;
	StoredMeterData.unit[2].meterData[3]=0x52;
	StoredMeterData.unit[2].meterStatus = 0x00;
	
	StoredMeterData.unit[3].year = 24;
	StoredMeterData.unit[3].mon = 5;
	StoredMeterData.unit[3].day = 14;
	StoredMeterData.unit[3].hour = 14;
	StoredMeterData.unit[3].min = 00;
	StoredMeterData.unit[3].sec = 00;
	StoredMeterData.unit[3].meterData[0]=0x00;
	StoredMeterData.unit[3].meterData[1]=0x01;
	StoredMeterData.unit[3].meterData[2]=0x20;
	StoredMeterData.unit[3].meterData[3]=0x86;
	StoredMeterData.unit[3].meterStatus = 0x00;
	
	msg_len = MODEM_sendCurrentData(msg);
	
	printf("message[%d] : \r\n", msg_len);
	for(i = 0; i<msg_len; i++)
	{
		printf("%02X", msg[i]);
	}
	printf("\r\ntest make meter message\r\n");
	return 0;
}
