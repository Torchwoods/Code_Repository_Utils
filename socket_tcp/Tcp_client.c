/*******************************************************************************
 Filename:       tcp_client.h
 Revised:        $Date$
 Revision:       $Revision$

 Description:   Client communication via TCP ports


 Copyright 2013 Texas Instruments Incorporated. All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License").  You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless used solely and exclusively in conjunction with
 a Texas Instruments radio frequency device, which is integrated into
 your product.  Other than for the foregoing purpose, you may not use,
 reproduce, copy, prepare derivative works of, modify, distribute, perform,
 display or sell this Software and/or its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,l
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>   
#include <fcntl.h>    
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "Tcp_client.h"
#include "Polling.h"
#include "LogUtils.h"
#include "Types.h"
#include "globalVal.h"
#include "comMsgPrivate.h"


/******************************************************************************
 * Constants
 *****************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
dataBuffer_t  RemoteSocketData;//存储socket数据
server_details_t *SocketHead;


/******************************************************************************
 * Function Prototypes
 *****************************************************************************/
void tcp_socket_event_handler(server_details_t * server_details);
void tcp_socket_reconnect_to_server(server_details_t * server_details);

/******************************************************************************
 * Functions
 *****************************************************************************/
int tcp_send_packet(server_details_t * server_details, uint8_t * buf, int len)
{
	if (write(polling_fds[server_details->fd_index].fd, buf, len) != len)
	{
		return -1;
	}

	return 0;
}

//添加新连接
int tcp_new_Client_connection(server_details_t * server_details, server_incoming_data_handler_t server_incoming_data_handler, char * name, server_connected_disconnected_handler_t server_connected_disconnected_handler)
{

/*
	struct hostent *server;
	//返回对应于给定主机名的包含主机名字和地址信息的hostent结构指针
    server = gethostbyname(hostname);
    if (server == NULL) 
	{
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        return -1;
    }
	
	bzero((char *) &server_details->serveraddr, sizeof(server_details->serveraddr));
	server_details->serveraddr.sin_family = AF_INET;//AF_INET Ipv4网络协议
	//填写IP地址
	memcpy((char *)&server_details->serveraddr.sin_addr.s_addr,(char *)server->h_addr,server->h_length);
	server_details->serveraddr.sin_port = htons(port);
*/
	server_details->server_incoming_data_handler = server_incoming_data_handler;
	server_details->server_reconnection_timer.in_use = false;
	server_details->name = name;
	server_details->server_connected_disconnected_handler = server_connected_disconnected_handler;
	server_details->connected = false;
	
	tcp_socket_reconnect_to_server(server_details);
	
	return 0;
}

int tcp_disconnect_from_server(server_details_t * server)
{
	//TBD
	return 0;
}

//非阻塞connect连接
int tcp_conn_nonb(int sockfd, const struct sockaddr_in *saptr, socklen_t salen, int nsec)
{
    int flags, n, error, code;
	int ret = -1;
    socklen_t len;
    fd_set rset,wset;
    struct timeval tval;
	//设置非阻塞
    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	errno = 0;
	ret = connect(sockfd,saptr,salen);
	if(ret < 0 && errno != EINPROGRESS)
	{
		return -1;
	}

	if(ret == 0)
		goto done;
		
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	
	tval.tv_sec = nsec;
	tval.tv_usec = 0;
	//如果nsec为0，将使用缺省的超时时间，即其结构指针为NULL
	//如果过tval结构中的时间为0，表示不做任何等待，立即返回
	if ((n = select(sockfd+1, &rset, &wset,NULL, nsec ? &tval : NULL)) == 0)
	{
		errno = ETIMEDOUT;
		return -1;
	}

	if(FD_ISSET(sockfd,&rset)||FD_ISSET(sockfd,&wset))
	{
		len = sizeof(error);
		if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)<0)
			return -1;
	}else{
		LOG_PRINT("select error:sockfd not set\n");
	}
done:	
	fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */
	if(error)
	{
		errno=error;
		return -1;
	}
	return 0;
}

