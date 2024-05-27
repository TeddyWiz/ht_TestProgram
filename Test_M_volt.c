#include <stdio.h>
#include <string.h>

char voltTrans(char indata)
{
	    char i = 0;
	        for(i=0 ; i<31; i++)
			    {
				            if(indata > (36-i))
						            {
								                return i;
										        }
					        }
}
char voltTrans1(char indata)
{
	    char temp = 0;
	        if(indata > 36) {
			        return 0x00;
				    }
		    else if(indata < 7){
			            return 0x1f;
				        }
		        temp = 37 - indata;
			    return temp;
}
int main() {
	    // Write C code here
	    char vol = 37;
	    char temp1 = 0, temp2=0;
	    int i =0;
	    for(i=0; i<40; i++)
	    {
	    	temp1 = voltTrans(i);
	    	temp2 = voltTrans1(i);
	    	printf("[%02d] 1 : %2d, 2 : %2d\n", i, temp1, temp2);
	    }
	printf("Try programiz.pro");
	  
	 return 0;
}
