#include <stdio.h>
#define uchar unsigned char


typedef struct {
	uchar imei[8];
	uchar imsi[8];
} MobileId_t;


typedef struct {
	uchar year;
	uchar mon;
	uchar day;
	uchar hour;
	uchar min;
	uchar sec;
} NbiotTimeStamp_t;

typedef struct {
	uchar protocol; // 1 -  1
	uchar len; // 1 -  2
	uchar mtype; // 1 -  3
	MobileId_t id; // 16 - 19
	NbiotTimeStamp_t currTime; // 6  - 25
	uchar resetCommand; // 1  - 26
	uchar mi; // 1  - 27
	uchar ri; // 1  - 28
	uchar checksum; // 1  - 29
} NbiotAck_t;


static uchar cal_checksum(uchar *p, int len)
{
	uchar checksum = 0;

	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}

	return checksum;
}

int main() {
    int cnt=0;
    char tempMSG[100] ={0xA1, 0x1A, 0x50, 0x86, 0x47, 0x00, 0x04, 0x86, 0x06, 0x81, 0x8F, 0x45, 0x00, 0x61, 0x22, 0x60, 0x99, 0x93, 0x4F, 0x25, 0x06, 0x17, 0x12, 0x18, 0x12, 0x00, 0x00, 0x00, 0xDE};
    NbiotAck_t *pAck = (NbiotAck_t *)tempMSG;
    
    uchar rChecksum = *(tempMSG + pAck->len + 2);
    uchar cChecksum = cal_checksum(&pAck->mtype, pAck->len);
    printf("checksum count : %d\n", pAck->len + 2);
    printf("DL : checksum error (read : %x, calc : %x) \n", rChecksum,
				  cChecksum);
    return 0;
}