int tcp_connect_to_server(server_details_t * server_details)
{
	int fd;
	int tr;
	struct hostent *server;
	char Seriveip[16] = {0};
	uint16 ServicePort = 0;

	if(getWifiServerInfofile(Seriveip,&ServicePort))
	{
		LOG_PRINT("Can't get Remote Service Information\n");
		return -1;
	}
	
	LOG_PRINT("Service Information: %s:%d\n",Seriveip,ServicePort);

	//返回对应于给定主机名的包含主机名字和地址信息的hostent结构指针
    server = gethostbyname(Seriveip);
    if (server == NULL) 
	{
        fprintf(stderr,"ERROR, no such host as %s\n", Seriveip);
        return -1;
    }
	
	bzero((char *) &server_details->serveraddr, sizeof(server_details->serveraddr));
	server_details->serveraddr.sin_family = AF_INET;//AF_INET Ipv4网络协议
	//填写IP地址
	memcpy((char *)&server_details->serveraddr.sin_addr.s_addr,(char *)server->h_addr,server->h_length);
	server_details->serveraddr.sin_port = htons(ServicePort);
	
	//创建SOCKET，IPV4 TCP
	fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (fd < 0)
	{
		LOG_PRINT("ERROR opening socket");
	}//连接
	//else if(connect(fd, &server_details->serveraddr,sizeof(server_details->serveraddr))==-1)
	else if (tcp_conn_nonb(fd, &server_details->serveraddr, sizeof(server_details->serveraddr),1) == -1)
	{
		close(fd);
	}
	else
	{	
		//do_register(fd);
		//将当前socket添加到FD集合中，并设置回调函数，记录该FD位于集合的位置
		if ((server_details->fd_index = polling_define_poll_fd(fd, POLLIN, (event_handler_cb_t)tcp_socket_event_handler, server_details)) != -1)
		{
			LOG_PRINT("Successfully connected to %s server\n", server_details->name);

			server_details->connected = true;
			//tu_set_timer
			if (server_details->server_connected_disconnected_handler != NULL)
			{
				server_details->server_connected_disconnected_handler();
			}
			return 0;
		}

		LOG_PRINT("ERROR adding poll fd");

		close(fd);
	}
	return -1;
}

//网络连接连接
void tcp_socket_reconnect_to_server(server_details_t * server_details)
{
	if (tcp_connect_to_server(server_details) == -1)
	{
		tu_set_timer(&server_details->server_reconnection_timer, SERVER_RECONNECTION_RETRY_TIME, false , (timer_handler_cb_t)tcp_socket_reconnect_to_server ,server_details);
	}
}

void tcp_socket_cacheBuf_clear(void)
{
	memset(&RemoteSocketData,0,sizeof(RemoteSocketData));
}

