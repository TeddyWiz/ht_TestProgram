#include <stdio.h>
#include <string.h>

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

int main() {
    char buf[100]="c1020801780f00000000001c1301000000";
    char hexbuf[50]={0,};
    char checksum = 0;
    int i = 0;
    int len = strlen(buf);
    printf("in[%d]=%s\n", len, buf);
    for(i=0;i<(len/2);i++)
    {
        hexbuf[i] = str2hex(buf+(2*i));
        printf("%d : %02x\n", i,  (hexbuf[i]&0x000000FF));
    }
    printf("check sum start\n");
    for(i=2;i<len;i++)
    {
        checksum += (hexbuf[i]&0x000000FF);
        printf("%d : %02x\n",i, (checksum&0x000000FF));
    }
    printf("checksum : %02x\n", checksum&0x000000FF);
    // Write C code here
    printf("end program \n");

    return 0;
}
