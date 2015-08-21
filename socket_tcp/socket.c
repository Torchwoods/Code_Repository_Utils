/************************************************************************************************************* 
 * �ļ���: 	  socket.c 
 * ����:      socket��ʼ���Ȳ������� 
 * ����:      oxp_edward@163.com 
 * ����ʱ��:    2014��12��31��9:45 
 * ����޸�ʱ��:2014��12��31��9:45 
 * ��ϸ:       	socket��������
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

/*********************Socket Server ������������****************/


/*************************************************************************************************************************
*����        : int socketServiceInit(unsigned int port)
*����        : �����socket��ʼ��
*����        :  port���˿ں�
*����        :  socketFd
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��ʹ��epoll��ʱ����Ҫ����socketΪ������ģʽ
*************************************************************************************************************************/
int socketServiceInit(unsigned int port)
{
	struct sockaddr_in serv_addr;
	int ret;
	
	if(NULL != socketRecordHead)
	{
		return -1;
	}
	//1�������ڴ�ռ����ڴ洢socket��һЩ����
	socketRecord_t *lsSocket = malloc(sizeof(socketRecord_t));	
	
	do{
		if(NULL == lsSocket)
			break;
		//2������һ��socket�������͵�IPV4 ����ʽ�׽��֡�
		lsSocket->socketFd = socket(AF_INET,SOCK_STREAM,0);
		if(-1 == lsSocket->socketFd)
		{
			Log("Error opening Socket: \n");
			break;
		}
		//3�����õ�Ϊ������ģʽ
		fcntl(lsSocket->socketFd,F_SETFL,O_NONBLOCK);
		bzero(&serv_addr,sizeof(struct sockaddr_in));
		//4�����������˵�sockaddr�ṹ
		serv_addr.sin_family = AF_INET; //IPV4 
		//(���������ϵ�long����ת��Ϊ�����ϵ�long����)�������������������κ�ip�������� //INADDR_ANY ��ʾ��������������IP��ַ�� ��������������԰󶨵����е�IP��
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		//server_addr.sin_addr.s_addr=inet_addr("192.1 68.1 .1 "); //�� �ڰ󶨵�һ���̶�IP,inet_addr�� �ڰ����ּӸ�ʽ��ipת��Ϊ����ip
		serv_addr.sin_port = htons(port);//���ö˿ں�(���������ϵ�short����ת��Ϊ�����ϵ�short����)
		//5������socketfd��������IP��ַ
		ret = bind(lsSocket->socketFd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
		if(-1 == ret)
		{
			Log("Error on binding: \n");
			break;
		}
		//6�������������ӵ����ͻ��˵���Ŀ
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
*����        : int createSocketRec(void)
*����        : �����socket��ʼ��
*����        : ��
*����        : socketfd
				-1 ʧ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��ʹ��epoll��ʱ����Ҫ����socketΪ������ģʽ
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
		//���տͻ�����������
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
*����        : void deleteSocketRec(int rmSocketFd)
*����        : ����˹ر���������
*����        : rmSocketFd: 
*����        : ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : �ڹرյ�ͬʱ����Ҫ����socketFD�ӵ�������ɾ��
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
*����        : int socketSeverGetNumClients(void)
*����        : ����˻�ȡ�Ѿ����ӵĿͻ�����Ŀ
*����        : ��
*����        : -1��ʧ��
				>0 :�ͻ�����Ŀ
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ���е��Ѿ������ϵĿͻ�����Ϣ��������������
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
*����        : void socketSeverGetClientFds(int *fds, int maxFds)
*����        : ����˹ر���������
*����        : fds: �洢socketFd������
			   maxFds��fds�����С
*����        : ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��������õ�����socketfd
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
*����        : void socketSeverClose(void)
*����        : ����˹ر���������
*����        : �� 
*����        : ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : �ڹرյ�ͬʱ����Ҫ����socketFD�ӵ�������ɾ��
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
*����        : int socketSeverSendAllclients(unsigned char* buf, int len)
*����        : ����������еĿͻ��˷�������
*����        : buf�����ݴ洢 
			   len�����ݳ���
*����        : 0���ɹ�
			  -1��ʧ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��
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

/*********************Socket Client ������������****************/

/*************************************************************************************************************************
*����        : int SocketClientInit(const char *addr,const char* port)
*����        : �ͻ��˳�ʼ��
*����        : addr:IP��ַ
			   port:�˿ں�
*����        : socketfd
				-1 ʧ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��
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
*����        : int socketClientClose(int socketFd)
*����        : �ͻ��˹ر���������
*����        : socketFd: �ͻ����ļ�������
*����        : 0���ɹ�
			  -1��ʧ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��
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

/*********************����������������****************/
/*************************************************************************************************************************
*����        : int socketSend(int fdClient,unsigned char* buf, int len)
*����        : ���������ݺ���
*����        : fdClient: socket�ļ�������
			   buf:���ݴ洢��
			   len:���ݳ���
*����        : ����д����ֽ���
				-1 д��ʧ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��
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
*����        : int SocketRecv(int fdClient,unsigned char *buf,int len)
*����        : �������ݺ���
*����        : fdClient: socket�ļ�������
			   buf:���ݴ洢��
			   len:���ݳ���
*����        : ���ض�ȡ���ֽ���
				-1 ��ȡʧ��
*����        : ��
*����        : edward
*ʱ��        : 20141231
*����޸�ʱ��: 20141231
*˵��        : ��
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
