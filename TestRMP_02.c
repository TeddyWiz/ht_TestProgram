#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define uint16 unsigned short
#define uint8	unsigned char
#define uint32 unsigned int
#define uchar unsigned char

#define YEAR_MIN 2017
#define YEAR_MAX 2100

#define TRUE 1
#define FALSE 0

#define CAL_COUNT_TIME_UNIT(x) 		(x<25? 1: x<49? 2: x<73? 3: x<97? 4: x<145? 6: x<289? 12: 24)

#define MAXCOUNT 	70000//7000
//#define REPORT_VERSION 0xA4
#define REPORT_VERSION 0xA4
#define REPORT_TIME 1
//structure
typedef struct MeterData{
	uint16 num;
	//uint8 cal;
	//uint8 status;
	uint8 meterData[4];
	uint8 temperature[2];
	uint8 pressure[2];
	struct MeterData *Next;
}MeterData;


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
	uint16 nData;
	uint8 saveInterval;
	uint8 caliberDp;
	//uint8 meterStatus;
	uint8 dif;
	uint8 vif;
	uint8 meterSerial[4];
	MeterUnitData_t unit;
} MeterStoredData_t;

typedef struct {
	uint8 year;
	uint8 mon;
	uint8 day;
	uint8 hour;
	uint8 min;
	uint8 sec;
	uint8 isTimeSync;
} MeterTimeData_t;


typedef union{
	uint32 data_b32;
	uint8 data_b8[4];
} dataTrans4_t;

typedef union{
	uint16 data_b16;
	uint8 data_b8[2];
} dataTrans2_t;

typedef struct {
	uchar interval;
	uchar numData;
	uchar refValuePos;
} NbiotMeterDataHigh_t;

typedef struct {
	uint16 diff[24];
} NbiotMeterDataV16_t;

typedef struct {
	uint32 Val[24];
} NbiotMeterDataV17_t;
#if 0
typedef struct {
	uchar interval;
	uchar numData;
	uchar refValuePos;
	//uchar refValue[4];
	uchar valueDiff[24][4];
} NbiotMeterData1_t; //seoul protocol version 1.7
#endif
//gloval data
MeterData *HeadNode = NULL;
MeterStoredData_t StoredMeterData;
MeterTimeData_t meterPreTime;
MeterTimeData_t meterSaveTime;


int tempSaveNum[5001];
int tempSaveindex=0;
Date_t now={.year=2024, .mon=7, .day=22, .hour=0, .min=0, .sec=1};
//Linked List Function
// 새로운 노드 생성 함수
MeterData *createMeterData(uint16 num) {
    MeterData* newNode = (MeterData*)malloc(sizeof(MeterData));
    newNode->num = num;
    newNode->Next = NULL;
    return newNode;
}

void appendMeterData(MeterData** head, uint16 data) {
    MeterData *newNode = createMeterData(data);
    if (*head == NULL) {
        *head = newNode;
        return;
    }
    MeterData *temp = *head;
    while (temp->Next != NULL) {
        temp = temp->Next;
    }
    temp->Next = newNode;
}

char addHeadMeterData(MeterData** head, uint16 data) {
	MeterData *newNode = createMeterData(data);
	//printf("New Addr : 0x%lx\n", newNode);
	//printf("head Addr : 0x%lx\n", head);
	if (*head == NULL) {
        *head = newNode;
        return 1;
    }
	newNode->Next = *head;
	*head = newNode;
	return 0;
}

#if 1
void allClearMeterData(void)
{
	StoredMeterData.nData = 1;
	MeterData *Temp;
	while(HeadNode != NULL)
	{
		Temp = HeadNode;
		HeadNode = Temp->Next;
		free(Temp);
	}
}
#else
void allClearMeterData(MeterData **head)
{
	StoredMeterData.nData = 1;
	MeterData *Temp;
	while(*head != NULL)
	{
		Temp = *head;
		*head = Temp->Next;
		free(Temp);
	}
}
#endif

void RemoveLastMeterData(MeterData **head)
{
	MeterData *Temp = *head;
	MeterData *pre = NULL;
	while(Temp->Next != NULL)
	{
		pre = Temp;
		Temp = Temp->Next;
	}
	printf("  Remove Last data = %d\n",Temp->num);
	free(Temp);
	pre->Next = NULL;
}

