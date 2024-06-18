#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define uint8 unsigned char
#define uint32 unsigned long
#define uchar unsigned char
#define uint16 unsigned short
#define BOOL 	unsigned char
#define TRUE 1
#define FALSE 0

// data sector
#define FIRST_DATA_SECTOR 0
#define NUM_DATA_SECTOR 18
#define UNIT_PER_DATA_SECTOR 5 // �ϳ��� sector�� �ټ� �������� ����
#define NUM_DATE_TO_STORE (NUM_DATA_SECTOR * UNIT_PER_DATA_SECTOR)

#define FLASH_DATA_MARKER 0x41544144

// map sector
#define MAP_SECTOR (FIRST_DATA_SECTOR + NUM_DATA_SECTOR)
#define NUM_MAP_SECTOR 1
#define FLASH_MAP_MARKER 0x2050414D

#define DATA_FLASH_RETRY_COUNT 5

#define ALL_FF 0xFFFFFFFF
#define _IS_SET(reg, n) ((reg) & (1 << (n)))


typedef struct {
	uint16 year; // yearH, yearL
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 reserved;
} Date_t;


typedef struct { // 100 bytes
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 caliber_dp;
	uint8 data[24][4]; // 96
} data_unit_t;

typedef struct { // 508
	uint32 marker; // 4
	data_unit_t dataUnit[UNIT_PER_DATA_SECTOR]; // 500
	uint32 checksum; // 4
} data_sector_t;

typedef struct { // 4
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 savePos;
} unit_map_t;

typedef struct { // 368
	uint32 marker; // 4
	unit_map_t unitMap[NUM_DATE_TO_STORE]; // 90 * 4 = 360
	uint32 checksum; // 4
} map_sector_t;

typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 caliber_dp;
	uint8 meterValue[4];
} flash_save_data_t;

typedef struct {
	uint8 year;
	uint8 mon;
	uint8 dayFlag[4];
} unit_day_data_map_t;

typedef struct {
	uint8 nDays;
	unit_day_data_map_t unitDayDataMap[5];
} day_data_map_t;

typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
} ymd_t;

typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 hour;
} dataTimeS_t;

typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 savePos;
	uint16 sortKey;
	uint8 txSeq;
} date_sort_t;

typedef union{
	uint32 data_b32;
	uint8 data_b8[4];
} dataTrans4_t;


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
	uchar interval;
	uchar numData;
	uchar refValuePos;
	uchar refValue[4];
	uchar valueDiff[24][2];
} NbiotMeterData_t;

typedef struct {
	uchar interval;
	uchar numData;
	uchar refValuePos;
	//uchar refValue[4];
	uchar valueDiff[24][4];
} NbiotMeterData1_t; //seoul protocol version 1.7

data_sector_t g_save_data[NUM_DATA_SECTOR];
map_sector_t g_Map;
uint8 sortMapdata[90];
Date_t now;

dataTimeS_t LastdataTime={0,};
static uchar cal_checksum(uchar *p, int len)
{
	uchar checksum = 0;

	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}

	return checksum;
}

static int isValidDate(uint8 year, uint8 mon, uint8 day)
{
	return (year > 20 && year < 100 && mon >= 1 && mon <= 12 && day >= 1 && day <= 31);
}

