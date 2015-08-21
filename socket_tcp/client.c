/************************************************************************************************************* 
 * 文件名: 	  		client.c 
 * 功能:      		基于TCP创建socket服务端程序
 * 作者:      		oxp_edward@163.com 
 * 创建时间:    	2014年12月31日9:45 
 * 最后修改时间:	2012年3月12日 
 * 详细:      		根据的socket建立TCP连接过程，创建客户端程序，
					并在与服务器建立连接后，想服务发送数据
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
	//1、初始化Socket
	socketFd  = SocketClientInit(argv[1],argv[2]);
	if(0 > socketFd)
	{
		Log("Error on SocketClientInit function\n ");
		return -1;
	}
	//2、往服务器发送数据，并接收服务器数据
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
	//3、关闭的连接
	socketClientClose(socketFd);
	return 0;
}