//所有TCP连接事件的处理函数  poll_fds_sidestruct[i].event_handler_cb
void tcp_socket_event_handler(server_details_t * server_details)
{
	int remaining_len;
	hostCmd cmd;

	//非阻塞方式读取数据，
	remaining_len = recv(polling_fds[server_details->fd_index].fd,&RemoteSocketData.buffer[RemoteSocketData.bufferNum], (MaxCacheBufferLength-RemoteSocketData.bufferNum), MSG_DONTWAIT);
	//读取错误
	if (remaining_len < 0)
	{
		LOG_PRINT("ERROR reading from socket (server %s)\n", server_details->name);
	}
	else if (remaining_len == 0)//连接关闭
	{
		LOG_PRINT("Server %s disconnected", server_details->name);
		close(polling_fds[server_details->fd_index].fd);
		polling_undefine_poll_fd(server_details->fd_index);
		server_details->connected = false;
		
		if (server_details->server_connected_disconnected_handler != NULL)
		{
			server_details->server_connected_disconnected_handler();
		}
		//重新连接
		tcp_socket_reconnect_to_server(server_details);
	}
	else
	{
		RemoteSocketData.bufferNum += remaining_len;
		while (RemoteSocketData.bufferNum >= MinCmdPacketLength)
		{
			uint16 packetHeaderPos=0;
			uint16 cmdPacketLen;
			uint8 cmdPacketFCS;
			bzero(&cmd, sizeof(cmd));

			if(lookupFirstPacketHeader(RemoteSocketData.buffer,RemoteSocketData.bufferNum,&packetHeaderPos) == FindNoHeader)
			{
				tcp_socket_cacheBuf_clear();
				break;
			}

			//丢弃非法包头数据
			if(packetHeaderPos != 0)
			{
				memmove(RemoteSocketData.buffer,&RemoteSocketData.buffer[packetHeaderPos],(RemoteSocketData.bufferNum-packetHeaderPos));
				RemoteSocketData.bufferNum -= packetHeaderPos;
			}

			cmdPacketLen = (((RemoteSocketData.buffer[CMD_MSG_LEN0_POS]<<8)&0xff00)|(RemoteSocketData.buffer[CMD_MSG_LEN1_POS]&0x00ff));

			LOG_PRINT("Message cmd length = %d recvData.bufferNum=%d\n",cmdPacketLen,RemoteSocketData.bufferNum);

			// 2byte(TP)+6byte(Mac)+1byte(Dir)+2byte(D0,D1)
			if((cmdPacketLen+4) > RemoteSocketData.bufferNum)
			{
				LOG_PRINT("[localServer]: Packet length error,Wait to receive other datas\n");
				break;
			}

			if(checkPacketFCS(&RemoteSocketData.buffer[CMD_MSG_LEN0_POS],cmdPacketLen+3)==true)
			{
				memcpy(cmd.data,&RemoteSocketData.buffer[CMD_MSG_TP_POS],cmdPacketLen);
				cmd.size = cmdPacketLen;
				cmd.idx = 0;
				LOG_PRINT("recvMsg_ProcessIncoming\n");
				server_details->server_incoming_data_handler(&cmd);
			}else{
				LOG_PRINT("Drop the error fcs data packet.\n");
			}

			if(RemoteSocketData.bufferNum>=(cmdPacketLen+4))
			{
				memmove(RemoteSocketData.buffer,&RemoteSocketData.buffer[cmdPacketLen+4],(RemoteSocketData.bufferNum-(cmdPacketLen+4)));
				RemoteSocketData.bufferNum -= (cmdPacketLen+4);
			}else{
				tcp_socket_cacheBuf_clear();
			}
		}
		
	}
}
//===========================================================================

void tcp_Server_deleteSocketRec(server_details_t *server_details)
{
	server_details_t *srchRec, *prevRec = NULL;

	// Head of the timer list
	srchRec = SocketHead;

	// Stop when rec found or at the end
	while ((polling_fds[srchRec->fd_index].fd != polling_fds[server_details->fd_index].fd) && (srchRec->next))
	{
		prevRec = srchRec;
		// over to next
		srchRec = srchRec->next;
	}

	if (polling_fds[srchRec->fd_index].fd  != polling_fds[server_details->fd_index].fd)
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
			printf("deleteSocketRec: removing first rec, which is always the listining socket\n");
			return;
		}

		//remove record from list
		prevRec->next = srchRec->next;

		close(polling_fds[srchRec->fd_index].fd);
		polling_undefine_poll_fd(srchRec->fd_index);
		srchRec->connected = false;
		free(srchRec);
	}
}