static int isValidMeterValue(uint8 *p)
{
	if ((*(p + 0) + *(p + 1) + *(p + 2) + *(p + 3)) == 0) {
		// all 0 --> invalid value
		return 0;
	}
#if 0
    char valueStr[0x10] = "";
    sprintf(valueStr, "%02X%02X%02X%02X", *(p+3), *(p+2), *(p+1), *(p+0));

    int valid = 1;
    for(int i = 0; i < strlen(valueStr); i++) {
        // string���� ��ȯ���� �� ���ڰ� �ƴѰ� ������ invalid
        if(valueStr[i] < '0' || valueStr[i] > '9') {
            valid = 0;
            break;
        }
    }
#else
	int valid = 1;
	for (int i = 0; i < 4; i++) {
		// 9���� ū���
		uint8 value = *(p + i);
		uint8 upper = (value >> 4) & 0x0f;
		uint8 lower = value & 0x0f;

		if (upper > 9 || lower > 9) {
			valid = 0;
			break;
		}
	}

#endif

	return valid;
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
void int2bcd(uint32 *pBCD, uint32 *pInt, uint8 nDigit)
{
	unsigned char i = 0;
	uint32 tempInt = *pInt;
	*pBCD = 0;
	for(i=0; i<nDigit; i++)
	{
		*pBCD |= (tempInt%10)<<(4*i);
		tempInt /= 10;
	}
}

void EndianTrans4b(uint8 *outData, uint8 *inData)
{
	//*outData = *(inData+3);
	outData[0] = inData[3];
	outData[1] = inData[2];
	outData[2] = inData[1];
	outData[3] = inData[0];
}
#if 0
static int read_mapSector(uint8 *buf)
{
	int result = 0;

	for (int retry = 0; retry < DATA_FLASH_RETRY_COUNT; retry++) {
		//if (readSector(MAP_SECTOR, buf)) {
			//map_sector_t *p = (map_sector_t *)buf;
			map_sector_t *p = &g_Map;
			if (p->marker == ALL_FF && p->checksum == ALL_FF) {
				break;
			}

			if (p->marker == FLASH_MAP_MARKER) {
				uint32 checksum = cal_checksum(
					(uint8 *)p, (int)((uint8 *)&p->checksum - (uint8 *)p));
				if (checksum == p->checksum) {
					result = 1;
					break;
				} else {
					if (retry >= DATA_FLASH_RETRY_COUNT - 1) {
						printf("Map: checksum error [cal:%08lx read:%08lx]\n",
						       checksum, p->checksum);
					}
				}
			} else {
				if (retry >= DATA_FLASH_RETRY_COUNT - 1) {
					printf("Map: invalid marker: %08lx\n", p->marker);
				}
			}
		//}
		//MISC_delayMs(5);
	}
	return result;
}
#endif

static void hex2binary(uint8 hex, char *binary)
{
	*binary = '\0';
	char *temp = binary;
	for (int i = 7; i >= 0; i--) {
#if 1
		if (_IS_SET(hex, i)) {
			//sprintf(binary, "%s1", binary);
			*temp = '1';
		} else {
			//sprintf(binary, "%s0", binary);
			*temp = '0';
		}
		temp++;
#else
		if (_IS_SET(hex, i)) {
			binary[i] = 1;
		} else {
			binary[i] = 0;
		}
#endif
	}
}
uint8 FindHour(uint8 datapos)
{
	uint8 tempSect = datapos / 5;
	uint8 tempUnit = datapos % 5;
	printf("sect =%d, Unit= %d\n", tempSect, tempUnit);
	data_sector_t *pData = &g_save_data[tempSect];
	LastdataTime.year = pData->dataUnit[tempUnit].year;
	LastdataTime.mon = pData->dataUnit[tempUnit].mon;
	LastdataTime.day = pData->dataUnit[tempUnit].day;
	for(int i=0; i<24; i++)
	{
		if(((pData->dataUnit[tempUnit].data[i][0]!=0)&&(pData->dataUnit[tempUnit].data[i][1]!=0)&& \
		(pData->dataUnit[tempUnit].data[i][2]!=0)&&(pData->dataUnit[tempUnit].data[i][3]!=0)))
		{
			return i;
		}
	}
	return 23;
}
static int get_dataSavePos(uint8 year, uint8 mon, uint8 day)
{
	//uint32 buf32[SECTOR_SIZE / 4];
	//read_mapSector((uint8 *)buf32);

	map_sector_t *pMap = &g_Map;//(map_sector_t *)buf32;

	int firstEmpty = -1;
	int matched = -1;
	int oldest = -1;

	uint32 minYMD = 0xFFFFFFFF;

	for (int unit = 0; unit < NUM_DATE_TO_STORE; unit++) {
		unit_map_t *pUnit = &pMap->unitMap[unit];

		if (isValidDate(pUnit->year, pUnit->mon, pUnit->day)) {
			if (pUnit->year == year && pUnit->mon == mon && pUnit->day == day) {
				matched = unit;
				break;
			}

			uint32 YMD = pUnit->year * 0x10000 + pUnit->mon * 0x100 + pUnit->day;
			if (YMD > 0 && minYMD > YMD) {
				minYMD = YMD;
				oldest = unit;
			}
		} else {
			if (firstEmpty < 0) {
				firstEmpty = unit;
			}
		}
	}

	if (matched >= 0) {
		return matched;
	}

	int savePos = 0;
	if (firstEmpty >= 0) {
		savePos = firstEmpty;
	} else if (oldest >= 0) {
		savePos = oldest;
	}

	unit_map_t *pUnitMap = &pMap->unitMap[savePos];
	memset((uint8 *)pUnitMap, 0, sizeof(unit_map_t));
	pUnitMap->year = year;
	pUnitMap->mon = mon;
	pUnitMap->day = day;
	pUnitMap->savePos = savePos;

	// update map sector
	pMap->checksum =
		cal_checksum((uint8 *)pMap, (int)((uint8 *)&pMap->checksum - (uint8 *)pMap));
	//writeSector(MAP_SECTOR, (uint8 *)buf32);

	// init specified position of data sector
	int sector = savePos / UNIT_PER_DATA_SECTOR;
	int unit = savePos % UNIT_PER_DATA_SECTOR;

#if 0
	if (read_dataSector(sector, (uint8 *)buf32) == 0) {
		memset((uint8 *)buf32, 0, SECTOR_SIZE);
	}
#endif
	//data_sector_t *pData = (data_sector_t *)buf32;
	data_sector_t *pData = &g_save_data[sector];

	pData->marker = FLASH_DATA_MARKER;
	data_unit_t *pUnitData = &pData->dataUnit[unit];
	memset((uint8 *)pUnitData, 0, sizeof(data_unit_t));
	pUnitData->year = year;
	pUnitData->mon = mon;
	pUnitData->day = day;

	pData->checksum =
		cal_checksum((uint8 *)pData, (int)((uint8 *)&pData->checksum - (uint8 *)pData));
	//writeSector(sector, (uint8 *)buf32);

	return savePos;
}

static int saveData(flash_save_data_t *newData)
{
	int savePos = get_dataSavePos(newData->year, newData->mon, newData->day);
	if (savePos < 0 || savePos >= (NUM_DATA_SECTOR * UNIT_PER_DATA_SECTOR)) {
		printf("dataFlash - get save position failed\n");
		return -1;
	}

	int sector = savePos / UNIT_PER_DATA_SECTOR;
	int unit = savePos % UNIT_PER_DATA_SECTOR;

	#if 0
	uint32 buf32[SECTOR_SIZE / 4];
	if (read_dataSector(sector, (uint8 *)buf32) == 0) {
		memset((uint8 *)buf32, 0, SECTOR_SIZE);
	}

	data_sector_t *pData = (data_sector_t *)buf32;
	#endif
	data_sector_t *pData = &g_save_data[sector];

	pData->marker = FLASH_DATA_MARKER;

	data_unit_t *pUnit = &pData->dataUnit[unit];
	pUnit->year = newData->year;
	pUnit->mon = newData->mon;
	pUnit->day = newData->day;
	pUnit->caliber_dp = newData->caliber_dp;

	memcpy(pUnit->data[newData->hour], newData->meterValue, 4);

	if (isValidMeterValue(pUnit->data[newData->hour]) == 0) {
		printf("meter value invalid[%02X%02X%02X%02X]\n", pUnit->data[newData->hour][0],
		       pUnit->data[newData->hour][1], pUnit->data[newData->hour][2],
		       pUnit->data[newData->hour][3]);
		return -1;
	}

	pData->checksum =
		cal_checksum((uint8 *)pData, (int)((uint8 *)&pData->checksum - (uint8 *)pData));
	//writeSector(sector, (uint8 *)buf32);

	return savePos;
}

void dataFlash_displayDataSector(int sector)
{
	//uint32 buf32[SECTOR_SIZE / 4];

	//if (read_dataSector(sector, (uint8 *)buf32)) {
		//data_sector_t *pData = (data_sector_t *)buf32;
		data_sector_t *pData = &g_save_data[sector];
		for (int unit = 0; unit < UNIT_PER_DATA_SECTOR; unit++) {
			data_unit_t *pUnit = &pData->dataUnit[unit];
			if (isValidDate(pUnit->year, pUnit->mon, pUnit->day) == 0) {
				printf("[%2d:%d] empty\n", sector, unit);
				continue;
			}

			printf("[%2d:%d] %04d-%02d-%02d dp:%02X\n", sector, unit,
			       pUnit->year + 2000, pUnit->mon, pUnit->day, pUnit->caliber_dp);
			for (unsigned int hour = 0; hour < 24; hour++) {
				printf("\t%2d) %02X%02X%02X%02X ", hour, pUnit->data[hour][0],
				       pUnit->data[hour][1], pUnit->data[hour][2],
				       pUnit->data[hour][3]);
				if ((hour + 1) % 6 == 0) {
					printf("\n");
				}
			}
		}
	//}
}

void resetSortMap(void)
{
	//sortMapdata
	int i = 0, j=1;
	//printf("reset sortmap \r\n%d : ", j++);
	for(i=0; i<90; i++)
	{
		sortMapdata[i]=89-i;
		#if 0
		printf("%d ", sortMapdata[i]);
		if(((i+1)%10)==0)
			printf("\n%d : ", j++);
			#endif
	}
}
uint8 sortMapSector(map_sector_t *pMap)
{
	
	uint8 i, j;
	unit_map_t *tempMap = NULL;
	int tempComp = 0;
	uint8 tempvalue;
	
	//pMap =  &g_Map;
	memcpy(pMap, &g_Map, sizeof(map_sector_t));
	#if 0
	for(i=0; i<90; i++)
	{
		sortMapdata[i]=i;
	}
	#endif
	for(i=0; i<90; i++)
	{
		for(j=i+1; j<90; j++)
		{
			tempMap = &pMap->unitMap[sortMapdata[i]];
			if (isValidDate(tempMap->year, tempMap->mon, tempMap->day)==0)
			{
				tempvalue = sortMapdata[i];
				sortMapdata[i] = sortMapdata[j];
				sortMapdata[j] = tempvalue;
			}
			else
			{
				tempMap = &pMap->unitMap[sortMapdata[j]];
				if (isValidDate(tempMap->year, tempMap->mon, tempMap->day))
				{
					tempComp =  memcmp(&pMap->unitMap[sortMapdata[i]], &pMap->unitMap[sortMapdata[j]], sizeof(uint8)*3);
					if(tempComp < 0){
						tempvalue = sortMapdata[i];
						sortMapdata[i] = sortMapdata[j];
						sortMapdata[j] = tempvalue;
					}
				}
			}

			
		}
	}
	for(i=0; i<90; i++)
	{
		tempMap = &pMap->unitMap[sortMapdata[i]];
		if (isValidDate(tempMap->year, tempMap->mon, tempMap->day)==0)
			break;
	}
	#if 0
	if(i==0)
	{
		memset(&LastdataTime, 0, sizeof(LastdataTime));
	}
	else
	{
		LastdataTime.year = pMap->unitMap[sortMapdata[i-1]].year;
		LastdataTime.mon = pMap->unitMap[sortMapdata[i-1]].mon;
		LastdataTime.day = pMap->unitMap[sortMapdata[i-1]].day;
	}
	#endif
	printf("nday = %d\n", i);
	return i;
}
#if 0
static int make_dateList(int nDay,  day_data_map_t *pDayMap)
{
	int nMon = -1;
	int mon = -1;
	int year = -1;
	map_sector_t *pMap;
	unit_map_t *p = NULL;
	//fix_mapSector();
	//read_mapSector((uint8 *)pMap);
	pMap = &g_Map;

	for (unsigned int i = 0; i < nDay; i++) {
		//date_sort_t *p = pSort + i;
		p = &pMap->unitMap[sortMapdata[i]];
		printf("%02d-%02d-%02d\n", p->year, p->mon, p->day);
		if (year != p->year || mon != p->mon) {
			if (++nMon >= 5) {
				break;
			}
			year = p->year;
			mon = p->mon;
			pDayMap->unitDayDataMap[nMon].year = p->year;
			pDayMap->unitDayDataMap[nMon].mon = p->mon;
		}
		unsigned int nByte = (p->day - 1) / 8;
		unsigned int nBit = 7 - ((p->day - 1) % 8);
		pDayMap->unitDayDataMap[nMon].dayFlag[nByte] |= (1<<nBit);
	}

	nMon += 1; // ������ -1�̱� ����...

	for (unsigned int i = 0; i < nMon; i++) {
		unit_day_data_map_t *pUnitMap = &pDayMap->unitDayDataMap[i];
		if (isValidDate(pUnitMap->year, pUnitMap->mon, 1)) {
			printf("%04d-%02d ", pUnitMap->year + 2000, pUnitMap->mon);

			for (unsigned int j = 0; j < 4; j++) {
				char binary[0x10] = "";
				hex2binary(pUnitMap->dayFlag[j], binary);
				printf("%s ", binary);
			}
			printf("\n");
		}
	}

	return nMon;
}
#endif
static int make_dateList(int nDay, map_sector_t *pMap, day_data_map_t *pDayMap)
{
	int nMon = -1;
	int mon = -1;
	int year = -1;
	unit_map_t *p = NULL;
	
	for (unsigned int i = 0; i < nDay; i++) {
		//date_sort_t *p = pSort + i;
		p = &pMap->unitMap[sortMapdata[i]];
		if (year != p->year || mon != p->mon) {
			if (++nMon >= 5) {
				break;
			}
			year = p->year;
			mon = p->mon;
			pDayMap->unitDayDataMap[nMon].year = p->year;
			pDayMap->unitDayDataMap[nMon].mon = p->mon;
		}
		unsigned int nByte = (p->day - 1) / 8;
		unsigned int nBit = 7 - ((p->day - 1) % 8);
		pDayMap->unitDayDataMap[nMon].dayFlag[nByte] |= (1<<nBit);
	}

	nMon += 1; // 시작이 -1이기 때문...

	for (unsigned int i = 0; i < nMon; i++) {
		unit_day_data_map_t *pUnitMap = &pDayMap->unitDayDataMap[i];
		if (isValidDate(pUnitMap->year, pUnitMap->mon, 1)) {
			//printf("%04d-%02d ", pUnitMap->year + 2000, pUnitMap->mon);

			for (unsigned int j = 0; j < 4; j++) {
				char binary[0x10] = "";
				hex2binary(pUnitMap->dayFlag[j], binary);
				printf("%s ", binary);
			}
			printf("\n");
		}
	}

	return nMon;
}

static int readAndSort_map(map_sector_t *pMap, date_sort_t *pSort)
{
	//fix_mapSector();
	//read_mapSector((uint8 *)pMap);
	pMap = &g_Map;

	int nDay = 0;
	for (int nUnit = 0; nUnit < NUM_DATE_TO_STORE; nUnit++) {
		unit_map_t *pUnitMap = &pMap->unitMap[nUnit];
		if (isValidDate(pUnitMap->year, pUnitMap->mon, pUnitMap->day)) {
			date_sort_t *p = pSort + nDay;
			p->year = pUnitMap->year;
			p->mon = pUnitMap->mon;
			p->day = pUnitMap->day;
			p->savePos = pUnitMap->savePos;
			p->sortKey = pUnitMap->year * 400 + pUnitMap->mon * 31 + pUnitMap->day;
			nDay++;
		}
	}

	if (nDay >= 2) {
		for (int i = 0; i < nDay - 1; i++) {
			date_sort_t *top = pSort + i;
			for (int j = i + 1; j < nDay; j++) {
				date_sort_t *next = pSort + j;
				if (top->sortKey < next->sortKey) {
					date_sort_t temp;
					memcpy(&temp, next, sizeof(date_sort_t));
					memcpy(next, top, sizeof(date_sort_t));
					memcpy(top, &temp, sizeof(date_sort_t));
				}
			}
		}
	}

	return nDay;
}
void dataFlash_displayMapSector()
{
	uint32 buf32[512 / 4];
	map_sector_t *pMap = (map_sector_t *)buf32;

	//date_sort_t SortedData[NUM_DATE_TO_STORE];
	//memset(&SortedData, 0, sizeof(SortedData));

	//int nDay = readAndSort_map(pMap, SortedData);
	int nDay = sortMapSector(pMap);
	if (nDay == 0) {
		return;
	}
	printf("day : %d\n", nDay);
	#if 0
	printf("%04d-%02d-%02d ", pMap->unitMap[0].year + 2000, pMap->unitMap[0].mon,
				       pMap->unitMap[0].day);
					   #endif
	day_data_map_t dayDataMap;
	memset(&dayDataMap, 0, sizeof(dayDataMap));
	//int nMonth = make_dateList(nDay, SortedData, &dayDataMap);
	//pMap = &g_Map;
	int nMonth = make_dateList(nDay, pMap, &dayDataMap);

	for (int nSector = FIRST_DATA_SECTOR; nSector < NUM_DATA_SECTOR; nSector++) {
		printf("[sector %2d] ", nSector);
		for (int nUnit = 0; nUnit < UNIT_PER_DATA_SECTOR; nUnit++) {
			int pos = nSector * UNIT_PER_DATA_SECTOR + nUnit;
			unit_map_t *pUnit = &pMap->unitMap[pos];//&pMap->unitMap[pos];
			printf("(%d) ", nUnit);
			if (isValidDate(pUnit->year, pUnit->mon, pUnit->day)) {
				printf("%04d-%02d-%02d ", pUnit->year + 2000, pUnit->mon,
				       pUnit->day);
			} else {
				//printf("           ");
				printf("\t\t");
			}
		}
		printf("\n");
		//WDT_CLEAR;
	}
}
void display_sector(char mode)
{
	int i = 0;
	map_sector_t *pMap =  &g_Map;
	if(mode==0)
	{
		printf("sector data \r\n");
		for(i=0; i<90; i++)
		{
			printf("(%02d) %02d-%02d-%02d ", i,  pMap->unitMap[i].year, pMap->unitMap[i].mon, pMap->unitMap[i].day);
			if(((i+1)%10) == 0)
				printf("\n");
		}
	}
	else
	{
		printf("sort sector data \r\n");
		for(i=0; i<90; i++)
		{
			printf("(%02d) %02d-%02d-%02d ", i, pMap->unitMap[sortMapdata[i]].year, pMap->unitMap[sortMapdata[i]].mon, pMap->unitMap[sortMapdata[i]].day);
			if(((i+1)%10) == 0)
				printf("\n");
		}
	}
}


uint8 isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint16 dateToDays(dataTimeS_t inDate)
{
	uint8 daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	uint16 days = 0, temp = 0;
	for (temp = 1; temp < inDate.year; temp++) {
        days += isLeapYear(temp + 2000) ? 366 : 365;
    }
    // ���ذ� �����̸� 2���� �� ���� 29�Ϸ� ����
    if (isLeapYear(inDate.year + 2000)) {
        daysInMonth[2-1] = 29;
    }
    
    // ������ ���� ������ �� �� ���ϱ�
    for (temp = 0; temp < (inDate.mon-1); temp++) {
        days += daysInMonth[temp];
    }
    // ���� ���� �� �� ���ϱ�
    days += inDate.day;
	return days;
}

uint8 LastMonthDays(dataTimeS_t inDate)
{
	uint8 daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (isLeapYear(inDate.year + 2000)) {
        daysInMonth[2-1] = 29;
    }
	return daysInMonth[inDate.mon -1];
}

uint8 get_message_mode(dataTimeS_t now, dataTimeS_t LastAck)
{
	uint8 i, ListData[6]={1,2,3,4,6,12};
	int nowDays=0, LastDays=0;
	int tempTime = 0;
	
	nowDays = dateToDays(now);
	LastDays = dateToDays(LastAck);
	printf("now days : %d, Last days : %d \n", nowDays, LastDays);
	printf("diff days : %d \n", nowDays - LastDays);
	nowDays -= LastDays;
	printf("diff : %d\n", nowDays);
	tempTime = now.hour-LastAck.hour;
	printf("diff time : %d\n", tempTime);
	if((tempTime)<0)
	{
		nowDays--;
	}
	//printf("diff h : %d\n", nowDays);
	#if 1
	for(i=0; i<6; i++)
	{
		if((nowDays < ListData[i])||((nowDays == ListData[i])&&(tempTime == 0))){
			return ListData[i];
		}
	}
	return 24;
	#else
	if((nowDays < 1)||((nowDays == 1)&&(tempTime == 0))){
		return 1;
	}
	else if((nowDays < 2)||((nowDays == 2)&&(tempTime == 0)))
	{
		return 2;
	}else if((nowDays < 3)||((nowDays == 3)&&(tempTime == 0)))
	{
		return 3;	
	}
	else if((nowDays < 4)||((nowDays == 4)&&(tempTime == 0)))
	{
		return 4;	
	}
	else if((nowDays < 4)||((nowDays == 4)&&(tempTime == 0)))
	{
		return 4;	
	}
	#endif
}

dataTimeS_t decreaseDate(dataTimeS_t Indata, uint8 mode)
{
	//dataTimeS_t ret = {0,};
	if((Indata.hour - mode) < 0)
	{
		if(Indata.day == 1)
		{
			if(Indata.mon == 1)
			{
				Indata.mon = 12;
				Indata.day = 31;
				Indata.year--;
			}
			else
			{
				Indata.mon--;
				Indata.day = LastMonthDays(Indata);
			}
		}
		else
		{
			Indata.day--;
		}
		Indata.hour = Indata.hour + 24 - mode;
	}
	else
		Indata.hour -= mode;
	
	//memcpy(&ret, &Indata, sizeof(dataTimeS_t));
	//return ret;
	return Indata;
}

uint8 CheckSectMapSort(dataTimeS_t Indata)
{
	map_sector_t *pMap =  &g_Map;
	int i;
	for(i=0; i<90; i++)
	{
		if(memcmp(&pMap->unitMap[sortMapdata[i]], &Indata, sizeof(uint8)*3) == 0)
		{
			return sortMapdata[i];
		}
	}
	printf("error not find\n");
	return 0xFF;
}
uint8 CheckMeterDataCount(dataTimeS_t start, dataTimeS_t Last, uint8 mode)
{
	uint8 len = 0;
	dataTimeS_t tempdate = start;
	while(1)
	{
		tempdate = decreaseDate(tempdate, mode);
		if(memcmp(&tempdate, &Last, sizeof(dataTimeS_t))<=0)
		{
			printf("finish data\n");
			return len;
		}
		len++;
	}
	if(len>23)
	{
		return 23;
	}
	return len;
}

dataTimeS_t tempLastA;
dataTimeS_t LastAckDate;

uint8 meterList(dataTimeS_t Indata, dataTimeS_t LastAck, uint8 mode, uint32 *data)
{
	//int i;
	uint8 ret = 0, tempSect, tempUnit;
	uint8 datapos = 0, timefirst = 0;
	
	data_sector_t *pData =NULL;
	dataTimeS_t tempdate = Indata;
	uint8 Len =0;
	uint8 *tempData = NULL;
	while(1)
	{
		#if 0
		if((Indata.hour - mode) < 0)
		{
			if(Indata.day == 1)
			{
				if(Indata.mon == 1)
				{
					Indata.mon = 12;
					Indata.day = 31;
					Indata.year--;
				}
				else
				{
					Indata.mon--;
					Indata.day = LastMonthDays(Indata);
				}
			}
			else
			{
				Indata.day--;
			}
			Indata.hour = Indata.hour + 24 - mode;
		}
		else
			Indata.hour -= mode;
	#endif
		tempdate = decreaseDate(tempdate, mode);
		if(memcmp(&tempdate, &LastAck, sizeof(dataTimeS_t))<=0)
		{
			printf("finish data\n");
			return Len;
		}
		//printf("decrease time : %02d-%02d-%02d:%02d \r\n",tempdate.year, tempdate.mon, tempdate.day, tempdate.hour);
		datapos = CheckSectMapSort(tempdate);
		printf("data position : %d\n", datapos);
		if(datapos == 0xFF)
		{
			printf("not found data \r\n");
			data++;
			Len++;
			//return 0xFF;
			continue;
		}
#if 0
		for(i=0; i<91; i++)
		{
			if(memcmp(&pMap->unitMap[sortMapdata[i]], &Indata, sizeof(uint8)*3) == 0)
			{
				ret = 1;
				break;
			}
		}
		if(ret == 0)
		{
			printf("error not find\n");
			break;
		}
		tempSect = sortMapdata[i] /5;
		tempUnit = sortMapdata[i] % 5;
#endif
		tempSect = datapos / 5;
		tempUnit = datapos % 5;
		pData = &g_save_data[tempSect];
		printf("data List \r\n");
		do
		{
			if(timefirst == 1)
			{
				tempdate.hour -= mode;
			}
			timefirst = 1;
			if(memcmp(&tempdate, &LastAck, sizeof(dataTimeS_t))<=0)
			{
				printf("finish data\n");
				return Len;
			}
			Len++;

			 //data print
			 printf("%02d/%02d ",tempSect, tempUnit);
			printf("%02d-%02d-%02d:%02d [%02d] %02x%02x%02x%02x \n",\
			pData->dataUnit[tempUnit].year, pData->dataUnit[tempUnit].mon, pData->dataUnit[tempUnit].day, \
			tempdate.hour, pData->dataUnit[tempUnit].caliber_dp, pData->dataUnit[tempUnit].data[tempdate.hour][0], \
			pData->dataUnit[tempUnit].data[tempdate.hour][1], pData->dataUnit[tempUnit].data[tempdate.hour][2], pData->dataUnit[tempUnit].data[tempdate.hour][3]);

			uint32 sampledata = 0xFFFFFFFF;
			tempData = pData->dataUnit[tempUnit].data[tempdate.hour];
			#if 0
			printf("%02d-%02d-%02d:%02d[%2d/%d] %02X%02X%02X%02X\r\n", \
			pData->dataUnit[tempUnit].year, pData->dataUnit[tempUnit].mon, pData->dataUnit[tempUnit].day,\
			tempdate.hour, )
			#endif
			if((tempData[0]==0xFF)&&(tempData[1]==0xFF)&&(tempData[2]==0xFF)&&(tempData[3]==0xFF))
			//if(memcmp(pData->dataUnit[tempUnit].data[tempdate.hour], &sampledata, sizeof(uint32))==0)
			{
				memset(data, 0xFFFFFFFF, sizeof(uint32));
				//printf("data 0xFFFFFFFF\n");
			}
			else
			{
				//bcd2int(pData->dataUnit[tempUnit].data[tempdate.hour], data, 4);
				memcpy(data, pData->dataUnit[tempUnit].data[tempdate.hour], 4);
			}
			if(Len > 23)
			{
				return 23;
			}
			data++;
		}
		while ((tempdate.hour - mode)>=0);
		timefirst = 0;
	}
	return Len;
}

void TestPro(dataTimeS_t nowdate, dataTimeS_t ackdate)
{
	uint8 tempMode = 0;
	uint8 count =0, i;
	uint32 *tempData = NULL;
	uint32 *p=NULL;
	printf("Now %02d-%02d-%02d %02dh\r\n", nowdate.year, nowdate.mon, nowdate.day, nowdate.hour);
	printf("Last %02d-%02d-%02d %02dh\r\n", ackdate.year, ackdate.mon, ackdate.day, ackdate.hour);
	tempMode = get_message_mode(nowdate, ackdate);
	printf("temp Mode = %d\n", tempMode);
	count = CheckMeterDataCount(nowdate, ackdate, tempMode);
	printf("check count = %d\n", count);
	tempData = (uint32 *)malloc(sizeof(uint32)*count);
	memset(tempData, 0, sizeof(uint32)*count);
	count = meterList(nowdate, tempLastA, tempMode, tempData);
	printf("data count = %d\n", count);
	p = tempData;
	uint32 sampledata = 0xFFFFFFFF;

	for(i=0; i<count; i++)
	{
		if(*p&0x00FFFFFFFF == 0xFFFFFFFF)
		{
			printf("FF dif =%08lx(%08lx)\n", *p&0x00FFFFFFFF, *p);
		}
		
		printf("%02d: %ld\r\n", i, *p++);
		printf("diff %ld - %ld = %ld\n", tempData[i], tempData[i+1], tempData[i]-tempData[i+1]);
		//p++;
	}
	free(tempData);
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
MeterUnitData_t StroeUnit;

MeterUnitData_t *METER_getStoredData(void)
{
	return &StroeUnit; //&StoredMeterData.unit;
}
static int insert_multiData(NbiotMeterData_t *p)
{
#if 0

	METER_collectStoredData();

	uchar nData = METER_getNumberOfStoredData();
	if (nData > MAX_NUM_STORED_DATA) {
		nData = MAX_NUM_STORED_DATA;
	}
#endif

	//Date_t now;
	dataTimeS_t nowdate;
	uint8 i=0, posval = 0, refplus =0, datacnt =0;;
	uint32 *loadData = NULL;
	MeterUnitData_t *lastUnit;
	uint32 prevValue = 0, nowValue = 0;
	uint16 diff=0;
	//RTC_read(&now);
	#if 0
	now.year = 2024;
	now.mon = 3;
	now.day = 15;
	now.hour = 11;
	now.min = 45;
	now.sec = 22;
	#endif
	nowdate.year = now.year - 2000;
	memcpy(&nowdate.mon, &now.mon, sizeof(uint8)*3); //test 필요

	printf("now : %02d-%02d-%02d : %02d\n",nowdate.year,nowdate.mon,nowdate.day,nowdate.hour );
	printf("Last : %02d-%02d-%02d : %02d\n", LastAckDate.year, LastAckDate.mon, LastAckDate.day, LastAckDate.hour);
	//interval meter mode
	p->interval = get_message_mode(nowdate, LastAckDate);
	p->numData = CheckMeterDataCount(nowdate, LastAckDate, p->interval);
	
	printf("interval = %d\n", p->interval);
	printf("data count = %d\n", p->numData);
	//loadData = (uint32 *)OSAL_malloc(sizeof(uint32) * p->numData);
	lastUnit = METER_getStoredData();
	p->refValuePos = 0;
	memset(p->valueDiff[0], 0, 2);

	if(p->numData > 0)
	{
		loadData = (uint32 *)malloc(sizeof(uint32) * p->numData);
		memset(loadData, 0, sizeof(uint32) * p->numData);
		datacnt = meterList(nowdate, LastAckDate, p->interval, loadData);
		printf("data count : %d\n", datacnt);

		if (METER_isAllFF(lastUnit->meterData, 4) == FALSE) {
			bcd2int(lastUnit->meterData, &prevValue, 4);
			memcpy(p->refValue, &prevValue, 4);
			memset(p->valueDiff[0], 0x00, 2);
		}
		else
		{
			memset(p->valueDiff[0], 0xFF, 2);
			p->refValuePos++;
			for(i=0;  i<datacnt; i++)
			{
				if((loadData[i]&0x00ffffffff)==0xffffffff)
				{
					p->refValuePos++;
					memset(p->valueDiff[i+1], 0xFF, 2);
				}
				else
					break;
			}
			//memcpy(p->refValue, &loadData[i], 4);
			prevValue = 0;
			bcd2int((unsigned char *)&loadData[i], &prevValue, 4);
			memcpy(p->refValue, &prevValue, 4);
			memset(p->valueDiff[i+1], 0x00, 2);
			//prevValue = loadData[i];
			i++;
		}
		printf("md i = %d\n", i);
		//datacnt -= p->refValuePos;
		for(i; i<datacnt; i++)
		{
			//if(METER_isAllFF((unsigned char *)&loadData[i], 4)==TRUE)
			if((loadData[i]&0x00ffffffff)==0xffffffff)
			{
				memset(p->valueDiff[i + 1], 0xFF, 2);
			}
			else
			{
				nowValue = 0;
				bcd2int((unsigned char *)&loadData[i], &nowValue, 4);
				if(prevValue < nowValue)
				{
					memset(p->valueDiff[i + 1], 0, 2);
					printf("[%2d] pre : %8ld, now : %8ld\r\n", i, prevValue, nowValue);
				}
				else
				{
					diff = prevValue - nowValue;
					prevValue = nowValue;
					printf("%02d : %6d\r\n", i, diff);
					memcpy(p->valueDiff[i+1], &diff, 2);
				}
			}
		}
		p->numData++;
		free(loadData);
	}
	else
	{
		bcd2int(lastUnit->meterData, &prevValue, 4);
		memcpy(p->refValue, &prevValue, 4);
		memset(p->valueDiff[0], 0xFF, 2);
		p->numData=1;
	}

	printf("refVelue : %02x%02x%02x%03x\ndata count:%2d\n", p->refValue[0], p->refValue[1], p->refValue[2], p->refValue[3], p->numData);
	printf("position : %d\n", p->refValuePos);
	printf("diff data : \n");
	for(i=0; i<p->numData; i++)
	{
		printf("%d: %02x%02x %3d\n",i , p->valueDiff[i][1] ,p->valueDiff[i][0] ,p->valueDiff[i][0]);
	}
	printf("\n");

	
	//OSAL_free(loadData);
return 7 + (p->numData * 2); // 7 = interval(1), nData(1), validPos(1), refValue(4)
}
#define test1  0
static int insert_multiData1(NbiotMeterData1_t *p)
{
	//Date_t now;
	dataTimeS_t nowdate;
	uint8 i=0, refplus = 0, data_en = 0;
	uint32 *loadData = NULL;
	MeterUnitData_t *lastUnit;
	uint32 prevValue = 0, nowValue = 0;
	uint16 diff=0;
	//RTC_read(&now);
	#if 0
	now.year = 2024;
	now.mon = 3;
	now.day = 15;
	now.hour = 1;
	now.min = 45;
	now.sec = 22;
	#endif
	nowdate.year = now.year - 2000;
	memcpy(&nowdate.mon, &now.mon, sizeof(uint8)*3); //test 필요

	printf("now : %02d-%02d-%02d : %02d\n",nowdate.year,nowdate.mon,nowdate.day,nowdate.hour );
	printf("Last : %02d-%02d-%02d : %02d\n", LastAckDate.year, LastAckDate.mon, LastAckDate.day, LastAckDate.hour);
	
	//interval meter mode
	p->interval = get_message_mode(nowdate, LastAckDate);
	p->numData = CheckMeterDataCount(nowdate, LastAckDate, p->interval);
	printf("interval = %d\n", p->interval);
	printf("data count = %d\n", p->numData);
	
	memset(p->valueDiff, 0, sizeof(char)*24*4);
	
	lastUnit = METER_getStoredData();
	printf("last Unit : %02x%02x%02x%02x\r\n", lastUnit->meterData[0], lastUnit->meterData[1], lastUnit->meterData[2], lastUnit->meterData[3]);
	p->refValuePos = 0;

	if(p->numData > 0)
	{
		loadData = (uint32 *)malloc(sizeof(uint32) * p->numData);
		memset(loadData, 0, sizeof(uint32) * p->numData);
		p->numData = meterList(nowdate, LastAckDate, p->interval, loadData);

		if (METER_isAllFF(lastUnit->meterData, 4) == FALSE) {
			#if test1
			nowValue = 0;
			bcd2int(lastUnit->meterData, &nowValue, 4);
			memcpy(p->valueDiff[0], &nowValue, 4);
			#else
			bcd2int(lastUnit->meterData, (unsigned long *)p->valueDiff[0], 4);
			#endif
			//
			//printf("pre 0 : %08lx\n", prevValue);
			//printf("pre_0 : %02x%02x%02x%02x\r\n", p->valueDiff[0][0], p->valueDiff[0][1], p->valueDiff[0][2], p->valueDiff[0][3]);
			//memcpy(p->valueDiff[0], lastUnit->meterData, 4);
			
			refplus = 1;
		}
		else
		{
			memset(p->valueDiff[0], 0xFF, 4);
			p->refValuePos++;
			for(i=0;  i<p->numData; i++)
			{
				if((loadData[i]&0x00ffffffff)==0xffffffff)
				{
					p->refValuePos++;
					memset(p->valueDiff[i+1], 0xFF, 4);
				}
				else
					break;
			}
			
			//memcpy(p->valueDiff[i+1], &loadData[i], 4);
			//bcd2int(lastUnit->meterData, (unsigned long *)p->valueDiff[i+1], 4);
			//
			#if test1
			nowValue=0;
			bcd2int((unsigned char *)&loadData[i], &nowValue, 4);
			memcpy(p->valueDiff[i+1], &nowValue, 4);
			#else
			bcd2int((unsigned char *)&loadData[i], (unsigned long *)&p->valueDiff[i+1], 4);
			#endif
			i++;
		}

		//p->numData -= p->refValuePos;
		printf("md i = %d\n", i);
		for(i; i<p->numData; i++)
		{
			if((loadData[i]&0x00ffffffff)==0xffffffff)
			{
				memset(p->valueDiff[i+1], 0xFF, 4);
			}
			else
			{
				//memcpy(p->valueDiff[i+1], &loadData[i], 4);
				//prevValue = 0;
				
				//
				//printf("pre1 %d : %08lx\n", i, prevValue);
				//printf("pre2 %d : %02x%02x%02x%02x\r\n", i, p->valueDiff[i+1][0], p->valueDiff[i+1][1], p->valueDiff[i+1][2], p->valueDiff[i+1][3]);
				#if test1
				nowValue=0;
				bcd2int((unsigned char *)&loadData[i], &nowValue, 4);
				memcpy(p->valueDiff[i+1], &nowValue, 4);
				#else
				bcd2int((unsigned char *)&loadData[i], (unsigned long *)&p->valueDiff[i+1], 4);
				#endif
				//printf("pro3 %d : %02x%02x%02x%02x\r\n", i, p->valueDiff[i+1][0], p->valueDiff[i+1][1], p->valueDiff[i+1][2], p->valueDiff[i+1][3]);
				
			}
		}
		//p->numData+=refplus;
		p->numData++;
		printf("last data cnt=%d\n",p->numData);

		free(loadData);
	}
	else
	{
		//
		//memcpy(p->valueDiff[0], lastUnit->meterData, 4);
		#if test1
		prevValue = 0;
		bcd2int(lastUnit->meterData, &prevValue, 4);
		memcpy(p->valueDiff[0], &prevValue, 4);
		#else
		bcd2int(lastUnit->meterData, (unsigned long *)p->valueDiff[0], 4);
		#endif
		p->numData = 1;
	}
	printf("msg data cnt : %2d \nposition : %d\n", p->numData, p->refValuePos);
	for(i=0; i<p->numData; i++)
	{
		printf("%2d:%02x%02x%02x%02x\n", i,  p->valueDiff[i][3],  p->valueDiff[i][2],  p->valueDiff[i][1],  p->valueDiff[i][0]);
	}


	//OSAL_free(loadData);
	return 3 + (p->numData * 4); // 7 = interval(1), nData(1), validPos(1), refValue(4)
}
int main(int argc, char *argv[])
{
	flash_save_data_t save_data[90*24]={0,};
	dataTrans4_t meter_data;
	dataTimeS_t tempIndate;
	uint32 meter_add = 10;
	//meter_data.data_b32 = 10;
	int i, j=0, k=1, l=0, m=0;
	uint32 temp_test = 12345678, temp_trans = 0;
	int2bcd(&temp_trans, &temp_test, 8);
	printf("in:%08ld out:0x%08lX\n", temp_test, temp_trans);

	memset(g_save_data, 0xFF, sizeof(data_sector_t)*NUM_DATA_SECTOR);
	memset(&g_Map, 0xFF, sizeof(map_sector_t));
	srand(time(NULL));
	printf("start save data input\n");
	m=24;
	j=6;
	k=11;
	//for(i=0; i< 90*24; i++)
	for(i=0; i< 80*24; i++)
	{
		save_data[i].year = m;
		save_data[i].mon = j;
		save_data[i].day = k;
		save_data[i].hour = l++;
		memcpy(&tempIndate, &save_data[i], sizeof(uint8)*4);
		if(l>23)
		{
			l=0;
			if(++k>LastMonthDays(tempIndate))
			{
				if(j==12)
				{
					j=1;
					m++;
				}
				else
					j++;
				k=1;
			}
		}
		save_data[i].caliber_dp = 3;
		meter_add += (int)(rand()%100);
		temp_trans = 0;
		int2bcd(&temp_trans, &meter_add, 8);
		//printf("%d-%d-%d:%d meter data[%d]= %ld(%08lx)\n",save_data[i].year, save_data[i].mon, save_data[i].day,save_data[i].hour,i, meter_add, temp_trans);
		//memcpy(save_data[i].meterValue, &temp_trans, sizeof(uint32));
		meter_data.data_b32 = temp_trans;
		#if 0
		if((i%3)==0)
		{
			memset(save_data[i].meterValue, 0xff, sizeof(uint8)*4);
		}
		else
		{
			EndianTrans4b(save_data[i].meterValue, meter_data.data_b8);
		}
		
		#else
		save_data[i].meterValue[0] = meter_data.data_b8[3];
		save_data[i].meterValue[1] = meter_data.data_b8[2];
		save_data[i].meterValue[2] = meter_data.data_b8[1];
		save_data[i].meterValue[3] = meter_data.data_b8[0];
		#endif
		saveData(&save_data[i]);
	}
	printf("finish save data input\n");
	resetSortMap();
	
	dataFlash_displayMapSector();
	
	//resetSortMap();
	display_sector(0);
	map_sector_t pMap;
	int t_day = sortMapSector(&pMap);
	LastdataTime.hour = FindHour(sortMapdata[t_day-1]);

	printf("Last data time : %02d-%02d-%02d:%02d\n",LastdataTime.year,\
	LastdataTime.mon, LastdataTime.day, LastdataTime.hour);

	printf("LastData Time %02d-%02d-%02d:%02d\n", LastdataTime.year, \
	LastdataTime.mon, LastdataTime.day, LastdataTime.hour);


	display_sector(1);
	//printf("display 0 Sector = %02d, uinit = %02d\n", sortMapdata[0]/5, sortMapdata[0]%5);
	//dataFlash_displayDataSector(sortMapdata[0]/5);

	dataTimeS_t tempNow;
	
	uint8 tempMode = 0;
	tempNow.year = 24;
	tempNow.mon = 6;
	tempNow.day = 13;
	tempNow.hour = 00;

#if 1
	tempLastA.year = 24;
	tempLastA.mon = 6;
	tempLastA.day = 11;
	tempLastA.hour = 17;
#else
	tempLastA.year = 0;
	tempLastA.mon = 0;
	tempLastA.day = 0;
	tempLastA.hour = 0;
#endif
#if 0
	printf("Last %02d-%02d-%02d %02dh\r\n", tempLastA.year, tempLastA.mon, tempLastA.day, tempLastA.hour);
	printf("Now %02d-%02d-%02d %02dh\r\n", tempNow.year, tempNow.mon, tempNow.day, tempNow.hour);
	tempMode = get_message_mode(tempNow, tempLastA);
	printf("temp Mode = %d\n", tempMode);
	meterList(tempNow, tempMode, NULL);
	#endif
	//TestPro(tempNow, tempLastA);
#if 0
	tempNow.mon = 1;
	tempNow.day = 1;
	tempNow.hour = 9;

	tempLastA.year = 23;
	tempLastA.mon = 12;
	tempLastA.day = 27;
	tempLastA.hour = 9;

	for(i=0; i<10; i++)
	{
		tempNow.day += 1;
		tempNow.hour++;
		TestPro(tempNow, tempLastA);
	}
#endif
	#if 0
	map_sector_t *pMap =  &g_Map;
	
	int a, b;
	a = 7;
	b = 2;
	printf("%d, %d cmp = %d\n", a, b, memcmp(&pMap->unitMap[a], &pMap->unitMap[b], sizeof(uint8)*3));
	printf("%d : %d-%d-%d\n", a, pMap->unitMap[a].year, pMap->unitMap[a].mon, pMap->unitMap[a].day);
	printf("%d : %d-%d-%d\n", b, pMap->unitMap[b].year, pMap->unitMap[b].mon, pMap->unitMap[b].day);

	a = 6;
	b = 10;
	printf("%d, %d cmp = %d\n", a, b, memcmp(&pMap->unitMap[a], &pMap->unitMap[b], sizeof(uint8)*3));
	printf("%d : %d-%d-%d\n", a, pMap->unitMap[a].year, pMap->unitMap[a].mon, pMap->unitMap[a].day);
	printf("%d : %d-%d-%d\n", b, pMap->unitMap[b].year, pMap->unitMap[b].mon, pMap->unitMap[b].day);
	#endif
	#if 0
	printf("data sector\r\n");
	for(i=0; i<2;i++)
	{
		dataFlash_displayDataSector(i);
	}
	#endif
	#if 0
	uint32 ffdata=0xFFFFFFFF;
	uint8 sample[4]={0,};
	dataTrans4_t testsample;
	sample[0] = 0xFF;
	sample[1] = 0xFF;
	sample[2] = 0xFF;
	sample[3] = 0xFF;
	testsample.data_b8[0] =0xFF;
	testsample.data_b8[1] =0xFF;
	testsample.data_b8[2] =0xFF;
	testsample.data_b8[3] =0xFF;
	int retdata = 0;
	
	printf("sample - 0x%02x%02x%02x%02x\n", sample[0], sample[1], sample[2], sample[3]);
	printf("ffdata = 0x%08lx\n", ffdata);
	printf("testrans = 0x%08lx\n", testsample.data_b32);
	retdata = memcmp(sample ,&ffdata, sizeof(uint32));
	printf("0xFFFFFFFF comp = %d\n", retdata);
#endif
	dataTimeS_t testnow;
	now.year = 2024;
	now.mon = 6;
	now.day = 14;
	now.hour = 11;
	now.min = 45;
	now.sec = 22;

	uint8 datapos1;
	testnow.year = now.year-2000;
	testnow.mon = now.mon;
	testnow.day = now.day;
	testnow.hour = now.hour;
	datapos1 = CheckSectMapSort(testnow);
	uint8 tempSect1 = datapos1 / 5;
    uint8 tempUnit1 = datapos1 % 5;
	data_sector_t *pData = &g_save_data[tempSect1];

	
	#if 1
	LastAckDate.year = 24;
	LastAckDate.mon = 6;
	LastAckDate.day = 13;
	LastAckDate.hour = 9;
	#else
	#if 1
	LastAckDate.year = LastdataTime.year;
	LastAckDate.mon = LastdataTime.mon;
	LastAckDate.day = LastdataTime.day;
	LastAckDate.hour = LastdataTime.hour; 
	#else
	LastAckDate.year = 0;
	LastAckDate.mon = 0;
	LastAckDate.day = 0;
	LastAckDate.hour = 0;
	#endif
	#endif

	printf("input storedata\n");
	memset(&StroeUnit, 0, sizeof(MeterUnitData_t));
	StroeUnit.year = now.year -2000;
	StroeUnit.mon = now.mon;
	StroeUnit.day = now.day;
	StroeUnit.hour = now.hour;
	StroeUnit.min = 45;
	StroeUnit.sec = 22;
	StroeUnit.isTimeSync = 0;
	StroeUnit.meterStatus = 0x00;
	#if 0
	StroeUnit.meterData[0]= 0xFF;
	StroeUnit.meterData[1]= 0xFF;
	StroeUnit.meterData[2]= 0xFF;
	StroeUnit.meterData[3]= 0xFF;
	#else
	StroeUnit.meterData[0]= pData->dataUnit[tempUnit1].data[StroeUnit.hour][0];
	StroeUnit.meterData[1]= pData->dataUnit[tempUnit1].data[StroeUnit.hour][1];
	StroeUnit.meterData[2]= pData->dataUnit[tempUnit1].data[StroeUnit.hour][2];
	StroeUnit.meterData[3]= pData->dataUnit[tempUnit1].data[StroeUnit.hour][3];
	#endif

	uint8 msg[1024];
	char tempdebug[1024]={0,};
	int tempdebugcnt = 0;
	int lens = 0;
	
	printf("\r\nver 1.6\n");
	lens = insert_multiData((NbiotMeterData_t *)msg);
	//lens = insert_multiData1((NbiotMeterData1_t *)msg);
	printf("length = %d\n", lens);
	msg[lens] = 0;
	//printf("msg:%s\n", msg);
	#if 1
	memset(tempdebug, 0, sizeof(tempdebug));
	for(int cnt = 0; cnt< lens; cnt++)
	{
		//tempdebugcnt = snprintf(tempdebug,512,"%s%02x",tempdebug, msg[cnt]);
		printf("%02x", msg[cnt]);
	}
	//tempdebug[tempdebugcnt] = 0;
	//printf("msg:%s\n", tempdebug);
	#endif
	printf("\r\nver 1.7\n");
	//lens = insert_multiData((NbiotMeterData_t *)msg);
	memset(msg, 0, sizeof(msg));
	lens = insert_multiData1((NbiotMeterData1_t *)msg);
	printf("length = %d\n", lens);
	msg[lens] = 0;
	//printf("msg:%s\n", msg);
	#if 1
	memset(tempdebug, 0, sizeof(tempdebug));
	for(int cnt = 0; cnt< lens; cnt++)
	{
		//tempdebugcnt = snprintf(tempdebug,512,"%s%02x",tempdebug, msg[cnt]);
		printf("%02x", msg[cnt]);
	}
	printf("\n");
	#endif
	return 0;
}

