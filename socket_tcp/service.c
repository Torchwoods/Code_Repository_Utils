/************************************************************************************************************* 
 * 文件名: 	  		service.c 
 * 功能:      		基于TCP创建socket服务端程序
 * 作者:      		oxp_edward@163.com 
 * 创建时间:    	2014年12月31日9:45 
 * 最后修改时间:	2012年3月12日 
 * 详细:      		根据的socket建立TCP连接过程，创建服务程序，
					并在服务端等待接收和处理客户端的数据，
					并将结果输出到终端上
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

#define MAXFDS 256

int main(int argc,char**argv)
{
	int socketFd;
	int nfds;
	int epfd;
	int NewsocketFd;
	int Count;
	int rtn;
	unsigned char buf[1024];
	struct epoll_event ev, events[MAXFDS];
	
	if(2 > argc)
	{
		Log("Input error\n");
		Log("Usage: %s <port>\n",argv[0]);
		Log("Eample: %s 1234\n",argv[0]);
		return -1;
	}
	
	//1、服务器初始化
	socketFd  = socketServiceInit(atoi(argv[1]));
	if(0 > socketFd)
	{
		Log("Error socketServiceInit\n");
		return -1;
	}
	
	
	do{
		//~向内核申请MAXFDS+1大小的存储fd的空间
		epfd = epoll_create(MAXFDS+1);
		if(0 > epfd)
		{
			Log("Error epoll_create\n"); 
			break;
		}
		//~设置需要监听的fd和触发事件类型
		ev.data.fd = socketFd;
		/*
		EPOLLIN ：表示对应的文件描述符可以读； 
		EPOLLOUT：表示对应的文件描述符可以写； 
		EPOLLPRI：表示对应的文件描述符有紧急的数据可读 
		EPOLLERR：表示对应的文件描述符发生错误； 
		EPOLLHUP：表示对应的文件描述符被挂断； 
		EPOLLET：表示对应的文件描述符有事件发生； 
		*/
		ev.events  =  EPOLLIN | EPOLLET;
		//~控制某个epoll文件描述符上的事件，可以注册事件，修改事件，删除事件
		rtn = epoll_ctl(epfd,EPOLL_CTL_ADD,socketFd,&ev);
		/*
		参数： 
		epfd：由 epoll_create 生成的epoll专用的文件描述符； 
		op：要进行的操作例如注册事件，可能的取值
			EPOLL_CTL_ADD 注册、
			EPOLL_CTL_MOD 修 改、
			EPOLL_CTL_DEL 删除 

		fd：关联的文件描述符； 
		event：指向epoll_event的指针； 
			如果调用成功返回0,不成功返回-1 
		*/
		if(0 > rtn)
		{
			Log("Error epoll_ctl\n"); 
			break;
		}
		
		for(;;)
		{	//~轮询I/O事件的发生
			nfds = epoll_wait(epfd,events,MAXFDS,0);
			/*
			epfd:由epoll_create 生成的epoll专用的文件描述符； 
			epoll_event:用于回传代处理事件的数组； 
			maxevents:每次能处理的事件数； 
			timeout:等待I/O事件发生的超时值；
				-1相当于阻塞，
				0相当于非阻塞。
				一般用-1即可返回发生事件数。 
			*/
			for(Count = 0;Count<nfds;++Count)
			{	
				//创建新的请求连接
				if(socketFd == events[Count].data.fd)
				{
					//2、接收客户端的连接请求
					NewsocketFd = createSocketRec();
					if(0 > NewsocketFd)
					{
						continue;
					}
					//将客户端的FD放入epoll中
					ev.data.fd = NewsocketFd;
					ev.events = EPOLLIN | EPOLLET;
					epoll_ctl(epfd,EPOLL_CTL_ADD,NewsocketFd,&ev);
				//接收到数据，读socket 	
				}else if(events[Count].events & EPOLLIN){
					//Log("read");
					if(socketFd == events[Count].data.fd) continue;
					bzero(buf,sizeof(buf));
					//3、接收客户端数据
					rtn = SocketRecv(events[Count].data.fd,buf,sizeof(buf));
					if(0 == rtn)
					{
						Log("Connect Close \n");
						epoll_ctl(epfd, EPOLL_CTL_DEL, events[Count].data.fd, NULL);
						deleteSocketRec(events[Count].data.fd);
						continue;
					}
					Log("[Recv]: %s",buf);
					//4、发送数据到客户端
					socketSeverSendAllclients(buf,sizeof(buf));
				//有数据待发送，写socket 
				}else if(events[Count].events & EPOLLOUT){
					//Log("write\n");
					strcpy(buf,"data from service");
					socketSeverSendAllclients(buf,sizeof(buf));
				}	
			}
		}
		
	}while(0);
	//5、关闭所有的连接，断开TCP请求
	socketSeverClose();
	return 0;
}





