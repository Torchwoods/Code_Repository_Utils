#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include<pthread.h>
#include<wait.h>
#include<limits.h>
#include<linux/types.h>
#include<sys/time.h>
#include<signal.h>
#include <poll.h>
//#include<inttypes.h>
#include "zbSocSocketServer.h"
#include "zbSocCmd.h" 
#define DEVICE "/dev/ttyUSB0"
void WriteCOM(void);
int main(int argc,char**argv)
{
	int zbSoc_fd;
	
    zbSoc_fd = zbSocOpen(DEVICE);
    if(zbSoc_fd == -1)
    {
        exit(-1);
    }
	
	if(0 != socketSeverInit())
	{
		return -1;
	}

    while(1)
    {
		//返回socket连接个数
		int numClientFds = socketSeverGetNumClients();

		//poll on client socket fd's and the ZllSoC serial port for any activity
		if (numClientFds)
		{
			int pollFdIdx;
			//client_fds用于存储socket 连接描述符
			int *client_fds = malloc(numClientFds * sizeof(int));
			//socket client FD's + zllSoC serial port FD
			struct pollfd *pollFds = malloc(
					((numClientFds + 1) * sizeof(struct pollfd)));

			if (client_fds && pollFds)
			{
				//set the zllSoC serial port FD in the poll file descriptors
				pollFds[0].fd = zbSoc_fd;
				pollFds[0].events = POLLIN;//可读

				//Set the socket file descriptors
				socketSeverGetClientFds(client_fds, numClientFds);
				for (pollFdIdx = 0; pollFdIdx < numClientFds; pollFdIdx++)
				{
					pollFds[pollFdIdx + 1].fd = client_fds[pollFdIdx];
					pollFds[pollFdIdx + 1].events = POLLIN | POLLRDHUP; //可读和套接字半关闭
					//printf("zllMain: adding fd %d to poll()\n", pollFds[pollFdIdx].fd);
				}

				printf("zllMain: waiting for poll()\n");

				poll(pollFds, (numClientFds + 1), -1);

				printf("zllMain: got poll()\n");

				//did the poll unblock because of the zllSoC serial?
				if (pollFds[0].revents)
				{
					printf("Message from ZLL SoC\n");
					usleep(5000);
					//串口处理函数
					zbSocProcessRpc();
				}
				//did the poll unblock because of activity on the socket interface?
				for (pollFdIdx = 1; pollFdIdx < (numClientFds + 1); pollFdIdx++)
				{
					if ((pollFds[pollFdIdx].revents))
					{
						printf("Message from Socket Sever\n");
						//socket处理函数
						socketSeverPoll(pollFds[pollFdIdx].fd, pollFds[pollFdIdx].revents);
					}
				}

				free(client_fds);
				free(pollFds);
			}
		}
    }
	return 0;
}

