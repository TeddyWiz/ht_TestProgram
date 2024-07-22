#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define uint16 unsigned short
#define uint8	unsigned char
#define uint32 unsigned long

#define YEAR_MIN 2017
#define YEAR_MAX 2100

#define TRUE 1

#define CAL_COUNT_TIME_UNIT(x) 		(x<24? 1: x<48? 2: x<72? 3: x<96? 4: x<144? 6: x<288? 12: 24)

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
} MeterTimeData_t;


typedef union{
	uint32 data_b32;
	uint8 data_b8[4];
} dataTrans4_t;

typedef union{
	uint16 data_b16;
	uint8 data_b8[2];
} dataTrans2_t;

//gloval data
MeterData *HeadNode = NULL;
MeterStoredData_t StoredMeterData;
MeterTimeData_t meterPreTime;
MeterTimeData_t meterSaveTime;

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

void allClearMeterData(MeterData **head)
{
	MeterData *Temp;
	while(*head != NULL)
	{
		Temp = *head;
		*head = Temp->Next;
		free(Temp);
	}
}

void RemoveLastMeterData(MeterData **head)
{
	MeterData *Temp = *head;
	while(Temp->Next != NULL)
	{
		Temp = Temp->Next;
	}
	free(Temp);
}

MeterData *findMeterData(MeterData* head, uint16 data) {
	if(head == NULL)
		return NULL;
	
	while(head != NULL)
	{
		if(head->num == data)
		{
			return head;
		}
		head = head->Next;
	}
	return NULL;
}

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