MeterData *findMeterData(uint16 data) {
	MeterData *temp = HeadNode;
	if(temp == NULL)
		return NULL;
	
	while(temp != NULL)
	{
		if(temp->num == data)
		{
			return temp;
		}
		temp = temp->Next;
	}
	return NULL;
}
#if 1
int findNumData(uint8 div, uint16 *dataNum)
{
	MeterData *temp = HeadNode;
	uint16 *tempRet = dataNum;

 	int count = 0;

	while(temp != NULL)
	{
		if((div == 24) &&(count > 23))
		{
			printf("over data remove\n");
			RemoveLastMeterData(&temp);
			break;
		}
		if((temp->num % div) == 0)
		{
			*tempRet = temp->num;
			tempRet++;
			count++;
		}
		temp = temp->Next;
	}
	return count;
}
#else
int findNumData(MeterData **head, uint8 div, uint16 *dataNum)
{
	MeterData *temp = *head;
	uint16 *tempRet = dataNum;

 	int count = 0;

	while(temp != NULL)
	{
		if((div == 24) &&(count > 23))
		{
			printf("over data remove\n");
			RemoveLastMeterData(head);
			break;
		}
		if((temp->num % div) == 0)
		{
			*tempRet = temp->num;
			tempRet++;
			count++;
		}
		temp = temp->Next;
	}
	return count;
}
#endif

char findRemoveData(MeterData **head, uint16 data)
{
	MeterData *temp = *head;
	MeterData *prev = NULL;

    // 헤드 노드가 삭제할 데이터인 경우
    if (temp != NULL && temp->num == data) {
        *head = temp->Next; // 헤드를 다음 노드로 변경
        free(temp); // 현재 헤드 메모리 해제
        return 1;
    }

    // 삭제할 데이터를 찾을 때까지 리스트 탐색
    while ((temp != NULL ) && (temp->num != data)) {
        prev = temp;
        temp = temp->Next;
    }

    // 데이터가 리스트에 없는 경우
    if (temp == NULL) return 2;

    // 노드를 리스트에서 제거
    prev->Next = temp->Next;
    free(temp); // 노드 메모리 해제
	return 0;
}

void printList(MeterData* head) {
	int cnt = 0;
    MeterData* temp = head;
    while (temp != NULL) {
        printf("[%d]%d -> ", cnt++, temp->num);
        temp = temp->Next;
    }
    printf("NULL\n");
}
#if 1
void printMeterData(void){
	int cnt = 0;
    MeterData* temp = HeadNode;
	uint16 temp16buff;
	printf("==== print MeterData ====\n");
    while (temp != NULL) {
		memcpy(&temp16buff, temp->pressure, sizeof(temp->pressure));
        printf("[%3d]%3d %02X%02X%02X%02X[%02X%02X|%2d]\n", cnt++, temp->num,   \
		temp->meterData[0], temp->meterData[1],temp->meterData[2],temp->meterData[3],\
		temp->temperature[0], temp->temperature[1], temp16buff );
        temp = temp->Next;
    }
    //printf("NULL\n");
}
#else
void printMeterData(MeterData* head){
	int cnt = 0;
    MeterData* temp = head;
	uint16 temp16buff;
	printf("==== print MeterData ====\n");
    while (temp != NULL) {
		memcpy(&temp16buff, temp->pressure, sizeof(temp->pressure));
        printf("[%3d]%3d %02X%02X%02X%02X[%02X%02X|%2d]\n", cnt++, temp->num,   \
		temp->meterData[0], temp->meterData[1],temp->meterData[2],temp->meterData[3],\
		temp->temperature[0], temp->temperature[1], temp16buff );
        temp = temp->Next;
    }
    //printf("NULL\n");
}
#endif

