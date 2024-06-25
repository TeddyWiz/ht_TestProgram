#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#define uint8 unsigned char
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

	char tesp_buff1[]="\r\n1,183.98.244.122,36931,300,3430332C4231303035433030383130303030354342303133314335433043393330323234423031333232354330433433423031333632354431433433423031333043384331433433313030313130303133413134343031383141343235433031343031384232343038303541354330313846303235303332394630323543333232353234383930323632333238463032374133324439304631463234434530393046313934453130304534453043344630463139344631303043383930443446304437454230313342303838433830434337304430433039374634433546303630303139354634463530333241393030303430303044303934463133413930303034303031383833303737334631323330383933454632333741433233414430303835413430313838323441,510\r\n\r\nOK\r\n";
	char tesp_buff2[]="\r\n1,183.98.244.122,36931,300,354330314230313332303543333731363130303132413134373834433039343331343343304234424345304446453042424530303031303041443030303130304644344546464646314638334641323330343343414430303031303046443443464646463538303331393533333939324539333731384233463632333742344337463443434530423545304543423046354230463342463030463030304244453346463030463030334635303033303033463930313230303043323037453443334542303830303030373234374134433345463037463030344134413436313830413541304544413046354533423930464630464342323332383136313030314346304343433044434430464144303030353030314534463031303031463446303330303830303032363544,210\r\n\r\nOK\r\n";
	char tesp_buff3[]="\r\n1,183.98.244.122,36931,210,324131343046393330323230304539333137323430393433303834334342303830463138344235423030313834424439454230444341303830463138344135413030313834414439454130434541344230303030313935333038363330383946454532423032323030393945454232423238313631303031423234303830354135433031334334303030314333443430303036303345343030303134304634334230313332363544423031333343314342303133303038453043343331303031423234303038413532303031303031333435,0\r\n\r\nOK\r\n";
	char *p=NULL, *p1;
	int payloadLen;
	int i, offset =0, j;
	char FotaTempBuf[12];
	char rxData[517]={0,};
	char temp_rxData[300];
	char *temp_buf[3],type;
	temp_buf[0] = tesp_buff1;
	temp_buf[1] = tesp_buff2;
	temp_buf[2] = tesp_buff3;
	int dataLen = 0, r_Len=0;
	char ACKADDR[12];
	//FOTA2 테스튼 Port : 36941

	for(j=0;j<3;j++)
	{
		p = strstr(temp_buf[j], "36931");
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
			dataLen = (int)(p1-p)/2;
			for (i = 0; i < dataLen; i++) {
			//for (i = 0; i < 3; i++) {
				FotaTempBuf[i] = _ascii2BCD(*(p + 0), *(p + 1));
				p += 2;
				printf(" %c",FotaTempBuf[i]);
			}
			FotaTempBuf[i+1] = 0;
			printf("len str : %s\r\n",FotaTempBuf);
			dataLen = fotaatoi(FotaTempBuf);
			printf("data len:%d\r\n", dataLen);
			
			p1 +=2;
			p = p1;
			for (i=0; i<12; i++)
			{
				ACKADDR[i]=(char)(*(p+4+i)&0xFF);
			}
			ACKADDR[i]=0;
			printf("ADDR: %s\r\n",ACKADDR);
			payloadLen = payloadLen - 4;
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
		
		//rxData[i+offset]=0;
		//printf("rxcv[%d]:%s\r\n\r\n",i, rxData);
		offset = offset+(payloadLen/2);
		printf("offset=%d\r\n",offset);
		printf("HEX : ");
		for(i=0; i<offset; i++)
		{
			printf("%02X",(rxData[i])&0x000000ff);
		}
		printf("\r\n");
		if(j==2)
		{
			unsigned char checksum =
					_calChecksum((unsigned char *)rxData, offset - 1);
			printf("checksum=%02X\n", (checksum&0x000000ff));
			printf("ms checksum=%02x\n",(rxData[offset-1]&0x000000ff));
		}
	}







	#if 0
	printf("===================================\r\n");
	printf("numtest:%d\n", numtest);
	numlen = int2str(numtest, numstr);
	printf("str:%s, numlen: %d\n", numstr, (int)numlen);
	printf("===================================\r\n");
	printf("msg:%s\n", temp_msg2);
	conv_str[0]= _ascii2BCD(temp_msg2[0],temp_msg2[1]);
	conv_str[1]= _ascii2BCD(temp_msg2[2],temp_msg2[3]);
	printf("1 HEX=%02X%02X\n", conv_str[0],conv_str[1]);
	printf("1 HEXstr=%s\n", conv_str);
	conv_hex = _ascii2BCD(conv_str[0], conv_str[1]);
	printf("2 HEX=%02X\n", conv_hex);

	printf("===================================\r\n");
	printf("original:%s\n", temp_msg);
	#define recv_msg "+NSONMI:"
	printf("+NSONMI: len:%d\r\n", (int)strlen(recv_msg));
	temp_p=strstr(temp_msg1,recv_msg);

	temp_p += strlen(recv_msg);
	temp_p++;
	printf("===================================\r\n");
	printf("1: %c : %s\r\n", *temp_p, temp_p);
	temp_p=strstr(temp_p, ",");
	printf("2: %c : %s\r\n", *temp_p, temp_p);
	temp_p++;
	printf("3: %c : %s\r\n", *temp_p, temp_p);
	temp_p1=strstr(temp_p, "\r\n");
	printf("4: %c : %s\r\n", *temp_p1, temp_p1);
	printf("len : %d\n", (int)(temp_p1-temp_p));
	memcpy(temp_len, temp_p, temp_p1-temp_p);

	printf("Len str : [%s]\n", temp_len);
	printf("===================================\r\n");

	temp_p += strlen(recv_msg);
	printf("pars soc : %c \r\n", *temp_p);

	temp_p = strstr(temp_msg, "\r\n");

	tempChar = *(temp_p+2);
	tempChar1 = *(temp_msg+2);
	printf("data : %c:%c\r\n", tempChar, tempChar1);
	temp_p = strstr(temp_msg, ",");
	temp_p++;
	printf("1 : pars %s\n", temp_p);
	socket = atoi(temp_msg);
	printf("socket : %d\r\n", socket);
	//temp_p++;
	temp_p = strstr(temp_p, ",");
	temp_p++;
	printf("ip:%s\n", temp_p);
	temp_p = strstr(temp_p, ",");
	temp_p++;
	printf("port:%s\n", temp_p);


	temp_p = strtok(temp_msg, ",");
	while(temp_p != NULL)
	{
		printf("parse: %s\n", temp_p);
		switch(cnt)
		{
			case 0:	//socket
			socket = atoi(temp_p);
			printf("socket : %d\r\n", socket);
			break;
			case 1: //IP
			printf("IP:%s\n", temp_p);
			break;
			case 2: //port
			port = atoi(temp_p);
			printf("port: %d, %s\n", port, temp_p);
			break;
			case 3: //lengh
			len = atoi(temp_p);
			printf("len: %d, %s\n", len, temp_p);
			break;
			case 4: //data
			printf("data: %s\n", temp_p);
			break;
			case 5: //
			break;

		}
		temp_p = strtok(NULL, ",");
		cnt++;
	}
#endif
	return 0;
}
