#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define uint16 unsigned short
#define uint8	unsigned char

typedef struct MeterData{
	uint16 num;
	uint8 cal;
	uint8 status;
	uint8 meterData[4];
	uint8 pressure[2];
	uint8 temperature[2];
	struct MeterData *Next;
}MeterData;

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
void allClearMeterData(MeterData** head)
{
	MeterData *HeadNode = *head;
	MeterData *NextNode = HeadNode->Next;
	while(HeadNode != NULL)
	{
		free(HeadNode);
		HeadNode = NextNode;
		NextNode = HeadNode->Next;
	}
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

int main(int argc, char *argv[])
{
	MeterData *HeadNode = createMeterData(1);
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
	allClearMeterData(&HeadNode);
	printf("clear node result\n");
	printList(HeadNode);
	return 0;
}
