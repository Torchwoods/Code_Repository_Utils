/**********************************************************
*ʵ��Ҫ��   ʵ��TCPЭ��Ĳ�����������ʹ����������ͬʱ����������ϵ�
*           ��������
*����������   �����׽��ֽ���TCP���ӵĹ��̣������ͻ��˳��������������
*           �����������Ӻ�������������ַ������������ӡ�
*��    �ڣ�   2010-9-17
*��    �ߣ� 
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
* �������
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

