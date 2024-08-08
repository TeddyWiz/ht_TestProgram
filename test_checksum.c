#include<stdio.h>
#include<string.h>

#define uint8   unsigned char

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

uint8 cal_checksum(uint8 *p, int len)
{
	uint8 checksum = 0;

	for (int i = 0; i < len; i++) {
		checksum += *p++;
	}

	return checksum;
}

int main(int argc, char *argv[])
{
    //char test_msg[]="a36970869692060814435f450061243599720f3400dd003c000b000600231460000105312312121212011300010618061b0d0000061800921700000000de00dd00dd00dd00dd00de00dd00dd00dd00dd00de00da00e000dd00dd00de00dd00dd00dd00dd00de00dd00dd00ca";
    //char test_msg[]="A33B70866416049616841F450061235190642F4300BF004A000A0013002323100001053024000000000100000101FFFFFF0000000101FFFFFFFFFFFFFF14";
    //char test_msg[]="A44770866416049618615F450061226099934F4500BF0000004B000A001900232310000205312424600000011300010118071D0E260001040044FC2E0020FC2E00FBFB2E00D7FB2E0094";
    char test_msg[2048];
    int i=0;
    int datalen =0;
    char conv_hex[2048];
    char result=0;
    int msg_len = 0;
    if(argc!=2)
    {
        printf("not match parameter %d\n", argc);
        printf("%d, %s\n", 1, argv[1]);
        printf("%d, %s\n", 2, argv[2]);
        return -1;
    }
    memset(test_msg, 0, sizeof(test_msg));
    strcpy(test_msg, argv[1]);
    msg_len = (int)strlen(test_msg);
    printf("ori[%d]:%s\r\n",(int)strlen(test_msg), test_msg);
    printf("ori[%d]:%s\r\n",(int)strlen(argv[1]), argv[1]);
    for(i=0; i<(msg_len/2); i++)
    {
        conv_hex[i]=_ascii2BCD(test_msg[i*2], test_msg[i*2+1]);
    }
    printf("len=%d\nHex:\r\n",i);
    for(i=0; i<(strlen(test_msg)/2); i++)
    {
        printf("%02X",(conv_hex[i]&0x000000FF));
    }
    printf("\n");
    datalen = (int)(conv_hex[1]&0x000000ff);
    printf("data len=%d\r\n", datalen);
    result = cal_checksum(conv_hex+2, datalen);
    //result = cal_checksum(conv_hex+2, 108-5);
    printf("result=%02X , %c%c\r\n", (result&0x000000ff), test_msg[msg_len -2], test_msg[msg_len -1]);

    return 0;
}