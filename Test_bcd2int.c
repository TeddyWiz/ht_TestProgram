#include <stdio.h>
#include <string.h>

#define uint8	unsigned char
#define uint16	unsigned short
#define uint32	unsigned long

typedef union{
	uint32 data32;
	uint8 data8[4];
}data8_32_t;

void bcd2int(uint8 *pBCD, uint32 *pInt, uint8 nDigit)
{
	#if 0
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
	*pInt = 0;
	//uint8 temp = 0;
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
	uint8 tempData[4] = {0x12, 0x34, 0x56, 0x78};
	uint8 tempData2[4] = {0,};
	uint32 tempData1 = 0;
	data8_32_t tempDataT;
	
	memcpy(tempDataT.data8, tempData, 4);
	printf("test \r\n");
	printf("Indata = 0x%02X %02X %02X %02X\r\n", tempData[0], tempData[1], tempData[2], tempData[3]);
	printf("tempDataT = 0x%02X %02X %02X %02X, %ld\r\n", tempDataT.data8[0], tempDataT.data8[1], tempDataT.data8[2], tempDataT.data8[3], tempDataT.data32);
	tempDataT.data8[3] = tempData[0];
	tempDataT.data8[2] = tempData[1];
	tempDataT.data8[1] = tempData[2];
	tempDataT.data8[0] = tempData[3];
	printf("tempDataT = 0x%02X %02X %02X %02X, %ld\r\n", tempDataT.data8[0], tempDataT.data8[1], tempDataT.data8[2], tempDataT.data8[3], tempDataT.data32);
	bcd2int(tempData, &tempData1, 4);
	printf("data trans 32 = %ld\r\n", tempData1);
	memcpy(tempData2, &tempData1, 4);
	printf("tempData2 = 0x%02X %02X %02X %02X\r\n", tempData2[0], tempData2[1], tempData2[2], tempData2[3]);
	
	return 0;
}
