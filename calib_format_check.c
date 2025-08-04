#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tcp_setver_cmd.h"


char ch2hex(char in)
{
    char result = 0xFF;
    if((in >= '0')&&(in <='9'))
    {
        result = in -'0';
    }
    else if((in >= 'a')&&(in <= 'f'))
    {
        result = in - 'a' + 0x0a;
    }
    else if((in >= 'A')&&(in <= 'F'))
    {
        result = in - 'A' + 0x0a;
    }
    else
    {
        printf("[%c] hex Error!\n", in);
    }
    return result;
}

char str2hex(char *msg)
{
    char result = 0;
    result = ch2hex(*msg);
    //printf("hex1=%02x\n", (result)&0x000000FF);
    result = result << 4;
    result = result | ch2hex(*(msg+1));
    //printf("hex2=%02x\n", (result)&0x000000FF);
    return result;
}

int hex2str(char *InMsg, int InLen, char *OutMsg)
{
    int sendLen =0;
    for(int i=0; i<InLen;i++)
    {
        sendLen += sprintf(OutMsg+sendLen, "%02X",(*(InMsg + i)&0x000000FF));
    }
    return sendLen;
}

unsigned char checkSum(char *buffer, int len)
{
    int i =0;
    unsigned char result = 0;
    for(i=0;i<len; i++)
    {
        result += *(buffer+i);
    }
    return result;
}

int main(int argc, char *argv[])
{
	char test_msg[2048];
    int msg_len = 0;
    char hexbuf[100]={0,};
    char checksum = 0;
    int i = 0;
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
    for(i=0;i<(msg_len/2);i++)
    {
        hexbuf[i] = str2hex(test_msg+(2*i));
        printf("%d : %02x\n", i,  (hexbuf[i]&0x000000FF));
    }
    printf("length : %d\n",hexbuf[1]);
    checksum = checkSum(hexbuf+2, hexbuf[1]);
	return 0;
}