void tcp_Server_processClent(server_details_t *server_details)
{
	
	int remaining_len;
	hostCmd cmd;

	//非阻塞方式读取数据，
	remaining_len = recv(polling_fds[server_details->fd_index].fd,&RemoteSocketData.buffer[RemoteSocketData.bufferNum], (MaxCacheBufferLength-RemoteSocketData.bufferNum), MSG_DONTWAIT);
	//读取错误
	if (remaining_len < 0)
	{
		LOG_PRINT("ERROR reading from socket (server %s)\n", server_details->name);
	}
	else if (remaining_len == 0)//连接关闭
	{
		LOG_PRINT("Server %s disconnected\n", server_details->name);
		//tcp_Server_deleteSocketRec(server_details);
		if (server_details->server_connected_disconnected_handler != NULL)
		{
			server_details->server_connected_disconnected_handler();
		}
	}
	else
	{
		RemoteSocketData.bufferNum += remaining_len;
		while (RemoteSocketData.bufferNum >= MinCmdPacketLength)
		{
			uint16 packetHeaderPos=0;
			uint16 cmdPacketLen;
			uint8 cmdPacketFCS;
			bzero(&cmd, sizeof(cmd));

			if(lookupFirstPacketHeader(RemoteSocketData.buffer,RemoteSocketData.bufferNum,&packetHeaderPos) == FindNoHeader)
			{
				tcp_socket_cacheBuf_clear();
				break;
			}

			//丢弃非法包头数据
			if(packetHeaderPos != 0)
			{
				memmove(RemoteSocketData.buffer,&RemoteSocketData.buffer[packetHeaderPos],(RemoteSocketData.bufferNum-packetHeaderPos));
				RemoteSocketData.bufferNum -= packetHeaderPos;
			}

			cmdPacketLen = (((RemoteSocketData.buffer[CMD_MSG_LEN0_POS]<<8)&0xff00)|(RemoteSocketData.buffer[CMD_MSG_LEN1_POS]&0x00ff));

			LOG_PRINT("Message cmd length = %d recvData.bufferNum=%d\n",cmdPacketLen,RemoteSocketData.bufferNum);

			// 2byte(TP)+6byte(Mac)+1byte(Dir)+2byte(D0,D1)
			if((cmdPacketLen+4) > RemoteSocketData.bufferNum)
			{
				LOG_PRINT("[localServer]: Packet length error,Wait to receive other datas\n");
				break;
			}

			if(checkPacketFCS(&RemoteSocketData.buffer[CMD_MSG_LEN0_POS],cmdPacketLen+3)==true)
			{
				memcpy(cmd.data,&RemoteSocketData.buffer[CMD_MSG_TP_POS],cmdPacketLen);
				cmd.size = cmdPacketLen;
				cmd.idx = 0;
				LOG_PRINT("recvMsg_ProcessIncoming\n");
				server_details->server_incoming_data_handler(&cmd);
			}else{
				LOG_PRINT("Drop the error fcs data packet.\n");
			}

			if(RemoteSocketData.bufferNum>=(cmdPacketLen+4))
			{
				memmove(RemoteSocketData.buffer,&RemoteSocketData.buffer[cmdPacketLen+4],(RemoteSocketData.bufferNum-(cmdPacketLen+4)));
				RemoteSocketData.bufferNum -= (cmdPacketLen+4);
			}else{
				tcp_socket_cacheBuf_clear();
			}
		}
		
	}
}

void tcp_ServerClient_handler(server_details_t *server_details)
{
	//this is a client socket is it a input or shutdown event
    if (polling_fds[server_details->fd_index].revents & POLLIN)
    {
    	if(server_details->server_incoming_data_handler)
		{
			tcp_Server_processClent(server_details);
		}
    }

    if (polling_fds[server_details->fd_index].revents & POLLRDHUP)
    {
        //its a shut down close the socket
        printf("Client fd:%d disconnected\n", polling_fds[server_details->fd_index].fd);
        //remove the record and close the socket
        tcp_Server_deleteSocketRec(server_details);
    }
}