uint8 CheckSaveMeterData(uint16 num)
{
	uint8 saveFlag = 1;
	int n = (num/12);
	uint8 p1[12]={1, 5, 7, 11, 2, 10, 3, 9, 4, 8, 6, 12};
	uint8 j;
	uint16 i;
	if(num<24)
		return 1;
	else if(num==24)
	{
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
	else if(num<48)
	{
		//p1={1,5,7,11} + 12*n pass
		for(j=0; j<4; j++)
		{
			if(num == (p1[j]+12*n))
			{
				return 0;
			}
		}
	}
	else if(num==48)
	{
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
	else if(num<72)
	{
		//p1 , p2 + 12*n pass 0-6 
		for(j=0; j<6; j++)
		{
			if(num == (p1[j]+12*n))
			{
				return 0;
			}
		}
	}
	else if(num==72)
	{
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
	else if(num<96)
	{
		//p4={4, 8}, p5=6 p6=12 + 12*n save 8-12
		saveFlag = 0;
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
	else if(num==96)
	{
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
	else if(num<144)
	{
		//p5 p6 + 12*n save 10-12
		saveFlag = 0;
		if((num == (p1[10]+12*n))||(num == 12*n)){
			return 1;
		}
	}
	else if(num==144)
	{
		//p5 + 12*n remove 10
		printf("Step 5: remove - ");
		for(i=0; i<n; i++)
		{
			printf("%d  ", p1[10]+12*i);
			findRemoveData(&HeadNode, p1[10]+12*i);
		}
		printf("\n");
	}
	else if(num<288)
	{
		//p6 + 12*n save
		saveFlag = 0;
		if(num == 12*n){
			return 1;
		}
	}
	else if(num==288)
	{
		//p6 + 12*n remove 11
		printf("Step 6: remove - ");
		for(i=0; i<n; i++)
		{
			printf("%d  ", p1[11]+24*i);
			findRemoveData(&HeadNode, p1[11]+24*i);
		}
		printf("\n");
	}
	else if(num<=552)
	{
		//nData%24 == 0 save
		saveFlag = 0;
		if((num%24)==0)
			return 1;
	}
	else if(num>=31200) //1300*24
	{
		StoredMeterData.nData = 576; //24*24
	}
	else	// over 552
	{
		//nData%24 == 0 save and remove Last data
		saveFlag = 0;
		if((num%24)==0)
		{
			//remove Last data
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

void insertDateToData(Date_t *pDate, MeterTimeData_t *pData, uint8 isIgnoreSec)
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
	insertDateToData(&now, &meterPreTime, 0);
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
		insertDateToData(&now, &meterSaveTime, 0);
		return 1;
	}
	else{
		return 0;
	}

	return 0;
}

int tempSaveNum[500];
int tempSaveindex=0;

void Unit2AddMeterData(MeterUnitData_t *pUnit)
{
	uint8 saveFlag = 1;
	Date_t now={.year=2024, .mon=7, .day=22, .hour=0, .min=0, .sec=1};
	saveFlag = CheckSaveMeterData(StoredMeterData.nData);
	if(saveMeterTime(StoredMeterData.nData, now) ==1)
	{
		printf("now Time : %2d-%2d-%2d %2d:%2d:%2d \n", now.year, now.mon, now.day,\
		now.hour, now.min, now.sec);
		printf("save Time : %2d-%2d-%2d %2d:%2d:%2d\n", meterSaveTime.year, \
		meterSaveTime.mon, meterSaveTime.day, meterSaveTime.hour, meterSaveTime.min,\
		meterSaveTime.sec );
	}
	if(saveFlag)
	{
		now.hour= (StoredMeterData.nData/60);
		now.min = (StoredMeterData.nData%60);
		if(now.hour > 24)
		{
			now.day = now.hour/24;
			now.hour = now.hour % 24;
		}

		tempSaveNum[tempSaveindex++] = StoredMeterData.nData;
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
	for(i=0; i<nDigit; i++)
	{
		*pInt *= 100;
		*pInt += ((*pBCD & 0xf0) >> 4) * 10 + (*pBCD & 0x0f);
		pBCD++;
	}
	
}


int main(int argc, char *argv[])
{
	
	dataTrans4_t meter_data;
	dataTrans2_t *Temp2Trans;
	
	MeterUnitData_t tempUnit;
	uint32 meter_add = 0, temp_trans = 0;
	int i =0, j;
	uint32 temp2byte=0;
	uint8 meteringData[4];
	uint16 temp16buff;
	
	int result[500] ={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 
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
    bcd2int(bcd, temp, 4);
    printf("1 0x%02X%02X%02X%02X-%5ld-%02X%02X%02X%02X\n", bcd[0], bcd[1], bcd[2], bcd[3], *temp, msg[0], msg[1], msg[2], msg[3] );
    
    printf("2 0x%02X%02X%02X%02X-%5ld-%02X%02X%02X%02X\n", bcd1[0], bcd1[1], bcd1[2], bcd1[3], *temp, msg[4], msg[5], msg[6], msg[7] );
	p+=4;
    temp = (uint32 *)p;
    bcd2int(bcd1, temp, 4);
    printf("3 0x%02X%02X%02X%02X-%5ld-%02X%02X%02X%02X\n", bcd1[0], bcd1[1], bcd1[2], bcd1[3], *temp, msg[4], msg[5], msg[6], msg[7]);

	printf("end\n");
	#endif
#if 0
	for(i=0; i<553; i++)
	{
		//add meterdata
		
		//addHeadMeterData(&HeadNode, i);
		meter_add += (int)(rand()%100);
		temp_trans = 0;
		int2bcd(&temp_trans, &meter_add, 8);
		meter_data.data_b32 = temp_trans;
		for(j=0;j<4;j++)
		{
			tempUnit.meterData[j] = meter_data.data_b8[3-j];
		}
		temp2byte = 20 + (int)(rand()%10);
		int2bcd(&temp_trans, &temp2byte, 4);
		tempUnit.temperature[0] = (temp_trans >> 8) & 0x000000FF;
		tempUnit.temperature[1] = (temp_trans) & 0x000000FF;
		temp2byte = 10 + (int)(rand()%10);
		memcpy(tempUnit.pressure, &temp2byte, sizeof(uint8)*2);

		Unit2AddMeterData(&tempUnit);
		memcpy(&temp16buff, HeadNode->pressure, sizeof(HeadNode->pressure));
		printf("%3d : %02X%02X%02X%02X[%02X%02X|%2d]\n", i,  
		HeadNode->meterData[0], HeadNode->meterData[1], HeadNode->meterData[2], HeadNode->meterData[3],
		HeadNode->temperature[0], HeadNode->temperature[1],temp16buff);

		
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
	}
	printMeterData(HeadNode);
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
	allClearMeterData(&HeadNode);
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
