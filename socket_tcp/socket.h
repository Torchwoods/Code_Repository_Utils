#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#define DEBUG
#ifdef 	DEBUG
#define A_OUT printf("%s:%s:%d:", __FILE__, __FUNCTION__,__LINE__);fflush(stdout);
#define Log(fmt,args...) A_OUT printf(fmt, ##args)
#else
#define Log(fmt,args...) printf(fmt, ##args)
#endif

typedef struct {
	void *next;
	int socketFd;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
} socketRecord_t;

#endif