uint8 CheckSaveMeterData(uint16 num)
{
	uint8 saveFlag = 0;
	int n = (num/12);
	uint8 p1[12]={1, 5, 7, 11, 2, 10, 3, 9, 4, 8, 6, 12};
	uint8 j;
	uint16 i;
	if(num<25)
		return 1;
	else if(num==25){
		//p1={1,5,7,11} + 12*n  remove
		printf("Step 1: remove - ");
		for(j=0; j<4; j++)
		{
			for(i=0; i<n; i++)
			{
				printf("%d  ", p1[j]+12*i);
				findRemoveData(&HeadNode, p1[j]+12*i);
			}
		}
		printf("\n");
	}
	else if(num<49){
		//p1={1,5,7,11} + 12*n pass
		saveFlag = 1;
		for(j=0; j<4; j++)
		{
			if(num == (p1[j]+12*n))
			{
				return 0;
			}
		}
	}
	else if(num==49){
		//p2={2,10} + 12*n pass remove 4-6
		printf("Step 2: remove - ");
		for(j=4; j<6; j++)
		{
			for(i=0; i<n; i++)
			{
				printf("%d  ", p1[j]+12*i);
				findRemoveData(&HeadNode, p1[j]+12*i);
			}
		}
		printf("\n");
	}
	else if(num<73){
		//p1 , p2 + 12*n pass 0-6 
		saveFlag = 1;
		for(j=0; j<6; j++)
		{
			if(num == (p1[j]+12*n))
			{
				return 0;
			}
		}
	}
	else if(num==73){
		//p3={3, 9} + 12*n remove 6-8
		printf("Step 3: remove - ");
		for(j=6; j<8; j++)
		{
			for(i=0; i<n; i++)
			{
				printf("%d  ", p1[j]+12*i);
				findRemoveData(&HeadNode, p1[j]+12*i);
			}
		}
		printf("\n");
	}
	else if(num<97){
		//p4={4, 8}, p5=6 p6=12 + 12*n save 8-12
		for(j=8; j<11; j++)
		{
			if(num == (p1[j]+12*n))
			{
				return 1;
			}
		}
		if(num == (12*n))
		{
			return 1;
		}
	}
	else if(num==97){
		//p4 + 12*n remove 8-10
		printf("Step 4: remove - ");
		
		for(j=8; j<10; j++)
		{
			for(i=0; i<n; i++)
			{
				printf("%d  ", p1[j]+12*i);
				findRemoveData(&HeadNode, p1[j]+12*i);
			}
		}
		printf("\n");
	}
	else if(num<145){
		//p5 p6 + 12*n save 10-12
		if((num == (p1[10]+12*n))||(num == 12*n)){
			return 1;
		}
	}
	else if(num==145){
		//p5 + 12*n remove 10
		printf("Step 5: remove - ");
		for(i=0; i<n; i++)
		{
			printf("%d  ", p1[10]+12*i);
			findRemoveData(&HeadNode, p1[10]+12*i);
		}
		printf("\n");
	}
	else if(num<289){
		//p6 + 12*n save
		if(num == 12*n){
			return 1;
		}
	}
	else if(num==289){
		//p6 + 12*n remove 11
		printf("Step 6: remove - ");
		for(i=0; i<n; i++)
		{
			printf("%d  ", p1[11]+24*i);
			findRemoveData(&HeadNode, p1[11]+24*i);
		}
		printf("\n");
	}
	else if(num<577){
		//nData%24 == 0 save
		if((num%24)==0)
			return 1;
	}
	else if(num>=30000)//30000)
	{
		saveFlag = 1;
		StoredMeterData.nData = 600; //24*24
		RemoveLastMeterData(&HeadNode);
	}
	else	// over 552
	{
		//nData%24 == 0 save and remove Last data
		if((num%24)==0)
		{
			//remove Last data
			//printf("remove %d\n", num);
			RemoveLastMeterData(&HeadNode);
			return 1;
		}	
	}
	return saveFlag;
}


int RTC_isValidDate(Date_t *date)
{
	int valid = 0;

	do {
		if (date->year < YEAR_MIN || date->year > YEAR_MAX) {
			break;
		}

		if (date->mon < 1 || date->mon > 12) {
			break;
		}

		if (date->day < 1 || date->day > 31) {
			break;
		}

		if (date->hour > 23 || date->min > 59 || date->sec > 59) {
			break;
		}

		valid = 1;

	} while (0);

	return valid;
}