void tcp_Server_handler(server_details_t *server_details)
{
	int tr = 1;
	int serverFd;
	server_details_t *srchRec;

	server_details_t *newSocket = malloc(sizeof(server_details_t));

	//newSocket init;
	bzero((char *) &newSocket->serveraddr, sizeof(newSocket->serveraddr));
	newSocket->serveraddr.sin_family = AF_INET;//AF_INET Ipv4网络协议
	newSocket->server_incoming_data_handler = server_details->server_incoming_data_handler;
	newSocket->server_reconnection_timer.in_use = false;
	newSocket->name = "Client";
	newSocket->server_connected_disconnected_handler = server_details->server_connected_disconnected_handler;
	newSocket->connected = false;

	//open a new client connection with the listening socket (at head of list)
	newSocket->clilen = sizeof(newSocket->serveraddr);

	//Head is always the listening socket
	serverFd= accept(polling_fds[server_details->fd_index].fd,
			(struct sockaddr *) &(newSocket->serveraddr), &(newSocket->clilen));

	//printf("connected\n");

	if (serverFd < 0) printf("ERROR on accept");

	// Set the socket option SO_REUSEADDR to reduce the chance of a
	// "Address Already in Use" error on the bind
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
	// Set the fd to none blocking
	fcntl(serverFd, F_SETFL, O_NONBLOCK);

	LOG_PRINT("New Client Connected fd:%d - IP:%s\n", serverFd, inet_ntoa(newSocket->serveraddr.sin_addr));
	newSocket->next = NULL;

	//find the end of the list and add the record
	srchRec = SocketHead;
	// Stop at the last record
	while (srchRec->next)
		srchRec = srchRec->next;

	// Add to the list
	srchRec->next = newSocket;

	if ((newSocket->fd_index = polling_define_poll_fd(serverFd, POLLIN | POLLRDHUP, (event_handler_cb_t)tcp_ServerClient_handler, newSocket)) != -1)
	{

		LOG_PRINT("New Client connect to %s\n", server_details->name);

		newSocket->connected = true;

		if (newSocket->server_connected_disconnected_handler != NULL)
		{
			newSocket->server_connected_disconnected_handler();
		}
	}

}


int tcp_create_Server(server_details_t *server_details)
{
	int ServerFd;
	int stat, tr = 1;
	ServerFd = socket(AF_INET, SOCK_STREAM, 0);
	if (ServerFd < 0)
	{
		LOG_PRINT("ERROR opening socket");
		return -1;
	}
	// Set the socket option SO_REUSEADDR to reduce the chance of a
	// "Address Already in Use" error on the bind
	setsockopt(ServerFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
	// Set the fd to none blocking
	fcntl(ServerFd, F_SETFL, O_NONBLOCK);

	stat = bind(ServerFd, (struct sockaddr *) &server_details->serveraddr,
		sizeof(server_details->serveraddr));
	if (stat < 0)
	{
		LOG_PRINT("ERROR on binding\n");
	}else if(listen(ServerFd, 5)){
		LOG_PRINT("ERROR on Listen\n");
	}else{
		//将当前socket添加到FD集合中，并设置回调函数，记录该FD位于集合的位置
		if ((server_details->fd_index = polling_define_poll_fd(ServerFd, POLLIN | POLLRDHUP, (event_handler_cb_t)tcp_Server_handler, server_details)) != -1)
		{
		
			LOG_PRINT("Successfully Create %s \n", server_details->name);

			server_details->connected = true;

			if (server_details->server_connected_disconnected_handler != NULL)
			{
				server_details->server_connected_disconnected_handler();
			}
			return 0;
		}

		LOG_PRINT("ERROR adding poll fd");
	}
	
	close(ServerFd);
	return -1;
	
}

void tcp_recreate_Server(server_details_t *server_details)
{
	if (tcp_create_Server(server_details) == -1)
	{
		tu_set_timer(&server_details->server_reconnection_timer, SERVER_RECONNECTION_RETRY_TIME, false , (timer_handler_cb_t)tcp_recreate_Server ,server_details);
	}
}

int tcp_new_server_ap_connection(u_short port, server_incoming_data_handler_t server_incoming_data_handler, char * name, server_connected_disconnected_handler_t server_connected_disconnected_handler)
{
	if(SocketHead == NULL)
	{
		server_details_t *server_details = malloc(sizeof(server_details_t));
		bzero((char *) &server_details->serveraddr, sizeof(server_details->serveraddr));
		server_details->serveraddr.sin_port = htons(port);
		server_details->serveraddr.sin_family = AF_INET;
		server_details->serveraddr.sin_addr.s_addr = INADDR_ANY;
		server_details->server_incoming_data_handler = server_incoming_data_handler;
		server_details->server_reconnection_timer.in_use = false;
		server_details->name = name;
		server_details->server_connected_disconnected_handler = server_connected_disconnected_handler;
		server_details->connected = false;
		
		if(tcp_create_Server(server_details)==-1)
		{
			free(server_details);
			SocketHead = NULL;
		}
		
		server_details->next = NULL;
		SocketHead = server_details;
	}
	return 0;
}

