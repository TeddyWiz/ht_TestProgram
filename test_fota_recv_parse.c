#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(int argc, char *argv[])
{
	char temp_msg[]="\r\n1,183.98.244.122,36931,2,4335,0\r\n";
	char *temp_p= NULL;
	int socket = 0;
	printf("original:%s\n", temp_msg);
	temp_p = strstr(temp_msg, ",");
	printf("1 : pars %s\n", temp_p);
	socket = atoi(temp_msg);
	printf("socket : %d\r\n", socket);
	temp_p = strtok(temp_msg, ",");

	while(temp_p != NULL)
	{
		printf("parse: %s\n", temp_p);
		temp_p = strtok(NULL, ",");
	}

	return 0;
}
