#ifndef ZBSOCSOCKETSERVER_H
#define ZBSOCSOCKETSERVER_H

#include "hal_types.h"
/*
 * serverSocketInit - initialises the server.
 */
int32 socketSeverInit(void);
   
#define MAX_CLIENTS 50


/*
 * getClientFds -  get clients fd's.
 */
void socketSeverGetClientFds(int *fds, int maxFds);

/* 
 * getClientFds - get clients fd's. 
 */
uint32 socketSeverGetNumClients(void);

/*
 * socketSeverPoll - services the Socket events.
 */
void socketSeverPoll(int clinetFd, int revent);

/*
 * socketSeverSendAllclients - Send a buffer to all clients.
 */
int32 socketSeverSendAllclients(uint8* buf, uint32 len);

/*
 * socketSeverSend - Send a buffer to a clients.
 */
int32 socketSeverSend(uint8* buf, uint32 len, int32 fdClient);

/*
 * socketSeverClose - Closes the client connections.
 */
void socketSeverClose(void);
#endif