#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define uint8 unsigned char
#define uint32 unsigned long
#define uchar unsigned char
#define uint16 unsigned short


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

data_sector_t g_save_data[NUM_DATA_SECTOR];
map_sector_t g_Map;
uint8 sortMapdata[90];

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

static int make_dateList(int nDay, date_sort_t *pSort, day_data_map_t *pDayMap)
{
	int nMon = -1;
	int mon = -1;
	int year = -1;

	for (unsigned int i = 0; i < nDay; i++) {
		date_sort_t *p = pSort + i;
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
	//uint32 buf32[SECTOR_SIZE / 4];
	map_sector_t *pMap =  &g_Map;//(map_sector_t *)buf32;

	date_sort_t SortedData[NUM_DATE_TO_STORE];
	memset(&SortedData, 0, sizeof(SortedData));

	int nDay = readAndSort_map(pMap, SortedData);
	if (nDay == 0) {
		return;
	}

	day_data_map_t dayDataMap;
	memset(&dayDataMap, 0, sizeof(dayDataMap));
	int nMonth = make_dateList(nDay, SortedData, &dayDataMap);

	for (int nSector = FIRST_DATA_SECTOR; nSector < NUM_DATA_SECTOR; nSector++) {
		printf("[sector %2d] ", nSector);
		for (int nUnit = 0; nUnit < UNIT_PER_DATA_SECTOR; nUnit++) {
			int pos = nSector * UNIT_PER_DATA_SECTOR + nUnit;
			unit_map_t *pUnit = &pMap->unitMap[pos];
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
void resetSortMap(void)
{
	//sortMapdata
	int i = 0, j=1;
	printf("reset sortmap \r\n%d : ", j++);
	for(i=0; i<90; i++)
	{
		sortMapdata[i]=89-i;
		printf("%d ", sortMapdata[i]);
		if(((i+1)%10)==0)
			printf("\n%d : ", j++);
	}
}
void sortMapSector(void)
{
	map_sector_t *pMap =  &g_Map;
	int i, j;
	unit_map_t tempMap;
	int tempComp = 0;
	uint8 tempvalue;
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
			tempComp =  memcmp(&pMap->unitMap[sortMapdata[i]], &pMap->unitMap[sortMapdata[j]], sizeof(uint8)*3);
			if(tempComp < 0){
				tempvalue = sortMapdata[i];
				sortMapdata[i] = sortMapdata[j];
				sortMapdata[j] = tempvalue;
			}
		}
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
	uint16 nowDays=0, LastDays=0;
	char tempTime = 0;
	
	nowDays = dateToDays(now);
	LastDays = dateToDays(LastAck);
	//printf("now days : %d, Last days : %d \n", nowDays, LastDays);
	printf("diff days : %d \n", nowDays - LastDays);
	nowDays -= LastDays;
	//printf("diff : %d\n", nowDays);
	tempTime = now.hour-LastAck.hour;
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
	dataTimeS_t ret = {0,};
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
	
	memcpy(&ret, &Indata, sizeof(dataTimeS_t));
	return ret;
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
	return len;
}

dataTimeS_t tempLastA;

uint8 meterList(dataTimeS_t Indata, dataTimeS_t LastAck, uint8 mode, uint32 *data)
{
	//int i;
	uint8 ret = 0, tempSect, tempUnit;
	uint8 datapos = 0, timefirst = 0;
	
	data_sector_t *pData =NULL;
	dataTimeS_t tempdate = Indata;
	uint8 Len =0;
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
			break;
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
			printf("%02d-%02d-%02d:%02d [%02d] %02x%02x%02x%02x \n",\
			pData->dataUnit[tempUnit].year, pData->dataUnit[tempUnit].mon, pData->dataUnit[tempUnit].day, \
			tempdate.hour, pData->dataUnit[tempUnit].caliber_dp, pData->dataUnit[tempUnit].data[tempdate.hour][0], \
			pData->dataUnit[tempUnit].data[tempdate.hour][1], pData->dataUnit[tempUnit].data[tempdate.hour][2], pData->dataUnit[tempUnit].data[tempdate.hour][3]);

			bcd2int(pData->dataUnit[tempUnit].data[tempdate.hour], data, 4);
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
	count = CheckMeterDataCount(nowdate, tempLastA, tempMode);
	printf("check count = %d\n", count);
	tempData = (uint32 *)malloc(sizeof(uint32)*count);
	memset(tempData, 0, sizeof(uint32)*count);
	count = meterList(nowdate, tempLastA, tempMode, tempData);
	printf("data count = %d\n", count);
	p = tempData;
	for(i=0; i<count; i++)
	{
		printf("%02d: %ld\r\n", i, *p++);
		//p++;
	}
	free(tempData);
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

	memset(g_save_data, 0, sizeof(data_sector_t)*NUM_DATA_SECTOR);
	memset(&g_Map, 0, sizeof(map_sector_t));
	srand(time(NULL));
	printf("start save data input\n");
	m=23;
	j= 12;
	k=27;
	for(i=0; i< 90*24; i++)
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
		#if 1
		EndianTrans4b(save_data[i].meterValue, meter_data.data_b8);
		#else
		save_data[i].meterValue[0] = meter_data.data_b8[3];
		save_data[i].meterValue[1] = meter_data.data_b8[2];
		save_data[i].meterValue[2] = meter_data.data_b8[1];
		save_data[i].meterValue[3] = meter_data.data_b8[0];
		#endif
		saveData(&save_data[i]);
	}
	printf("finish save data input\n");
	
	dataFlash_displayMapSector();
	
	resetSortMap();
	display_sector(0);
	sortMapSector();
	display_sector(1);
	//printf("display 0 Sector = %02d, uinit = %02d\n", sortMapdata[0]/5, sortMapdata[0]%5);
	//dataFlash_displayDataSector(sortMapdata[0]/5);

	dataTimeS_t tempNow;
	
	uint8 tempMode = 0;
	tempNow.year = 24;
	tempNow.mon = 1;
	tempNow.day = 1;
	tempNow.hour = 6;

	tempLastA.year = 23;
	tempLastA.mon = 12;
	tempLastA.day = 29;
	tempLastA.hour = 6;

#if 0
	printf("Last %02d-%02d-%02d %02dh\r\n", tempLastA.year, tempLastA.mon, tempLastA.day, tempLastA.hour);
	printf("Now %02d-%02d-%02d %02dh\r\n", tempNow.year, tempNow.mon, tempNow.day, tempNow.hour);
	tempMode = get_message_mode(tempNow, tempLastA);
	printf("temp Mode = %d\n", tempMode);
	meterList(tempNow, tempMode, NULL);
	#endif
	TestPro(tempNow, tempLastA);
#if 1
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
	

	return 0;
}
