/************************************************************************************************************* 
 * 文件名: 	  		uart.c 
 * 功能:      		Linux串口操作函数
 * 作者:      		oxp_edward@163.com 
 * 创建时间:    	2014年12月31日 
 * 最后修改时间:	2014年12月31日  
 * 详细:      		无
*************************************************************************************************************/  
#include <stdio.h>         
#include <stdlib.h>        
#include <unistd.h>       
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>        
#include <termios.h>     
#include <errno.h>        
#include <strings.h>
static int SerialPortFd = 0; //保存串口的文件描述符

/*************************************************************************************************************************
*函数        :	int SerialIsValid(void)
*功能        : 检查串口文件描述符的有效性
*参数        : 无
*返回        : 0：有效
			  -1：无效
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/
static int SerialIsValid(void)
{
	//SerialPortFd 安全检查
	if(0 == SerialPortFd)
	{
		perror("SerialPortFd is invalid");
		return -1;
	}
	return 0;
}
/*************************************************************************************************************************
*函数        :	int SerialIsValid(void)
*功能        : 检查串口文件描述符的有效性
*参数        : 无
*返回        : 0：有效
			  -1：无效
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/

int SerialOpen(const char *devicePath)
{
	SerialPortFd = open(devicePath,O_RDWR | O_NOCTTY | O_NDELAY);
	if(SerialPortFd < 0)
	{
		perror(devicePath);
		printf("%s open failed\n",devicePath);
		return -1;
	}
	return 0;
}
/*************************************************************************************************************************
*函数        :	int SerialConfig(int nSpeed,int nBits,char nEvent,int nStop)
*功能        : 串口参数设置
*参数        : nSpeed:波特率
			   nBits :数据位
			   nEvent:奇偶校验位
			   nStop :停止位
*返回        : 0：设置成功
			  -1：设置失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/

int SerialConfig(int nSpeed,int nBits,char nEvent,int nStop)
{
	int rtn = 0;
	int speed = 0;
	struct termios NewConfig,OldConfig;
	
	//SerialPortFd 安全检查
	if(SerialIsValid())
		return -1;
	
	
	//1、保存原串口配置
	/*tcgetattr(serialPortFd,&options)得到与SerialFd指向对象的相关参数，
	并将它们保存于oldtio,该函数,还可以测试配置是否正确，该串口是否
	可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.*/
	rtn = tcgetattr(SerialPortFd,&OldConfig);
	if(0 != rtn)
	{
		perror("Error on tcgetattr");
		return -1;
	}
	
	bzero(&NewConfig,sizeof(NewConfig));
	
	//2、激活选项
	//CLOCAL:修改控制模式，保证程序不会占用串口
	//CREAD:修改控制模式，接收使能，使得能够从串口中读取输入数据
	NewConfig.c_cflag |= CLOCAL | CREAD;
	
	//3、设置数据位
	//在设置数据位时，需要先使用CSIZE位清空数据位设置,然后再设置相应的数据位
	NewConfig.c_cflag &= ~CSIZE;
	switch(nBits)
	{
		case 8: NewConfig.c_cflag |= CS8;break;
		case 7: NewConfig.c_cflag |= CS7;break;
		case 6: NewConfig.c_cflag |= CS6;break;
		case 5: NewConfig.c_cflag |= CS5;break;
		default:
			printf("Error on nBits\n");
			return -1;
	}
	
	//4、设置奇偶校验位
	//设置奇偶校验需要使用到termiso中的两个成员：c_cflag和c_iflag,
	//首先激活c_cflag中的校验位使能标志PARENB,这样会对输出数据产生校验位，
	//而输入数据进行校验检测。同时还要激活c_iflag中的对于输入数据的奇偶校验使能INPCK
	//c_cflag中的PARODD为1时使用奇校验，为0时使用偶校验
	switch(nEvent)
	{
		case 'o':
		case 'O':  //设置为奇校验
			NewConfig.c_cflag |=(PARODD|PARENB);
			NewConfig.c_iflag |= INPCK;
		break;
		case 'e':
		case 'E':  //设置为偶校验
			NewConfig.c_cflag |=PARENB;
			NewConfig.c_cflag &= ~PARODD;
			NewConfig.c_iflag |= INPCK;
		break;
		case 's':
		case 'S':	/*as no parity*/
			NewConfig.c_cflag &=~PARENB;
			NewConfig.c_cflag &= ~CSTOPB;
		break;
		case 'n':
		case 'N': //设置无奇偶校验位
			NewConfig.c_cflag &= ~PARENB;
			NewConfig.c_iflag &= ~INPCK;
		break;
		default:	
			printf("Error on nEvent\n");
			return -1;
	}
	
	//5、设置波特率
	switch(nSpeed)
	{
		case 2400: speed = B2400; break;
		case 4800: speed = B4800; break;
		case 9600: speed = B9600; break;
		case 19200: speed = B19200; break;
		case 38400: speed = B38400; break;
		case 115200: speed = B115200; break;
		default:
			printf("Error on nSpeed\n");
			return -1;
	}
	cfsetispeed(&NewConfig,speed);
	cfsetospeed(&NewConfig,speed);
	
	//6、设置停止位
	//设置停止位是通过激活c_cflag中的CSTOPB而实现的。若停止位为1个，这清除CSTOPB，若停止位为2个，则激活CSTOPB
	switch(nStop)
	{
		case 1: NewConfig.c_cflag &= ~CSTOPB;break;
		case 2: NewConfig.c_cflag |=  CSTOPB;break;
		default:
			printf("Error on nStop\n");
			return;
	}
	
	//7、设置等待时间和最小接收字符
	NewConfig.c_cc[VTIME] = 0;//读取一个字符等待1*(1/10)s
	NewConfig.c_cc[VMIN] = 0;//读取字符的最少个数

	
	//8、清除串口缓冲
	//在串口重新设置后，需要对当前的串口设备进行适当的处理，
	//这时可以调用tcdrain,tcflow,tcflush等函数来处理目前串口缓冲区中的数据
	//tcdrain 函数使程序阻塞，直到输出缓冲区中的数据全部发送完毕
	//tcflow  函数用于暂停或重新开始输出
	//tcflush 函数用于清空输入/输出缓冲区
	
	//如果发生数据溢出，接收数据，但是不再读取
	tcflush(SerialPortFd,TCIFLUSH);
	
	//9、激活配置
	//在串口配置完成后，需要使用tcsetattr函数激活配置。
	rtn = tcsetattr(SerialPortFd,TCSANOW,&NewConfig);
	if(0 != rtn)
	{
		perror("Error on tcsetattr");
		return -1;
	}
	
	printf("Serial Set Done\n");
	return 0;
}
/*************************************************************************************************************************
*函数        :	int SerialReceive(unsigned char *buf,int size)
*功能        :  串口接收数据
*参数        : buf: 接收缓冲区
			   size：缓冲区大小
*返回        :错误-1，返回接收数据的大小
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/

int SerialReceive(unsigned char *buf,int size)
{
	int rtn ;
	//SerialPortFd 安全检查
	if(SerialIsValid())
		return -1;
	rtn = read(SerialPortFd,buf,size);
//	printf("ReadLength = %d\n",rtn);
	return rtn;
}
/*************************************************************************************************************************
*函数        : int SerialSend(unsigned char *buf,int DataLength)
*功能        : 串口发送数据
*参数        : buf：发送缓冲区
			   DataLength：需要发送的字节数
*返回        : rtn：成功发送的字节数
			  -1：发送失败
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/

int SerialSend(unsigned char *buf,int DataLength)
{
	int rtn ;
	//SerialPortFd 安全检查
	if(SerialIsValid())
		return -1;
#if 0
	int remain = DataLength;
	int offset = 0;
	int sub = 0;
	while(remain > 0 )
	{
		sub = (remain >= 8? 8:remain);
		write(SerialPortFd,buf+offset,sub);
		tcflush(SerialPortFd,TCOFLUSH);
		remain -= 8;
		offset += 8;
	}
#else
	rtn = write(SerialPortFd,buf,DataLength);
#endif
	return rtn;	
}
/*************************************************************************************************************************
*函数        : int SerialIsValid(void)
*功能        : 关闭串口
*参数        : 无
*返回        : 0：关闭成功
*依赖        : 无
*作者        : edward
*时间        : 20141231
*最后修改时间: 20141231
*说明        : 无
*************************************************************************************************************************/
int SerialClose(void)
{
	//SerialPortFd 安全检查
	if(SerialIsValid())
			return -1;
	close(SerialPortFd);
	return 0;
}
