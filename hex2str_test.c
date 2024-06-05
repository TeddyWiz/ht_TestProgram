// Online C compiler to run C program online
#include <stdio.h>
char hex2char(unsigned char hexdata)
{
    unsigned char ret = 0;
    if((hexdata >=0)&&(hexdata<10))
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
	if((temp_data=hex2char((hexdata>>4)&0x0f))>0)
	{
	    *hexstr++ = temp_data;
	    if((temp_data=hex2char(hexdata&0x0f))>0)
    	{
    	    *hexstr++ = temp_data;
    	    return 1;
    	}
    	else
    	    return 0;
	}
	else
	    return 0;
}
int main() {
    // Write C code here
    unsigned char tempindata[4]={0x12, 0x34, 0x56, 0x78};
    char ret_str[10] = {0,}, temp_ret = 0;
    int i = 0, lens = 0;
    printf("Try programiz.pro\r\n");
    for (i= 0; i<4; i++)
    {
        if(hex2str(ret_str+(2*i), tempindata[i])==1)
        {
            printf("%s\r\n", ret_str);
            lens+=2;
        }
        else
        {
            printf("conv fail\r\n");
            break;
        }
    }
    printf("result : %s\r\n", ret_str);
    return 0;
}