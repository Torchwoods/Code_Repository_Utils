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

#include "zbSocSocketServer.h"
#include "zbSocCmd.h"

#define MAX_CLIENTS 50
#define SRPC_TCP_PORT 7788
#define SRPC_MSG_LEN 1
typedef struct{
	void *next;
	int socketFd;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
}socketRecord_t;

socketRecord_t *socketRecordHead = NULL;

/***************************************************************************************************
 * @fn      serverSocketInit
 *
 * @brief   initialises the server.
 * @param   
 *
 * @return  Status
 */
int32 socketSeverInit(void)
{
	struct sockaddr_in serv_addr;
	int stat, tr = 1;

	if (socketRecordHead == NULL)
	{
		// New record
		socketRecord_t *lsSocket = malloc(sizeof(socketRecord_t));

		lsSocket->socketFd = socket(AF_INET, SOCK_STREAM, 0);
		if (lsSocket->socketFd < 0)
		{
			printf("ERROR opening socket");
			return -1;
		}

		// Set the socket option SO_REUSEADDR to reduce the chance of a
		// "Address Already in Use" error on the bind
		setsockopt(lsSocket->socketFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
		// Set the fd to none blocking
		fcntl(lsSocket->socketFd, F_SETFL, O_NONBLOCK);

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(SRPC_TCP_PORT);
		stat = bind(lsSocket->socketFd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr));
		if (stat < 0)
		{
			printf("ERROR on binding: %s\n", strerror(errno));
			return -1;
		}
		//will have 5 pending open client requests
		listen(lsSocket->socketFd, 5);

		lsSocket->next = NULL;
		//Head is always the listening socket
		socketRecordHead = lsSocket;
	}

	//printf("waiting for socket new connection\n");

	return 0;
}
/*********************************************************************
 * @fn      socketSeverGetNumClients()
 *
 * @brief   get clients fd's.
 *
 * @param   none
 *
 * @return  list of Timerfd's
 */
uint32 socketSeverGetNumClients(void)
{
	uint32 recordCnt = 0;
	socketRecord_t *srchRec;

	//printf("socketSeverGetNumClients++\n", recordCnt);

	// Head of the timer list
	srchRec = socketRecordHead;

	if (srchRec == NULL)
	{
		//printf("socketSeverGetNumClients: socketRecordHead NULL\n");
		return -1;
	}

	// Stop when rec found or at the end
	while (srchRec)
	{
		//printf("socketSeverGetNumClients: recordCnt=%d\n", recordCnt);
		srchRec = srchRec->next;
		recordCnt++;
	}

	//printf("socketSeverGetNumClients %d\n", recordCnt);
	return (recordCnt);
}
/*********************************************************************
 * @fn      socketSeverGetClientFds()
 *
 * @brief   get clients fd's.
 *
 * @param   none
 *
 * @return  list of Timerfd's
 */
void socketSeverGetClientFds(int *fds, int maxFds)
{
	uint32 recordCnt = 0;
	socketRecord_t *srchRec;

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
/*********************************************************************
 * @fn      createSocketRec
 *
 * @brief   create a socket and add a rec fto the list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  new clint fd
 */
int createSocketRec(void)
{
	int tr = 1;
	socketRecord_t *srchRec;

	socketRecord_t *newSocket = malloc(sizeof(socketRecord_t));

	//open a new client connection with the listening socket (at head of list)
	newSocket->clilen = sizeof(newSocket->cli_addr);

	//Head is always the listening socket
	newSocket->socketFd = accept(socketRecordHead->socketFd,
			(struct sockaddr *) &(newSocket->cli_addr), &(newSocket->clilen));

	//printf("connected\n");

	if (newSocket->socketFd < 0) printf("ERROR on accept");

    

	// Set the socket option SO_REUSEADDR to reduce the chance of a
	// "Address Already in Use" error on the bind
	setsockopt(newSocket->socketFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
	// Set the fd to none blocking
	fcntl(newSocket->socketFd, F_SETFL, O_NONBLOCK);

	//printf("New Client Connected fd:%d - IP:%s\n", newSocket->socketFd, inet_ntoa(newSocket->cli_addr.sin_addr));

	newSocket->next = NULL;

	//find the end of the list and add the record
	srchRec = socketRecordHead;
	// Stop at the last record
	while (srchRec->next)
		srchRec = srchRec->next;

	// Add to the list
	srchRec->next = newSocket;

	return (newSocket->socketFd);
}

/*********************************************************************
 * @fn      deleteSocketRec
 *
 * @brief   Delete a rec from list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  none
 */
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
		printf("deleteSocketRec: record not found\n");
		return;
	}

	// Does the record exist
	if (srchRec)
	{
		// delete the timer from the list
		if (prevRec == NULL)
		{
			//trying to remove first rec, which is always the listining socket
			printf(
					"deleteSocketRec: removing first rec, which is always the listining socket\n");
			return;
		}

		//remove record from list
		prevRec->next = srchRec->next;

		close(srchRec->socketFd);
		free(srchRec);
	}
}


