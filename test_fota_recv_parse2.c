#include <stdio.h>

#include<string.h>
#include<stdlib.h>

#define uint8 unsigned char
char hex2char(unsigned char hexdata)
{
    unsigned char ret = 0;
    if((hexdata >= 0)&&(hexdata < 10))
    {
        ret = hexdata + '0';
    }
    else if((hexdata >9) & (hexdata <= 0x0f))
    {
        ret = (hexdata - 0x0a) + 'a';
    }
    return ret;
}

char hex2str(char *hexstr, unsigned char hexdata)
{
	char temp_data = 0;
	temp_data=hex2char((hexdata>>4)&0x0f);
	printf("%c(%02X) ",temp_data, temp_data);
	*hexstr++ = temp_data;
	temp_data=hex2char(hexdata&0x0f);
	printf("%c(%02X) ",temp_data, temp_data);
    *hexstr++ = temp_data;
	return 2;
}

int str2hexstr(char *hexstr, char *strdata, int len)
{
	int i,ret = 0;
	for(i=0; i<len; i++)
	{
		ret += hex2str(hexstr, *strdata++);
		hexstr += 2;
	}
	return ret;
}

unsigned char _calChecksum(unsigned char *p, int len)
{
	uint8 checksum = 0;
	for (int i = 0; i < len; i++) {
		checksum += *(p + i);
	}
	return checksum;
}
static unsigned int fotaatoi(char *cdata)
{
	int data = 0;

	while (*cdata) {
		if (*cdata >= '0' && *cdata <= '9') {
			if (data != 0) {
				int mulData = data;
				for (int i = 0; i < 9; i++) {
					data += mulData;
				}
			}
			data = data + *cdata;
			data = data - '0';
		} else {
			return data;
		}

		cdata++;
	}

	return data;
}

char int2str(unsigned int num, char *strData)
{
	unsigned int dev = 10000;
	int i =0;
	unsigned char temp = 0, len = 0, start=0;
	for(i=0; i<5; i++)
	{
		temp = num/dev;
		printf("%d: temp:%d\n",i, temp);
		if(temp>10)
			temp = 0;
		if((temp!=0)|(start==1))
		{
			start=1;
			*strData++ = '0'+temp;
			len++;
		}else if((i==4)&&(start==0))
		{
			*strData++ = '0'+temp;
			len++;
		}
		num = num%dev;
		dev = dev/10;
		printf("%d) num:%d dev:%d\n", i, num, dev);
	}
	*strData = 0;
	return len;
}

static uint8 _ascii2Hex(uint8 ch)
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

static uint8 _ascii2BCD(char a, char b)
{
	return (_ascii2Hex(a) * 0x10 + _ascii2Hex(b));
}


