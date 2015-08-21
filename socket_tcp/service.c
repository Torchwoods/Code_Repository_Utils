/************************************************************************************************************* 
 * �ļ���: 	  		service.c 
 * ����:      		����TCP����socket����˳���
 * ����:      		oxp_edward@163.com 
 * ����ʱ��:    	2014��12��31��9:45 
 * ����޸�ʱ��:	2012��3��12�� 
 * ��ϸ:      		���ݵ�socket����TCP���ӹ��̣������������
					���ڷ���˵ȴ����պʹ���ͻ��˵����ݣ�
					�������������ն���
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
	
	//1����������ʼ��
	socketFd  = socketServiceInit(atoi(argv[1]));
	if(0 > socketFd)
	{
		Log("Error socketServiceInit\n");
		return -1;
	}
	
	
	do{
		//~���ں�����MAXFDS+1��С�Ĵ洢fd�Ŀռ�
		epfd = epoll_create(MAXFDS+1);
		if(0 > epfd)
		{
			Log("Error epoll_create\n"); 
			break;
		}
		//~������Ҫ������fd�ʹ����¼�����
		ev.data.fd = socketFd;
		/*
		EPOLLIN ����ʾ��Ӧ���ļ����������Զ��� 
		EPOLLOUT����ʾ��Ӧ���ļ�����������д�� 
		EPOLLPRI����ʾ��Ӧ���ļ��������н��������ݿɶ� 
		EPOLLERR����ʾ��Ӧ���ļ��������������� 
		EPOLLHUP����ʾ��Ӧ���ļ����������Ҷϣ� 
		EPOLLET����ʾ��Ӧ���ļ����������¼������� 
		*/
		ev.events  =  EPOLLIN | EPOLLET;
		//~����ĳ��epoll�ļ��������ϵ��¼�������ע���¼����޸��¼���ɾ���¼�
		rtn = epoll_ctl(epfd,EPOLL_CTL_ADD,socketFd,&ev);
		/*
		������ 
		epfd���� epoll_create ���ɵ�epollר�õ��ļ��������� 
		op��Ҫ���еĲ�������ע���¼������ܵ�ȡֵ
			EPOLL_CTL_ADD ע�ᡢ
			EPOLL_CTL_MOD �� �ġ�
			EPOLL_CTL_DEL ɾ�� 

		fd���������ļ��������� 
		event��ָ��epoll_event��ָ�룻 
			������óɹ�����0,���ɹ�����-1 
		*/
		if(0 > rtn)
		{
			Log("Error epoll_ctl\n"); 
			break;
		}
		
		for(;;)
		{	//~��ѯI/O�¼��ķ���
			nfds = epoll_wait(epfd,events,MAXFDS,0);
			/*
			epfd:��epoll_create ���ɵ�epollר�õ��ļ��������� 
			epoll_event:���ڻش��������¼������飻 
			maxevents:ÿ���ܴ�����¼����� 
			timeout:�ȴ�I/O�¼������ĳ�ʱֵ��
				-1�൱��������
				0�൱�ڷ�������
				һ����-1���ɷ��ط����¼����� 
			*/
			for(Count = 0;Count<nfds;++Count)
			{	
				//�����µ���������
				if(socketFd == events[Count].data.fd)
				{
					//2�����տͻ��˵���������
					NewsocketFd = createSocketRec();
					if(0 > NewsocketFd)
					{
						continue;
					}
					//���ͻ��˵�FD����epoll��
					ev.data.fd = NewsocketFd;
					ev.events = EPOLLIN | EPOLLET;
					epoll_ctl(epfd,EPOLL_CTL_ADD,NewsocketFd,&ev);
				//���յ����ݣ���socket 	
				}else if(events[Count].events & EPOLLIN){
					//Log("read");
					if(socketFd == events[Count].data.fd) continue;
					bzero(buf,sizeof(buf));
					//3�����տͻ�������
					rtn = SocketRecv(events[Count].data.fd,buf,sizeof(buf));
					if(0 == rtn)
					{
						Log("Connect Close \n");
						epoll_ctl(epfd, EPOLL_CTL_DEL, events[Count].data.fd, NULL);
						deleteSocketRec(events[Count].data.fd);
						continue;
					}
					Log("[Recv]: %s",buf);
					//4���������ݵ��ͻ���
					socketSeverSendAllclients(buf,sizeof(buf));
				//�����ݴ����ͣ�дsocket 
				}else if(events[Count].events & EPOLLOUT){
					//Log("write\n");
					strcpy(buf,"data from service");
					socketSeverSendAllclients(buf,sizeof(buf));
				}	
			}
		}
		
	}while(0);
	//5���ر����е����ӣ��Ͽ�TCP����
	socketSeverClose();
	return 0;
}