/*********************************************************************
 * @fn          CheckcalcFcs
 *
 * @brief       check Fcs
 *
 * @param       pBuf - incomin messages sizeof
 *
 * @return      uint8_t
 */
uint8_t CheckcalcFcs(unsigned char *msg,int size)
{
    uint8_t result =0;
    int idx = 1;
    int len = (size -1);
    while((len--) != 0)
    {
        result ^=msg[idx++];
    }

    return (result==msg[size])?0:1;
}
/*********************************************************************
 * @fn          Socket_ProcessIncoming
 *
 * @brief       This function processes incoming messages.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
void Socket_ProcessIncoming(uint8_t *pBuf,uint32_t byteRead, uint32_t clientFd)
{
	uint8_t data = 0;
	uint8_t DataLength = 0;

	if(pBuf[0] == MT_RPC_SOF)
	{
		if(CheckcalcFcs(pBuf,byteRead))
		{
			printf("socket format error\n");
			return;
		}
		
		DataLength = pBuf[1];
		if(DataLength)
		{
			data = pBuf[4];
		}
		
		switch(pBuf[2])
		{
			case MT_RPC_CMD_SET_LED :
				printf("LED SET CMD\n");
				zbSocSetDevice(MT_RPC_CMD_SET_LED,data);
			break;
			case MT_RPC_CMD_SET_BFF :
				printf("BFF SET CMD\n");
				zbSocSetDevice(MT_RPC_CMD_SET_BFF,data);
			break;
			case MT_RPC_CMD_GET_TEMP :
				printf("TEMP GET CMD\n");
				zbSocGetDeviceData(MT_RPC_CMD_GET_TEMP);
			break;
			case MT_RPC_CMD_GET_WARING :
				printf("WARING GET CMD\n");
				zbSocGetDeviceData(MT_RPC_CMD_GET_WARING);
			break;
			case MT_RPC_CMD_GET_HUM :
				printf("HUM GET CMD\n");
				zbSocGetDeviceData(MT_RPC_CMD_GET_HUM);
			break;
			case MT_RPC_CMD_GET_WSPEED :
				printf("WSPEED GET CMD\n");
				zbSocGetDeviceData(MT_RPC_CMD_GET_WSPEED);
			break;
			case MT_RPC_CMD_GET_WDIRC :
				printf("WDIRC GET CMD\n");
				zbSocGetDeviceData(MT_RPC_CMD_GET_WDIRC);
			break;
			case MT_RPC_CMD_GET_LIGHTSENSER :
				printf("LIGHTSENSER GET CMD\n");
				
			break;
			default:
			break;
		}
	}
	return;
}

/***************************************************************************************************
 * @fn      zbSocSocketServerProcess
 *
 * @brief   Callback for Rx'ing SRPC messages.
 *
 * @return  Status
 ***************************************************************************************************/
