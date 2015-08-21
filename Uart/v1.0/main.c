#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include<pthread.h>
#include<wait.h>
#include<limits.h>
#include<linux/types.h>
#include<sys/time.h>
#include<signal.h>

#include"Uart.h"
#include"debug.h"
#define BUFSIZE	256

typedef enum{
	FALSE = 0,
	TRUE = 1
}boolean;
boolean wait_flag;
int fd;


void signal_handler_IO (int status);

int Init_Signal_Handler(int fd,struct sigaction *saio);


int main(void)
{
	int ret;
	struct sigaction saio;
	
	if((fd = HW_Uart_Open("/dev/ttyUSB0"))<0)
	{
		perror("open_port error\n");
		return (-1);
	}
	if((ret= HW_Uart_Config(BAUDRATE_115200,DATABITS_8,PARITY_N,STOPBIT_1))<0)
	{
		perror("set_opt error!\n");
		return (-1);
	}
	Init_Signal_Handler(fd,&saio);
	
	unsigned char buff[BUFSIZE]={0};
	int reCount = 0;
	int i=0;
	unsigned char buf=0;
	//printf("buf:%x\n",buff[0]);
	while(1)
	{
		if(wait_flag==TRUE)
		{
			while((reCount = read(fd,&buf,sizeof(buf)))>0)
			{
				//printf("d%d\n",i);
				buff[i++] = buf;
				usleep(5000);
			}
			buff[i] = '\0';
			//printf("dd\n");
			for(ret=0;ret<i;ret++)
				printf("buf:%x\n",buff[ret]);
			if(i > 0)
			{
	//			printf("Read[%d]:%s\n",reCount,buf);
				for(ret=0;ret<i;ret++)
				{

					write(fd,&buff[ret],1);
					tcflush(fd,TCOFLUSH);
					usleep(5000);
				}
				i=0;
			}
			wait_flag = FALSE;
			bzero(&buff,sizeof(buf));
		}
		
		sleep(2);
	}
	return 0;
}


int Init_Signal_Handler(int fd,struct sigaction *saio)
{
	/* install the signal handle before making the device asynchronous*/
	saio->sa_handler = signal_handler_IO;
	sigemptyset(&saio->sa_mask);
	//saio.sa_mask = 0; 必须用sigemptyset函数初始话act结构的sa_mask成员 
	saio->sa_flags = 0;
	saio->sa_restorer = NULL;
	sigaction(SIGIO,saio,NULL);
	/* allow the process to recevie SIGIO*/
	fcntl(fd,F_SETOWN,getpid());
	/* Make the file descriptor asynchronous*/
	fcntl(fd,F_SETFL,FASYNC);
	return 0;
}

/******************************************
 信号处理函数，设备wait_flag=FASLE
 ******************************************************/
void signal_handler_IO(int status)
{
 // printf("received SIGIO signale.\n");
  wait_flag = TRUE; 
}