char IP_Str_Parse(char *ipStr, char *ipData)
{

}
int main(int argc, char *argv[])
{
	char temp_msg[]="\r\n1,183.98.244.122,36931,2,4335,300\r\n";
	char temp_msg1[]="\r\n1,48\r\n\r\nOK\r\n\r\n+NSOSTR:1,101,1\r\n\r\n+NSONMI:1,1024\r\n";
	char temp_msg2[]="4331";
	char *temp_p= NULL;
	char *temp_p1= NULL;
	char temp_len[5]={0,};
	int socket = 0, port = 0, len=0;
	char cnt = 0;
	char tempChar = 0, tempChar1 = 0;

	char conv_str[10]={0};
	char conv_hex=0;

	char numstr[10]={0}, numlen=0;
	unsigned int numtest=0;

	char tesp_buff1[]="\r\n1,183.98.244.122,36941,265,3236312CB1005C008100005CB0131C5C0C930224B013225C0C43B013625D1C43B0130C8C1C43100110013A1440181A425C014018B240805A5C018F0250329F025C322524890262328F027A32D90F1F24CE090F194E100E4E0C4F0F194F100C890D4F0D7EB013B088C80CC70D0C097F4C5F0600195F4F5032A90004000D094F13A900040018830773F1230893EF237AC23AD0085A4018824A5C01B013205C371610012A14784C0943143C0B4BCE0DFE0BBE000100AD000100FD4EFFFF1F83FA23043CAD000100FD4CFFFF580319533992E93718B3F6237B4C7F4CCE0B5E0ECB0F5B0F3BF00F000BDE3FF00F003F5003003F9012000C207E4C3EB0800007247A4C3EF07F004A4A4618DA,0\r\n\r\nOK\r\n";
	char tesp_buff2[]="\r\n1,183.98.244.122,36941,158,800007247A4C3EF07F004A4A46180A5A0EDA0F5E3B90FF0FCB2328161001CF0CCC0DCD0FAD0005001E4F01001F4F03008000265D2A140F9302200E93172409430843CB080F184B5B00184BD9EB0DCA080F184A5A00184AD9EA0CEA4B000019530863089FEE2B0220099EEB2B28161001B240805A5C013C40001C3D4000603E4000140F43B013265DB0133C1CB013008E0C431001B24008A5200100133435,0\r\n\r\nOK\r\n";
	char *p=NULL, *p1;
	int payloadLen;
	int i, offset =0, j;
	char FotaTempBuf[12];
	char rxData[517]={0,};
	char temp_rxData[300];
	char *temp_buf[3],type;
	temp_buf[0] = tesp_buff1;
	temp_buf[1] = tesp_buff2;
	int dataLen = 0, dataLen1 = 0,r_Len=0;
	char ACKADDR[12], ACKADDRCon[12];
	//FOTA2 테스튼 Port : 36941
    printf("start program\n");
	printf("ori1: %s\r\n", tesp_buff1);
    p = strstr(tesp_buff1, "36941");
    // data len
    //p = strstr(p, ",");
    p = strstr(p, ",") + 1; 
    payloadLen = fotaatoi(p);
    printf("len %d\r\n", payloadLen);
    p = strstr(p, ",") + 1;
    p1 = strstr(p,"2C");
    printf("count=%d %d\r\n",(int)(p1-p), (int)(p1-p)/2);
    dataLen1 = (int)(p1-p)/2;
    for (i = 0; i < dataLen1; i++) {
    //for (i = 0; i < 3; i++) {
        FotaTempBuf[i] = _ascii2BCD(*(p + 0), *(p + 1));
        p += 2;
        printf(" %c",FotaTempBuf[i]);
    }
    FotaTempBuf[i+1] = 0;
    printf("len str : %s\r\n",FotaTempBuf);
    dataLen = fotaatoi(FotaTempBuf);
    printf("data len:%d\r\n", dataLen);
    
    p = p1;
    for (i=0; i<6; i++)
    {
        ACKADDR[i]=(char)(*(p+4+i)&0xFF);
    }
    ACKADDR[i]=0;
    printf("ADDR: %s\r\n",ACKADDR);
	len = str2hexstr( ACKADDRCon, ACKADDR, 6);
	ACKADDRCon[len] =0;
	printf("con ADDR[%d]: %s\n", len, ACKADDRCon);
    payloadLen = payloadLen - (dataLen1 +1);
    printf("off len = %d (%d)\n", payloadLen, (dataLen1 +1));
    p+=2;  //"2C" 다음 값
    printf("original:%s\n",p);
    for (i = 0; i < payloadLen; i++) {
        rxData[i] = _ascii2BCD(*(p + 0), *(p + 1));
        p += 2;
    }
    printf("HEX : ");
    for(i=0; i<payloadLen; i++)
    {
        printf("%02X",(rxData[i])&0x000000ff);
    }
	printf("\n\n i:%d\n", i);
    printf("\r\n");
    unsigned char checksum =
            _calChecksum((unsigned char *)rxData, payloadLen - 1);
    printf("checksum=%02X\n", (checksum&0x000000ff));
    printf("ms checksum=%02x\n",(rxData[payloadLen-1]&0x000000ff));
	printf("payloadlen:%d\n", payloadLen);
	unsigned long address = 0;
	for (i = 1; i < 4; i++) {
		if (address) {
			unsigned long mulData = address;
			for (int j = 0; j < 255; j++) {
				address += mulData;
			}
		}
		address |= rxData[i];
	}
	printf("address:%06lX\n",address);
	//printf("")
    #if 0
	for(j=0;j<2;j++)
	{
		p = strstr(temp_buf[j], "36941");
		// data len
		p = strstr(p, ",");
		p = strstr(p, ",") + 1; 
		payloadLen = fotaatoi(p);
		printf("len %d:%d\r\n",j, payloadLen);
		p = strstr(p, ",") + 1;
		if(offset==0)
		{
			p1 = strstr(p,"2C");
			printf("count=%d %d\r\n",(int)(p1-p), (int)(p1-p)/2);
			dataLen1 = (int)(p1-p)/2;
			for (i = 0; i < dataLen1; i++) {
			//for (i = 0; i < 3; i++) {
				FotaTempBuf[i] = _ascii2BCD(*(p + 0), *(p + 1));
				p += 2;
				printf(" %c",FotaTempBuf[i]);
			}
			FotaTempBuf[i+1] = 0;
			printf("len str : %s\r\n",FotaTempBuf);
			dataLen = fotaatoi(FotaTempBuf);
			printf("data len:%d\r\n", dataLen);
			
			p = p1;
			for (i=0; i<6; i++)
			{
				ACKADDR[i]=(char)(*(p+4+i)&0xFF);
			}
			ACKADDR[i]=0;
			printf("ADDR: %s\r\n",ACKADDR);
			payloadLen = payloadLen - (dataLen1 +1);
            printf("off len = %d (%d)\n", payloadLen, (dataLen1 +1));
            p+=2;
			#if 0
			for (i = 0; i < 2; i++) {
				FotaTempBuf[i] = _ascii2BCD(*(p1 + 0), *(p1 + 1));
				p1 += 2;
			}
			
			type = _ascii2BCD(*(FotaTempBuf + 0), *(FotaTempBuf + 1));
			printf("type=%02X\r\n", (type&0x000000ff));
			p= p1;
			for (i=0; i<12; i++)
			{
				ACKADDR[i]=(char)(*(p+i)&0xFF);
			}
			ACKADDR[i]=0;
			printf("ADDR: %s\r\n",ACKADDR);
			p1 = ACKADDR;
			for (i = 0; i < 6; i++) {
				FotaTempBuf[i] = _ascii2BCD(*(p1 + 0), *(p1 + 1));
				p1 += 2;
			}
			FotaTempBuf[i]=0;
			printf("addr=%s\r\n",FotaTempBuf);
			unsigned long mulData=0, address=0;
			for (i = 0; i < 3; i++) {
				if (address) {
					unsigned long mulData = address;
					for (int k = 0; k < 255; k++) {
						address += mulData;
					}
				}
				address |= _ascii2BCD(FotaTempBuf[i*2], FotaTempBuf[i*2+1]);;
			}
			printf("addr=0x%06lX\r\n",address);

			p+=12;
			payloadLen = payloadLen-12;
			printf("offset len= %d\r\n",payloadLen);
			#endif
		}
		printf("ori:%s\r\n", p);
        #if 0
		for (i = 0; i < payloadLen; i++) {
			temp_rxData[i] = _ascii2BCD(*(p + 0), *(p + 1));
			p += 2;
		}
		//p1=temp_rxData;
		for (i = 0; i < (payloadLen/2); i++) {
			//rxData[i+offset] = _ascii2BCD(*(p1 + 0), *(p1 + 1));
			rxData[i+offset] = _ascii2BCD(temp_rxData[i*2], temp_rxData[i*2+1]);
			//p1 += 2;
		}
        #endif
        
        for (i = 0; i < payloadLen; i++) {
			rxData[i+offset] = _ascii2BCD(*(p + 0), *(p + 1));
			p += 2;
		}
		offset = offset+payloadLen;
		//rxData[i+offset]=0;
		//printf("rxcv[%d]:%s\r\n\r\n",i, rxData);
		//offset = offset+(payloadLen/2);
		printf("offset=%d\r\n",offset);
		printf("HEX : ");
		for(i=0; i<offset; i++)
		{
			printf("%02X",(rxData[i])&0x000000ff);
		}
		printf("\r\n");
		if(j==1)
		{
			unsigned char checksum =
					_calChecksum((unsigned char *)rxData, offset - 1);
			printf("checksum=%02X\n", (checksum&0x000000ff));
			printf("ms checksum=%02x\n",(rxData[offset-1]&0x000000ff));
		}
	}
    #endif
    return 0;
}