void zbSocSocketServerProcess(int clientFd)
{
	char buffer[256];
	int byteToRead;
	int byteRead;
	int rtn;

	//获取需要读取的的字节数
	rtn = ioctl(clientFd, FIONREAD, &byteToRead);

	if (0 > rtn)
	{
		printf("SRPC_RxCB: Socket error\n");
	}
	//处理函数
	while (byteToRead)
	{
		
		bzero(buffer, 256);
		byteRead = 0;
		//Get 0xFE Len cmd0 cmd1 data,fcs
		byteRead += read(clientFd, buffer, 4);
		//Get the rest of the message
		//printf("SRPC: reading %x byte message\n",buffer[SRPC_MSG_LEN]);
		byteRead += read(clientFd, &buffer[4], buffer[SRPC_MSG_LEN]+1);
		byteToRead -= byteRead;
		if (byteRead < 0) 
		{
			perror("SRPC ERROR: error reading from socket\n");
			return;
		}
			
		if (byteRead < buffer[SRPC_MSG_LEN])
		{
			perror("SRPC ERROR: full message not read\n");
			return;
		}
		
        for(rtn=0;rtn<byteRead;rtn++)
            printf("%x ",buffer[rtn]);
        printf("\n");

		//printf("Read the message[%x]\n",byteRead);
		Socket_ProcessIncoming((uint8_t*) buffer,byteRead, clientFd);
	}
	
	return;
}

/*********************************************************************
 * @fn      socketSeverPoll()
 *
 * @brief   services the Socket events.
 *
 * @param   clinetFd - Fd to services
 * @param   revent - event to services
 *
 * @return  none
 */
void socketSeverPoll(int clinetFd, int revent)
{
	//printf("pollSocket++\n");

	//is this a new connection on the listening socket
	if (clinetFd == socketRecordHead->socketFd)
	{
		createSocketRec();
	}
	else
	{
		//this is a client socket is it a input or shutdown event
		if (revent & POLLIN)
		{
			zbSocSocketServerProcess(clinetFd);	
		}
		if (revent & POLLRDHUP)
		{
			//its a shut down close the socket
			//printf("Client fd:%d disconnected\n", clinetFd);

			//remove the record and close the socket
			deleteSocketRec(clinetFd);
		}
	}
	//write(clientSockFd,"I got your message",18);

	return;
}

/***************************************************************************************************
 * @fn      socketSeverSend
 *
 * @brief   Send a buffer to a clients.
 * @param   uint8* srpcMsg - message to be sent
 *          int32 fdClient - Client fd
 *
 * @return  Status
 */
int32 socketSeverSend(uint8* buf, uint32 len, int32 fdClient)
{
	int32 rtn;

	//printf("socketSeverSend++: writing to socket fd %d\n", fdClient);

	if (fdClient)
	{
		rtn = write(fdClient, buf, len);
		if (rtn < 0)
		{
			printf("ERROR writing to socket %d\n", fdClient);
			return rtn;
		}
	}

	//printf("socketSeverSend--\n");
	return 0;
}

/***************************************************************************************************
 * @fn      socketSeverSendAllclients
 *
 * @brief   Send a buffer to all clients.
 * @param   uint8* srpcMsg - message to be sent
 *
 * @return  Status
 */
int32 socketSeverSendAllclients(uint8* buf, uint32 len)
{
	int rtn;
	socketRecord_t *srchRec;

	// first client socket
	srchRec = socketRecordHead->next;

	// Stop when at the end or max is reached
	while (srchRec)
	{
		//printf("SRPC_Send: client %d\n", cnt++);
		rtn = write(srchRec->socketFd, buf, len);
		if (rtn < 0)
		{
			printf("ERROR writing to socket %d\n", srchRec->socketFd);
			printf("closing client socket\n");
			//remove the record and close the socket
			deleteSocketRec(srchRec->socketFd);

			return rtn;
		}
		srchRec = srchRec->next;
	}

	return 0;
}

/***************************************************************************************************
 * @fn      socketSeverClose
 *
 * @brief   Closes the client connections.
 *
 * @return  Status
 */
void socketSeverClose(void)
{
	int fds[MAX_CLIENTS], idx = 0;

	socketSeverGetClientFds(fds, MAX_CLIENTS);

	while (socketSeverGetNumClients() > 1)
	{
		printf("socketSeverClose: Closing socket fd:%d\n", fds[idx]);
		deleteSocketRec(fds[idx++]);
	}

	//Now remove the listening socket
	if (fds[0])
	{
		printf("socketSeverClose: Closing the listening socket\n");
		close(fds[0]);
	}
}

