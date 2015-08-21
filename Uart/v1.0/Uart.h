#ifndef _UART_H_
#define _UART_H_

#define ttySAC0	0
#define ttySAC1	1
#define ttySAC2	2
#define ttySAC3	3
#define ttyUSB0	4

#define BAUDRATE_2400 2400
#define BAUDRATE_4800 4800
#define BAUDRATE_9600 9600
#define BAUDRATE_115200 115200
#define BAUDRATE_460800 115200

#define DATABITS_8	8
#define DATABITS_7	7

#define PARITY_N	'N'
#define PARITY_O	'O'
#define PARITY_E	'E'

#define STOPBIT_1	1
#define STOPBIT_2	1

int HW_Uart_Config (int nSpeed, int nBits, char nEvent, int nStop);
int HW_Uart_Open (const char* DevicePath) ;
#endif