//void insertDateToData(Date_t *pDate, MeterTimeData_t *pData, uint8 isIgnoreSec)
void insertDateToData(Date_t *pDate, MeterUnitData_t *pData, uint8 isIgnoreSec)
{
	if (RTC_isValidDate(pDate)) {
		// Data의 year는 RTC의 year에서 2000을 빼서 사용
		pData->year = pDate->year - 2000;
		pData->mon = pDate->mon;
		pData->day = pDate->day;
		pData->hour = pDate->hour;
		pData->min = pDate->min;
		pData->sec = (isIgnoreSec == TRUE) ? 0 : pDate->sec;
		//pData->isTimeSync = RTC_isTimeSync();
	}
}

int saveMeterTime(uint16 num, Date_t now)
{
	uint8 saveFlag = 0;
	insertDateToData(&now, (MeterUnitData_t *)&meterPreTime, 0);
	if(num < 24){
		saveFlag = 1;
	}
	else if(num < 48){
		if((num%2)==0){
			saveFlag = 1;
		}
	}
	else if(num < 72){
		if((num%3)==0){
			saveFlag = 1;
		}
	}
	else if(num < 96){
		if((num%4)==0){
			saveFlag = 1;
		}
	}
	else if(num < 144){
		if((num%6)==0){
			saveFlag = 1;
		}
	}
	else if(num < 288){
		if((num%12)==0){
			saveFlag = 1;
		}
	}
	else{
		if((num%24)==0){
			saveFlag = 1;
		}
	}
	if(saveFlag){
		insertDateToData(&now, (MeterUnitData_t *)&meterSaveTime, 0);
		return 1;
	}
	else{
		return 0;
	}

	return 0;
}


void Unit2AddMeterData(MeterUnitData_t *pUnit)
{
	uint8 saveFlag = 1;
	
	saveFlag = CheckSaveMeterData(StoredMeterData.nData);
	if(saveMeterTime(StoredMeterData.nData, now) ==1)
	{
		#if 0
		printf("now Time : %2d-%2d-%2d %2d:%2d:%2d \n", now.year, now.mon, now.day,\
		now.hour, now.min, now.sec);
		printf("save Time : %2d-%2d-%2d %2d:%2d:%2d\n", meterSaveTime.year, \
		meterSaveTime.mon, meterSaveTime.day, meterSaveTime.hour, meterSaveTime.min,\
		meterSaveTime.sec );
		#endif
	}
	if(saveFlag)
	{
		#if 0
		now.hour= (StoredMeterData.nData/60);
		now.min = (StoredMeterData.nData%60);
		if(now.hour > 24)
		{
			now.day = now.hour/24;
			now.hour = now.hour % 24;
		}
		#endif

		//tempSaveNum[tempSaveindex++] = StoredMeterData.nData;
		//if(StoredMeterData.nData != 576)
		printf("s: ");
		addHeadMeterData(&HeadNode, StoredMeterData.nData);
		
		//HeadNode->cal = StoredMeterData.caliberDp;
		//HeadNode->status = StoredMeterData.meterStatus;
		memcpy(HeadNode->meterData, pUnit->meterData, sizeof(uint8)*4);
		memcpy(HeadNode->temperature, pUnit->temperature, sizeof(uint8)*4);
		#if 0
		if(StoredMeterData.nData>24)
		{
			printf("==== num : %d ====\n", StoredMeterData.nData);
			printMeterData(HeadNode);
		}
		#endif
	}
	else
		printf("   ");
	StoredMeterData.nData++;
}

/// @brief integer를 BCD로 변환
/// @param pBCD 
/// @param pInt 
/// @param nDigit 
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

void bcd2int(uint8 *pBCD, uint32 *pInt, uint8 nDigit)
{
	unsigned char i = 0;
	*pInt = 0;
	for(i=0; i<nDigit; i++)
	{
		*pInt *= 100;
		*pInt += ((*pBCD & 0xf0) >> 4) * 10 + (*pBCD & 0x0f);
		pBCD++;
	}
	
}

uint8 METER_isAllFF(uchar *p, int len)
{
	uint8 result = TRUE;

	for (int i = 0; i < len; i++) {
		if (*(p + i) != 0xFF) {
			result = FALSE;
			break;
		}
	}
	return result;
}

