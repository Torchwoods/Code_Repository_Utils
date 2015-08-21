/*
*Uart.c
*串口初始化
*/

#include "Uart.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include<linux/types.h>
#include<sys/time.h>

/*
	宏定义
*/
#ifdef DEBUG
	#define A_OUT printf("%s:%s:%d:", __FILE__, __FUNCTION__,__LINE__);fflush(stdout);
	#define Printf(fmt, arg...) A_OUT printf(fmt, ##arg)
#else
	#define A_OUT
	#define Printf(fmt, arg...)
#endif

/*
	全局变量
*/
int SerialFd;//保存串口文件描述符


/*
******************************************************************************************
** 函数名称 ： int HW_Uart_Config (int nSpeed, int nBits, char nEvent, int nStop) 
** 函数功能 ： 设置串口参数                        
** 入口参数 ： 
**			   nSpeed:波特率
**			   nBits:数据位
**			   nEvent：奇偶校验位
**			   nStop:停止位			   
** 出口参数 ： -1 设置失败
**				0 设置成功
** 函数说明 ： 设置串口详细参数
******************************************************************************************
*/
int HW_Uart_Config (int nSpeed, int nBits, char nEvent, int nStop) 
{ 
	struct termios newtio, oldtio; 
	/*tcgetattr(SerialFd,&options)得到与SerialFd指向对象的相关参数，
	并将它们保存于oldtio,该函数,还可以测试配置是否正确，该串口是否
	可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.*/
	if (tcgetattr (SerialFd, &oldtio) != 0)
    {      
		perror ("SetupSerial 1");      
		return -1;   
	}
  
	bzero (&newtio, sizeof (newtio)); 
	//CLOCAL:修改控制模式，保证程序不会占用串口
	//CREAD:修改控制模式，使得能够从串口中读取输入数据
	newtio.c_cflag |= CLOCAL | CREAD; 
	
	//设置数据位
	newtio.c_cflag &= ~CSIZE; //屏蔽其他标志位
	switch (nBits)   
    {  
	case 7:   
		newtio.c_cflag |= CS7;   
		break;  
	case 8:   
		newtio.c_cflag |= CS8;   
		break; 
	}
	
	//设置奇偶校验
	switch (nEvent)   
    {
	case 'o':
	case 'O':   //设置为奇校验   
		newtio.c_cflag |= PARENB;    
		newtio.c_cflag |= PARODD;    
		newtio.c_iflag |= (INPCK | ISTRIP);    
	break;  
	case 'e':
	case 'E':    //设置为偶校验
		newtio.c_iflag |= (INPCK | ISTRIP);			  
		newtio.c_cflag |= PARENB;			  
		newtio.c_cflag &= ~PARODD;			  
		break;  
	case 'n':	//无奇偶校验位。	
	case 'N':      
		newtio.c_iflag &= ~INPCK ;//导致数据收发错误是由于这里设置错误， 将c_iflag看出C_cflag	
		newtio.c_cflag &= ~PARENB;		
	//	newtio.c_cflag &= ~INPCK;//导致收发数据错误
		break; 
	case 'S':  
	case 's':           /*as no parity */  
		newtio.c_cflag &= ~PARENB;  
		newtio.c_cflag &= ~CSTOPB;  
	break;  		
	default :
		return -1;
	}
  
	//设置串口输入波特率和输出波特率
	switch (nSpeed)    
    {    
	case 2400:      
		cfsetispeed (&newtio, B2400);			  
		cfsetospeed (&newtio, B2400);			  
		break;    
	case 4800:      
		cfsetispeed (&newtio, B4800);			  
		cfsetospeed (&newtio, B4800);			  
		break;   
	case 9600:     
		cfsetispeed (&newtio, B9600);			  
		cfsetospeed (&newtio, B9600);			  
		break;   
	case 115200:      
		cfsetispeed (&newtio, B115200);
		cfsetospeed (&newtio, B115200);	  
		break;  
	case 460800:   
		cfsetispeed (&newtio, B460800);			  
		cfsetospeed (&newtio, B460800);			  
		break;    
	default:     
		return -1;  
	}
      // 设置停止位 
	switch(nStop)
	{
		case 1:
			newtio.c_cflag &= ~CSTOPB; 
		break;
		case 2:
			newtio.c_cflag |= CSTOPB; 
		break;
		default:
			return -1;
	}
	
	//设置等待时间和最小接收字符
	newtio.c_cc[VTIME] = 0;	//读取一个字符等待1*(1/10)s 
	newtio.c_cc[VMIN] = 0;	//读取字符的最少个数
	//如果不是开发终端之类的，只是串口传输数据，
	//而不需要串口来处理，那么使用原始模式(Raw Mode)方式来通讯
	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	newtio.c_oflag  &= ~OPOST;   /*Output*/
	
	//如果发生数据溢出，接收数据，但是不再读取
	tcflush (SerialFd, TCIFLUSH);
	//激活配置 (将修改后的termios数据设置到串口中）
	if ((tcsetattr (SerialFd, TCSANOW, &newtio)) != 0)   
    {     
		perror ("com set error");			  
		return -1;
	} 
	Printf("set done!\n"); 
	return 0;
}

/*
******************************************************************************************
** 函数名称 ： int HW_Uart_Open (const char* DevicePath)  
** 函数功能 ： 打开串口设备                          
** 入口参数 ： DevicePath: 打开串口的绝对路径
** 出口参数 ： -1 打开失败
**				 打开成功 返回描述符
** 函数说明 ： 以非阻塞的方式打开串口设备
******************************************************************************************
*/
int HW_Uart_Open (const char* DevicePath) 
{
	
		SerialFd = open( DevicePath, O_RDWR | O_NOCTTY | O_NDELAY );      
		if (-1 == SerialFd)
		{	  
			perror ("Can't Open Serial Port /dev/ttySAC0");				  
			return (-1);	
		} 
		
#if 0
    if ( fcntl (SerialFd, F_SETFL, 0) < 0 )  
		Printf ("fcntl failed!\n");
	else 
		Printf ("fcntl=%d\n", fcntl (SerialFd, F_SETFL, 0));
	if ( 0 == isatty (STDIN_FILENO) ) 
		Printf ("standard input is not a terminal device\n");
	else
		Printf ("isatty success!\n");	  
	Printf ("SerialFd-open=%d\n", SerialFd);
  
#endif	
    return SerialFd;
}

/*
******************************************************************************************
** 函数名称 ： int HW_Uart_Close (int SerialFd) 
** 函数功能 ： 关闭串口设备                          
** 入口参数 ： SerialFd: 文件描述符
** 出口参数 ： void
** 函数说明 ： 无
******************************************************************************************
*/

void HW_Uart_Close(int SerialFd)
{
	close(SerialFd);
}

