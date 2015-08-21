#ifndef __UART_H__
#define __UART_H__
int SerialOpen(const char *devicePath);
int SerialConfig(int nSpeed,int nBits,char nEvent,int nStop);
int SerialReceive(unsigned char *buf,int size);
int SerialSend(unsigned char *buf,int DataLength);
int SerialClose(void);
#endif