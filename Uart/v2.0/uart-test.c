#include <stdio.h>

#include <string.h>
#include "uart.h"
int main(int argc,char**argv)
{
	int rtn;
	int Count = 0;
	unsigned char *send = "Hello Uart";
	unsigned char buf[1024] ={0};
	if(2 > argc)
	{
		printf("Input error\n");
		printf("Usage: %s <Device>\n",argv[0]);
		printf("Eample: %s /dev/ttyS0\n",argv[0]);
		return -1;
	}
	
	rtn = SerialOpen(argv[1]);
	if(0 > rtn)
		return -1;
	
	rtn = SerialConfig(115200,8,'n',1);
	if(0 > rtn)
		return -1;
	
	while(1)
	{
		rtn = SerialSend(send,strlen(send));
		if(0 < rtn)
		{
			printf("[Send]: %s\n",send);
		}
		sleep(1);
		rtn = SerialReceive(buf,sizeof(buf));
		if(0 < rtn)
		{
			printf("[Recv]: %s\n",buf);
		}
	}
	SerialClose();
}
