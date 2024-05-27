#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if(argc == 0)
	{
		printf("parameter Error\r\n");
		return 0;
	}
	int lens = 0;
	lens = strlen(argv[1]);
	printf("data=[%s]\r\nLength=%d\r\n",argv[1],lens);
	return 1;
}
