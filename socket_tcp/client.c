/************************************************************************************************************* 
 * �ļ���: 	  		client.c 
 * ����:      		����TCP����socket����˳���
 * ����:      		oxp_edward@163.com 
 * ����ʱ��:    	2014��12��31��9:45 
 * ����޸�ʱ��:	2012��3��12�� 
 * ��ϸ:      		���ݵ�socket����TCP���ӹ��̣������ͻ��˳���
					������������������Ӻ������������
*************************************************************************************************************/  
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include "socket.h"

#define SIZE	1024
int main(int argc,char**argv)
{
	int rtn;
	int socketFd;
	unsigned char buf[SIZE];
	unsigned char flag = 1;
	if(3 > argc)
	{
		Log("Input error\n");
		Log("Usage: %s <IP addr><port>\n",argv[0]);
		Log("Eample: %s 192.168.0.119 1234\n",argv[0]);
		return -1;
	}
	//1����ʼ��Socket
	socketFd  = SocketClientInit(argv[1],argv[2]);
	if(0 > socketFd)
	{
		Log("Error on SocketClientInit function\n ");
		return -1;
	}
	//2�����������������ݣ������շ���������
	while(flag > 0)
	{
		bzero(buf,sizeof(buf));
		printf("Input: ");
		fgets(buf,SIZE,stdin);
		rtn = strncmp(buf,"exit",4);
		if(0 == rtn)
		{
			flag = 0;
		}
		socketSend(socketFd,buf,sizeof(buf));
		bzero(buf,sizeof(buf));
		SocketRecv(socketFd,buf,sizeof(buf));
		Log("Recv: %s",buf);
	}
	//3���رյ�����
	socketClientClose(socketFd);
	return 0;
}