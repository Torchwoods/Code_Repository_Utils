/**********************************************************
*实验要求：   实现TCP协议的并发服务器，使服务器可以同时处理二个以上的
*           连接请求。
*功能描述：   根据套接字建立TCP连接的过程，创建客户端程序向服务器发送
*           连接请求，连接后向服务器发送字符串并保持连接。
*日    期：   2010-9-17
*作    者： 
**********************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "socket_client.h"
#define port 7788

void calcFcs(uint8_t *msg,int size);

/*
* 程序入口
* */
int main(int argc,char *argv[])
{
    socketClientInit("127.0.0.1:7788",NULL);
    
    uint8_t cmd[] = {
    0xfe,1,
        0x01,
        0x00,
        0x01,
        0x00
    };

    uint8_t cmd1[] = {
    0xfe,1,
        0x01,
        0x00,
        0x00,
        0x00
    };

    calcFcs(cmd,sizeof(cmd));
    calcFcs(cmd1,sizeof(cmd1));
    uint8_t flags = 0;
    while(1)
	{
        if(flags)
        {
            printf("\nLED ON\n");
	        socketClientSendData(cmd,sizeof(cmd));
        }else
        {
            printf("\nLED off\n");
            socketClientSendData(cmd1,sizeof(cmd1));
        }
        flags = !flags;
        sleep(5);
	}
    socketClientClose();
	return 0;
}

void calcFcs(uint8_t *msg,int size)
{
    uint8_t result =0;
    int idx = 1;
    int len = (size - 1);

    while( (len--) != 0  )
    {
        result ^= msg[idx++];
    }

    msg[(size-1)] = result;
}