int insert_multiData(uchar *p, uint8 proto)
{
	uint8 i=0, datacnt =0;
	uint32 prevValue = 0, nowValue = 0;
	uint16 diff=0;

	NbiotMeterDataHigh_t *temp_p = (NbiotMeterDataHigh_t *)p;
	MeterData *tempMeterData;
	uchar *temp_add = NULL;
	uint32 *refValue = NULL;
	uint16 findNum[24]={0,};

	//interval meter mode
	printf("NbiotMeterDataHigh_t size = %d\n", (int)sizeof(NbiotMeterDataHigh_t));
	printf("nData count = %d\n", StoredMeterData.nData-1);
	temp_p->interval = CAL_COUNT_TIME_UNIT(StoredMeterData.nData-1);
	temp_p->numData = findNumData(temp_p->interval, findNum);
	printf("find data num = ");
	for(i=0; i<temp_p->numData; i++)
	{
		printf("%d, ", findNum[i]);
	}
	printf("\n");
	
	if(proto == 0xA4)
	{
		temp_add = p + sizeof(NbiotMeterDataHigh_t);
	}
	else //if (proto == 0xA3)
	{
		refValue = (uint32 *)(p + sizeof(NbiotMeterDataHigh_t));
		temp_add = p + sizeof(NbiotMeterDataHigh_t) + 4;
	}
	printf("high size = %d\n", (int)(temp_add - p));

// Maxnum > 0
	tempMeterData = findMeterData(findNum[datacnt++]);
	if (METER_isAllFF(tempMeterData->meterData, 4) == FALSE) {
		if (proto == 0xA4){
			bcd2int(tempMeterData->meterData, (uint32 *)temp_add, 4);
			printf("[%4d %02X%02X%02X%02X %8u]\n",tempMeterData->num ,tempMeterData->meterData[0],tempMeterData->meterData[1],tempMeterData->meterData[2],tempMeterData->meterData[3], *(uint32 *)temp_add);
			printf("[%02X%02X%02X%02X]\n",*(temp_add),*(temp_add+1),*(temp_add+2),*(temp_add+3)); 
			temp_add += 4;
		}
		else //if(proto == 0xA3)
		{
			bcd2int(tempMeterData->meterData, &prevValue, 4);
			*refValue = prevValue; 
			memset(temp_add, 0x00, 2);
			temp_add += 2;
		}
		temp_p->refValuePos = 0;
	}
	else
	{
		if (proto == 0xA4){
			memset(temp_add, 0xFF, 4);
			temp_add += 4;
		}
		else	// proto == 0xA3
		{
			memset(temp_add, 0xFF, 2);
			temp_add += 2;
		}
		for(i=datacnt;  i<temp_p->numData; i++){
			tempMeterData = findMeterData(findNum[datacnt++]);
			if (METER_isAllFF(tempMeterData->meterData, 4)) {
				// meter down
				if (proto == 0xA4){
					memset(temp_add, 0xFF, 4);
					temp_add += 4;
				}
				else	// proto == 0xA3
				{
					memset(temp_add, 0xFF, 2);
					temp_add += 2;
				}
			}
			else
				break;
		}
		temp_p->refValuePos = datacnt - 1;
		if (proto == 0xA4){
			bcd2int(tempMeterData->meterData, (uint32 *)temp_add, 4);
			temp_add += 4;
		}
		else	// proto == 0xA3
		{
			bcd2int(tempMeterData->meterData, &prevValue, 4);
			*refValue = prevValue;
			memset(temp_add, 0x00, 2);
			temp_add += 2;
		}
	}
	//printf("ref pos :%d\n", temp_p->refValuePos);
	for(i=datacnt; i<temp_p->numData; i++)
	{
		tempMeterData = findMeterData(findNum[datacnt++]);
		if(METER_isAllFF(tempMeterData->meterData, 4))
		{
			if (proto == 0xA4){
				memset(temp_add, 0xFF, 4);
				temp_add += 4;
			}
			else	// proto == 0xA3
			{
				memset(temp_add, 0xFF, 2);
				temp_add += 2;
			}
		}
		else
		{
			if (proto == 0xA4){
				bcd2int(tempMeterData->meterData, (uint32 *)temp_add, 4);
				temp_add += 4;
			}
			else	// proto == 0xA3
			{
				nowValue = 0;
				bcd2int(tempMeterData->meterData, &nowValue, 4);
				if(prevValue < nowValue)
				{
					memset(temp_add, 0, 2);
					temp_add += 2;
					//printf("[%2d] pre : %8ld, now : %8ld\r\n", i, prevValue, nowValue);
				}
				else
				{
					diff = (prevValue - nowValue)>0xFFFE ? 0xFFFE : (uint16)(prevValue - nowValue);
					//printf("%02d : %6d | pre : %8ld, now : %8ld\r\n", i, diff, prevValue, nowValue);
					prevValue = nowValue;
					memcpy(temp_add, &diff, 2);
					temp_add += 2;
				}
			}
			
		}
	}

	printf("position : %d\n", temp_p->refValuePos);
	printf("data count : %2d\n", temp_p->numData);
	#if 0
	if(proto==0xA3)
	{
		printf("refVelue : %08d\ndata count:%2d\n", *refValue, temp_p->numData);
		printf("diff data : \n");
		temp_add = refValue + 4;
		for(i=0; i<temp_p->numData; i++)
		{
			printf("%d: %02x%02x\n", i,  );
		}
		printf("\n");
	}
	#endif
	
	//OSAL_free(loadData);
	if(proto == 0xA4)
	{
		return 3 + (temp_p->numData * 4); // 3 = interval(1), nData(1), validPos(1)
	}
	else
	{
		return 7 + (temp_p->numData * 2); // 7 = interval(1), nData(1), validPos(1), refValue(4)
	}

}

