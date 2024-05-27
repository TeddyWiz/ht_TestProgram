#include <stdio.h>
#include <string.h>

#define uchar 	unsigned char
typedef struct {
	unsigned char protocol;
	unsigned char len;
	unsigned char mtype;
} NbiotMsgHeader_t;

typedef struct {
	unsigned char imei[8];
	unsigned char imsi[8];
} MobileId_t;

typedef struct {
	uchar rssi;
	uchar ber;
	uchar cid[2];
	uchar rsrp[2];
	uchar rsrq[2];
	uchar snr[2];
} NbiotRadioQuality_t;


void make_id(MobileId_t *p)
{
	memcpy(p->imei, "866416049616841", 8);
	memcpy(p->imsi, "450061235190642", 8);
}
void make_radio(NbiotRadioQuality_t *p2)
{
	p2->rssi = 0x01;
	p2->ber = 0x02;
	p2->cid[0] = 0x03;
	p2->cid[1] = 0x04;
	p2->rsrp[0] = 0x05;
	p2->rsrp[1] = 0x06;
	p2->rsrq[0] = 0x07;
	p2->rsrq[1] = 0x08;
	p2->snr[0] = 0x09;
	p2->snr[1] = 0x0a;
}
unsigned int MODEM_sendPeriodicData(unsigned char *buf)
{
	unsigned int len = 0;
	unsigned char *pbuf = buf;
	NbiotMsgHeader_t *p = (NbiotMsgHeader_t *)buf;
	pbuf = buf + sizeof(NbiotMsgHeader_t);
	
	make_id((MobileId_t *)pbuf);
	pbuf += sizeof(MobileId_t);
	make_radio((NbiotRadioQuality_t *)pbuf);
	pbuf += sizeof(NbiotRadioQuality_t);
	p->protocol = 0xA4;
	p->mtype = 0x70;
	p->len = pbuf - buf;
	len = sizeof(NbiotMsgHeader_t) + sizeof(MobileId_t) + sizeof(NbiotRadioQuality_t);
	
	#if 0
	MobileId_t *p1 = (MobileId_t *)pbuf;
	pbuf += sizeof(MobileId_t);
	NbiotRadioQuality_t *p2 = (NbiotRadioQuality_t *)pbuf;
	
	p->protocol = 0xA4;
	p->mtype = 0x70;
	memcpy(p1->imei, "866416049616841", 8);
	memcpy(p1->imsi, "450061235190642", 8);
	p2->rssi = 0x01;
	p2->ber = 0x02;
	p2->cid[0] = 0x03;
	p2->cid[1] = 0x04;
	p2->rsrp[0] = 0x05;
	p2->rsrp[1] = 0x06;
	p2->rsrq[0] = 0x07;
	p2->rsrq[1] = 0x08;
	p2->snr[0] = 0x09;
	p2->snr[1] = 0x0a;
	len = sizeof(NbiotMsgHeader_t) + sizeof(MobileId_t) + sizeof(NbiotRadioQuality_t);
	#endif
	return len;
}

int main(int argc, char *argv[])
{
	unsigned char temp_buf[200]={0,};
	unsigned int len = 0;
	int i = 0;
	printf("test make message\r\n");
	len = MODEM_sendPeriodicData(temp_buf);
	printf("msg lenth = %d\r\n", len);
	for(i=0; i<len; i++)
	{
		if(((i+1)%16)==0)
			printf("\r\n");
		
		printf(" %02x",temp_buf[i]);
	}
	return 0;
}
