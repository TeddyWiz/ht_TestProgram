#include <stdio.h>
#include <string.h>

//a33b70867787051655631f450040120087553f480064004e000a000f002123300005030224000000000100000106ffffff0000000101ffffffffffffff4d
int data[]={-93,59,112,-122,119,-121,5,22,85,99,31,69,0,64,18,0,-121,85,63,67,0,100,0,73,0,10,0,26,0,33,35,48,0,5,3,2,36,0,0,0,0,1,0,0,1,6,-1,-1,-1,0,0,0,1,1,-1,-1,-1,-1,-1,-1,-1,78};
int main() {
    // Write C code here
    printf("Try programiz.pro");
    int len;
    
    len = sizeof(data);
    printf("len= %d %d\n", len/sizeof(int), sizeof(int));
    #if 1
    for(int i=0; i<(len/4); i++ )
    {
        printf("%02x", ((char)data[i])&0x000000FF);
    }
    #endif

    return 0;
}