int main(int argc, char *argv[])
{
	
	dataTrans4_t meter_data;
	dataTrans2_t *Temp2Trans;
	
	MeterUnitData_t tempUnit;
	MeterData *tempMeterData;
	NbiotMeterDataHigh_t *tempmsgHigh;
	NbiotMeterDataV16_t *tempmsg16;
	NbiotMeterDataV17_t *tempmsg17;
	uint32 *tempRefVal = NULL;
	uint16 findNum[24]={0,};
	int dataMaxNum =0;
	uint32 meter_add = 0, temp_trans = 0;
	unsigned long i =0;
	int j=0;
	uint32 temp2byte=0;
	uint8 meteringData[4];
	uint16 temp16buff;
	uchar *buff = NULL;
	uchar *sendBuff = NULL;
	uint8 nowProto = REPORT_VERSION;
	StoredMeterData.nData = 1;
	uint16 temprval, pressval;
	uint8 ReportTime = REPORT_TIME;
	
	int result[500] ={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 
		24, 26, 27, 28, 30, 32, 33, 34, 36, 38, 39, 40, 42, 44, 45, 46,
		48, 51, 52, 54, 56, 57, 60, 63, 64, 66, 68, 69, 72, 76, 78, 80, 
		84, 88, 90, 92, 96, 102, 108, 114, 120, 126, 132, 138, 144, 156,
		168, 180, 192, 204, 216, 228, 240, 252, 264, 276, 288, 312, 336,
		360, 384, 408, 432, 456, 480, 504, 528, 552};
	char resultmatch= 0;
#if 0
	uint8 bcd[4]={0x12, 0x34, 0x56, 0x78};
    uint8 bcd1[4]={0x23, 0x45, 0x67, 0x89};
    uint8 msg[12]= {0,};
	uint8 *p = msg;
    uint32 *temp = NULL;
    temp = (uint32 *)p;
    //bcd2int(bcd, temp, 4);
	bcd2int(bcd, (uint32 *)p, 4);
    printf("1 0x%02X%02X%02X%02X-%5ld-%02X%02X%02X%02X\n", bcd[0], bcd[1], bcd[2], bcd[3], *temp, msg[0], msg[1], msg[2], msg[3] );
    
    printf("2 0x%02X%02X%02X%02X-%5ld-%02X%02X%02X%02X\n", bcd1[0], bcd1[1], bcd1[2], bcd1[3], *temp, msg[4], msg[5], msg[6], msg[7] );
	p+=4;
    temp = (uint32 *)p;
    bcd2int(bcd1, temp, 4);
	//bcd2int(bcd1, (uint32 *)p, 4);
    printf("3 0x%02X%02X%02X%02X-%5ld-%02X%02X%02X%02X\n", bcd1[0], bcd1[1], bcd1[2], bcd1[3], *temp, msg[4], msg[5], msg[6], msg[7]);

	printf("end\n");
	#endif
#if 1
	printf("protocol Version %02X / Report Time %2d\n", nowProto, ReportTime);
	for(i=0; i<MAXCOUNT; i++)
	{
		//add meterdata
		now.hour= ((i+1)/60)%24;
		now.min = ((i+1)%60);
		now.day = 1+(((i+1)/60)/24)%30;
		now.mon = 1+((((i+1)/60)/24)/30)%12;
		//addHeadMeterData(&HeadNode, i);
		meter_add += (unsigned int)((rand()+i)%100);
		temp_trans = 0;
		int2bcd(&temp_trans, &meter_add, 8);
		meter_data.data_b32 = temp_trans;
		for(j=0;j<4;j++)
		{
			tempUnit.meterData[j] = meter_data.data_b8[3-j];
		}
		//printf("%4ld) %02X%02X%02X%02X - %8d\n",i ,tempUnit.meterData[0], tempUnit.meterData[1], tempUnit.meterData[2], tempUnit.meterData[3], meter_add);
		temp2byte = 20 + (int)((rand()+i)%10);
		temprval = temp2byte;
		int2bcd(&temp_trans, &temp2byte, 4);
		tempUnit.temperature[0] = (temp_trans >> 8) & 0x000000FF;
		tempUnit.temperature[1] = (temp_trans) & 0x000000FF;
		temp2byte = 10 + (int)((rand()+i)%10);
		pressval = temp2byte;
		memcpy(tempUnit.pressure, &temp2byte, sizeof(uint8)*2);
#if 0
		if(i==3005)
			//printMeterData(HeadNode);
			#endif
		#if 0
		if(((i+1)%23)==0)
		{
			memset(tempUnit.meterData,0xFF, sizeof(uint8)*4);
			memset(tempUnit.pressure, 0xFF, sizeof(uint8)*2);
			memset(tempUnit.temperature, 0xFF, sizeof(uint8)*2);
		}
		if(((i+1)%24)==0)
		{
			memset(tempUnit.meterData,0xFF, sizeof(uint8)*4);
			memset(tempUnit.pressure, 0xFF, sizeof(uint8)*2);
			memset(tempUnit.temperature, 0xFF, sizeof(uint8)*2);
		}
		if(((i+1)%30)==0)
		{
			memset(tempUnit.meterData,0xFF, sizeof(uint8)*4);
			memset(tempUnit.pressure, 0xFF, sizeof(uint8)*2);
			memset(tempUnit.temperature, 0xFF, sizeof(uint8)*2);
		}
		#endif

		Unit2AddMeterData(&tempUnit);
		memcpy(&temp16buff, HeadNode->pressure, sizeof(HeadNode->pressure));
		#if 0
		printf("%3d[%3d] : %2d/%2d %2d:%2d | %02X%02X%02X%02X[%02X%02X|%2d]\n", i, StoredMeterData.nData - 1, \
		now.mon, now.day, now.hour, now.min,\
		HeadNode->meterData[0], HeadNode->meterData[1], HeadNode->meterData[2], HeadNode->meterData[3],
		HeadNode->temperature[0], HeadNode->temperature[1],temp16buff);
		#else
		printf("%4ld[%4d] : %2d/%2d %2d:%2d | %8d[%2d|%2d]\n", i, StoredMeterData.nData - 1, \
		now.mon, now.day, now.hour, now.min,\
		meter_add, temprval, pressval);
		#endif

		
		#if 0
		HeadNode->cal = 3;
		HeadNode->status = 0x01;
		temp2byte = 20 + (int)(rand()%10);
		HeadNode->temperature[0]=0;
		HeadNode->temperature[1]=temp2byte;
		//sprintf(HeadNode->temperature,"%2d", temp2byte);
		temp2byte = 10 + (int)(rand()%10);
		HeadNode->pressure[0]=0;
		HeadNode->pressure[1]=temp2byte;
		//sprintf(HeadNode->pressure,"%2d", temp2byte);
		printf("%3d : %d/%d %02X%02X%02X%02X[%2d|%2d]\n", i, HeadNode->status, HeadNode->cal,\
		HeadNode->meterData[0], HeadNode->meterData[1], HeadNode->meterData[2], HeadNode->meterData[3],
		HeadNode->pressure[1], HeadNode->temperature[1]);
		#endif
		
		if(((i+1)%ReportTime)==0)
		{
			printf("===== Make message ====\n");
			buff = (uchar *)malloc(256);
			sendBuff = buff;
			sendBuff +=insert_multiData(sendBuff, nowProto);
			*sendBuff = 0xAE;
			sendBuff++;
			*sendBuff = 0xBC;
			sendBuff++;
			printf("message : ");
			for(j=0; j<(int)(sendBuff-buff); j++)
			{
				printf("%02X", buff[j]);
			}
			printf("\n");
			tempmsgHigh = (NbiotMeterDataHigh_t *)buff;
			printf("Interval : %d\n", tempmsgHigh->interval);
			printf("numData : %d\n", tempmsgHigh->numData);
			printf("refPos : %d\n", tempmsgHigh->refValuePos);
			printf("save Time : %2d-%2d-%2d %2d:%2d:%2d\n", meterSaveTime.year, \
		meterSaveTime.mon, meterSaveTime.day, meterSaveTime.hour, meterSaveTime.min,\
		meterSaveTime.sec );
			if(nowProto == 0xA4)
			{
				tempmsg17 = (NbiotMeterDataV17_t *)(buff + sizeof(NbiotMeterDataHigh_t));
				for(j=0; j<tempmsgHigh->numData; j++)
				{
					printf("%4d : %8d[0x%08x]\n", j, (tempmsg17->Val[j]&0x00FFFFFFFF), (tempmsg17->Val[j]&0x00FFFFFFFF));
				}
			}
			else	// 0xA3
			{
				
				tempmsg16 = (NbiotMeterDataV16_t *)(buff + 4 + sizeof(NbiotMeterDataHigh_t));
				tempRefVal = (uint32 *)(buff + sizeof(NbiotMeterDataHigh_t));
				printf("refVal : %8d[0x%08X]\n", (*tempRefVal & 0xFFFFFFFF), (*tempRefVal & 0xFFFFFFFF));
				for(j=0; j<tempmsgHigh->numData; j++)
				{
					*tempRefVal -= tempmsg16->diff[j];
					printf("%4d : %8d|%4d[0x%04x]\n", j, (*tempRefVal & 0xFFFFFFFF), tempmsg16->diff[j], tempmsg16->diff[j]);
				}
			}

			free(buff);
			printf("===== End Message ====\n");
		}
	}
	//printMeterData(HeadNode);
	printMeterData();
	#if 0
	printf("Result Check match\n");
	for(i=0; i<tempSaveindex; i++)
	{
		if(tempSaveNum[i] != result[i])
		{
			printf("Not match %d, %d\n", tempSaveNum[i], result[i]);
			resultmatch =1;
		}
	}
	printf("Result Match : %s\n", resultmatch == 0 ? "Success" : "Fail");
	dataMaxNum = findNumData(&HeadNode, 2, findNum);
	printf("find data = %d\n", dataMaxNum);
	#if 1
	for(i=0; i<dataMaxNum; i++)
	{
		tempMeterData = findMeterData(HeadNode, findNum[i]);
		printf("%3d[%3d] : %02X%02X%02X%02X\n", i, tempMeterData->num, tempMeterData->meterData[0], \
		tempMeterData->meterData[1], tempMeterData->meterData[2], tempMeterData->meterData[3]);
	}
	#endif
	#endif
	//allClearMeterData(&HeadNode);
	allClearMeterData();
	printList(HeadNode);

#endif
	#if 0 //function Test
	//createMeterData(1);
	addHeadMeterData(&HeadNode, 0);
	addHeadMeterData(&HeadNode, 1);
	addHeadMeterData(&HeadNode, 2);
	//printf("Next Addr : 0x%lx\n", HeadNode->Next);
	addHeadMeterData(&HeadNode, 3);
	//printf("Next Addr : 0x%lx\n", HeadNode->Next);
	addHeadMeterData(&HeadNode, 4);
	//printf("Next Addr : 0x%lx\n", HeadNode->Next);
	addHeadMeterData(&HeadNode, 5);
	//printf("Next Addr : 0x%lx\n", HeadNode->Next);
	addHeadMeterData(&HeadNode, 6);
	printList(HeadNode);
	findRemoveData(&HeadNode, 4);
	printf("test find remove 4\n");
	printList(HeadNode);

	allClearMeterData(HeadNode);
	printf("clear node result\n");
	printList(HeadNode);
	#endif
	return 0;
}
