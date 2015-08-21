/************************************************************************************************************* 
 * 文件名: 	  socket.c 
 * 功能:      socket初始化等操作函数 
 * 作者:      oxp_edward@163.com 
 * 创建时间:    2014年12月31日9:45 
 * 最后修改时间:2014年12月31日9:45 
 * 详细:       	socket操作函数
*************************************************************************************************************/  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <assert.h>
#include "socket.h"
#define MAX_CLIENTS 50
socketRecord_t *socketRecordHead = NULL;

/*********************Socket Server 操作函数部分****************/


/*************************************************************************************************************************
*函数        : int socketServiceInit(unsigned int port)
*功能        : 服务端socket初始化
*参数        :  port：端口号
*返回        :  socketFd
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 在使用epoll的时候需要设置socket为非阻塞模式
*************************************************************************************************************************/
int socketServiceInit(unsigned int port)
{
	struct sockaddr_in serv_addr;
	int ret;
	
	if(NULL != socketRecordHead)
	{
		return -1;
	}
	//1、分配内存空间用于存储socket的一些数据
	socketRecord_t *lsSocket = malloc(sizeof(socketRecord_t));	
	
	do{
		if(NULL == lsSocket)
			break;
		//2、创建一个socket连接类型的IPV4 ，流式套接字。
		lsSocket->socketFd = socket(AF_INET,SOCK_STREAM,0);
		if(-1 == lsSocket->socketFd)
		{
			Log("Error opening Socket: \n");
			break;
		}
		//3、设置的为非阻塞模式
		fcntl(lsSocket->socketFd,F_SETFL,O_NONBLOCK);
		bzero(&serv_addr,sizeof(struct sockaddr_in));
		//4、填充服务器端的sockaddr结构
		serv_addr.sin_family = AF_INET; //IPV4 
		//(将本机器上的long数据转化为网络上的long数据)服务器程序能运行在任何ip的主机上 //INADDR_ANY 表示主机可以是任意IP地址， 即服务器程序可以绑定到所有的IP上
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		//server_addr.sin_addr.s_addr=inet_addr("192.1 68.1 .1 "); //用 于绑定到一个固定IP,inet_addr用 于把数字加格式的ip转化为整形ip
		serv_addr.sin_port = htons(port);//设置端口号(将本机器上的short数据转化为网络上的short数据)
		//5、捆绑socketfd描述符到IP地址
		ret = bind(lsSocket->socketFd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
		if(-1 == ret)
		{
			Log("Error on binding: \n");
			break;
		}
		//6、设置允许连接的最大客户端的数目
		ret = listen(lsSocket->socketFd,10);
		if(-1 == ret)
		{
			Log("Error on Listen: \n");
			break;
		}
		lsSocket->next = NULL;
		socketRecordHead = lsSocket;
		Log("Service IP %s:%d\n",inet_ntoa(serv_addr.sin_addr),port);
		return lsSocket->socketFd;
	}while(0);
	
	free(lsSocket);
	return -1;
}

/*************************************************************************************************************************
*函数        : int createSocketRec(void)
*功能        : 服务端socket初始化
*参数        : 无
*返回        : socketfd
				-1 失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 在使用epoll的时候需要设置socket为非阻塞模式
*************************************************************************************************************************/
int createSocketRec(void)
{
	socketRecord_t *srchRec;
	int ret ,tr = 1;
	if(NULL == socketRecordHead)
	{
		return -1;
	}
	
	socketRecord_t *newSocket = malloc(sizeof(socketRecord_t));
	do{
		if(NULL == newSocket)break;
		newSocket->clilen = sizeof(newSocket->cli_addr);
		//接收客户端连接请求
		newSocket->socketFd = accept(socketRecordHead->socketFd,(struct sockaddr *) &(newSocket->cli_addr), &(newSocket->clilen));

		if(0 > newSocket->socketFd)
		{
			Log("Error on accept: \n");
			break;
		}
		fcntl(newSocket->socketFd, F_SETFL, O_NONBLOCK);
		newSocket->next = NULL;
		srchRec = socketRecordHead;
		while (srchRec->next)
			srchRec = srchRec->next;
		srchRec->next = newSocket;
		
		Log("content from %s\n",inet_ntoa(newSocket->cli_addr.sin_addr));
		return (newSocket->socketFd);
	}while(0);
	Log("Create error\n");
	free(newSocket);
	return -1;
}
/*************************************************************************************************************************
*函数        : void deleteSocketRec(int rmSocketFd)
*功能        : 服务端关闭连接请求
*参数        : rmSocketFd: 
*返回        : 无
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 在关闭的同时，需要将该socketFD从单链表中删除
*************************************************************************************************************************/
void deleteSocketRec(int rmSocketFd)
{
	socketRecord_t *srchRec, *prevRec = NULL;

	// Head of the timer list
	srchRec = socketRecordHead;

	// Stop when rec found or at the end
	while ((srchRec->socketFd != rmSocketFd) && (srchRec->next))
	{
		prevRec = srchRec;
		// over to next
		srchRec = srchRec->next;
	}

	if (srchRec->socketFd != rmSocketFd)
	{
		Log("deleteSocketRec: record not found\n");
		return;
	}

	// Does the record exist
	if (srchRec)
	{
		// delete the timer from the list
		if (prevRec == NULL)
		{
			//trying to remove first rec, which is always the listining socket
			Log(
					"deleteSocketRec: removing first rec, which is always the listining socket\n");
			return;
		}

		//remove record from list
		prevRec->next = srchRec->next;

		close(srchRec->socketFd);
		free(srchRec);
	}
}
/*************************************************************************************************************************
*函数        : int socketSeverGetNumClients(void)
*功能        : 服务端获取已经连接的客户端数目
*参数        : 无
*返回        : -1：失败
				>0 :客户端数目
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 所有的已经连接上的客户端信息都保存在链表中
*************************************************************************************************************************/
int socketSeverGetNumClients(void)
{
	int recordCnt = 0;
	socketRecord_t *srchRec;

	//Log("socketSeverGetNumClients++\n", recordCnt);

	// Head of the timer list
	srchRec = socketRecordHead;

	if (srchRec == NULL)
	{
		//Log("socketSeverGetNumClients: socketRecordHead NULL\n");
		return -1;
	}

	// Stop when rec found or at the end
	while (srchRec)
	{
		//Log("socketSeverGetNumClients: recordCnt=%d\n", recordCnt);
		srchRec = srchRec->next;
		recordCnt++;
	}

	//Log("socketSeverGetNumClients %d\n", recordCnt);
	return (recordCnt);
}
/*************************************************************************************************************************
*函数        : void socketSeverGetClientFds(int *fds, int maxFds)
*功能        : 服务端关闭连接请求
*参数        : fds: 存储socketFd的数组
			   maxFds：fds数组大小
*返回        : 无
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 遍历链表得到所有socketfd
*************************************************************************************************************************/
void socketSeverGetClientFds(int *fds, int maxFds)
{
	int recordCnt = 0;
	socketRecord_t *srchRec;

	assert(fds!=NULL);
	// Head of the timer list
	srchRec = socketRecordHead;

	// Stop when at the end or max is reached
	while ((srchRec) && (recordCnt < maxFds))
	{
		//printf("getClientFds: adding fd%d, to idx:%d \n", srchRec->socketFd, recordCnt);
		fds[recordCnt++] = srchRec->socketFd;

		srchRec = srchRec->next;
	}

	return;
}
/*************************************************************************************************************************
*函数        : void socketSeverClose(void)
*功能        : 服务端关闭连接请求
*参数        : 无 
*返回        : 无
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 在关闭的同时，需要将该socketFD从单链表中删除
*************************************************************************************************************************/
void socketSeverClose(void)
{
	int fds[MAX_CLIENTS], idx = 0;

	socketSeverGetClientFds(fds, MAX_CLIENTS);

	while (socketSeverGetNumClients() > 1)
	{
		Log("socketSeverClose: Closing socket fd:%d\n", fds[idx]);
		deleteSocketRec(fds[idx++]);
	}

	//Now remove the listening socket
	if (fds[0])
	{
		Log("socketSeverClose: Closing the listening socket\n");
		close(fds[0]);
	}
}
/*************************************************************************************************************************
*函数        : int socketSeverSendAllclients(unsigned char* buf, int len)
*功能        : 服务端向所有的客户端发送数据
*参数        : buf：数据存储 
			   len：数据长度
*返回        : 0：成功
			  -1：失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/
int socketSeverSendAllclients(unsigned char* buf, int len)
{
	int rtn;
	socketRecord_t *srchRec;

	assert(buf != NULL);
	// first client socket
	srchRec = socketRecordHead->next;

	// Stop when at the end or max is reached
	while (srchRec)
	{
		//printf("SRPC_Send: client %d\n", cnt++);
		rtn = write(srchRec->socketFd, buf, len);
		if (rtn < 0)
		{
			Log("ERROR writing to socket %d\n", srchRec->socketFd);
			Log("closing client socket\n");
			//remove the record and close the socket
			deleteSocketRec(srchRec->socketFd);

			return rtn;
		}
		srchRec = srchRec->next;
	}

	return 0;
}

/*********************Socket Client 操作函数部分****************/

/*************************************************************************************************************************
*函数        : int SocketClientInit(const char *addr,const char* port)
*功能        : 客户端初始化
*参数        : addr:IP地址
			   port:端口号
*返回        : socketfd
				-1 失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/
int SocketClientInit(const char *addr,const char* port)
{
	int rtn;
	int socketFd;
	struct sockaddr_in server_addr;
	
	assert(addr!=NULL);
	assert(port!=NULL);
	
	socketFd = socket(AF_INET,SOCK_STREAM,0);
	if(0 > socketFd)
	{
		Log("Error on socket\n");
		return -1;
	}
	
	bzero(&server_addr,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = inet_addr(addr);
	
	rtn = connect(socketFd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr));
	if(0 > rtn)
	{
		Log("Error on connect\n");
		return -1;
	}
	
	return socketFd;
}
/*************************************************************************************************************************
*函数        : int socketClientClose(int socketFd)
*功能        : 客户端关闭连接请求
*参数        : socketFd: 客户端文件描述符
*返回        : 0：成功
			  -1：失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/
int socketClientClose(int socketFd)
{
	if(socketFd)
	{
		close(socketFd);
		return 0;
	}
	return -1;
}

/*********************公共操作函数部分****************/
/*************************************************************************************************************************
*函数        : int socketSend(int fdClient,unsigned char* buf, int len)
*功能        : 服发送数据函数
*参数        : fdClient: socket文件描述符
			   buf:数据存储区
			   len:数据长度
*返回        : 返回写入的字节数
				-1 写入失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/
int socketSend(int fdClient,unsigned char* buf, int len)
{
	int rtn = -1;

	//Log("socketSend++: writing to socket fd %d\n", fdClient);

	assert(NULL != buf);
	
	if (fdClient)
	{
		rtn = write(fdClient, buf, len);
		if (rtn < 0)
		{
			Log("ERROR writing to socket %d\n", fdClient);
			return rtn;
		}
	}

	//Log("socketSend--\n");
	return rtn;
}

/*************************************************************************************************************************
*函数        : int SocketRecv(int fdClient,unsigned char *buf,int len)
*功能        : 接收数据函数
*参数        : fdClient: socket文件描述符
			   buf:数据存储区
			   len:数据长度
*返回        : 返回读取的字节数
				-1 读取失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/
int SocketRecv(int fdClient,unsigned char *buf,int len)
{
	int rtn;
	
	
	assert(NULL != buf);
	
	if(fdClient)
	{
		rtn = read(fdClient,buf,len);
		if (rtn < 0)
		{
			Log("ERROR Reading to socket %d\n", fdClient);
			return rtn;
		}
	}
	
	return rtn;
}
