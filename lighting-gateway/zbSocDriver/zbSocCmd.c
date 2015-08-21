/*zbSocCmd.c
 * 
 *
 */
#include "zbSocCmd.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include<limits.h>
#include<linux/types.h>
#include<sys/time.h>
#include<signal.h>
//#include<inttypes.h>

#include "zbSocSocketServer.h"
/**
 * ZIGBEE COMMOND
 */


/*
* LOCAL VARIABLES
*/

int serialPortFd = 0;//保存串口文件描述符

/************************************************************
** 函数名称 ： int zbSocOpen (const char*devicePath) 
** 函数功能 ： 打开串口设备                          
** 入口参数 ： devicePath：需要打开那个设备
** 出口参数 ： -1 打开失败
**				0 打开成功
** 函数说明 ： 以非阻塞的方式打开串口设备
*************************************************************
*/
int32_t zbSocOpen(const char *devcePath)
{
    serialPortFd = open(devcePath,O_RDWR|O_NOCTTY|O_NDELAY);
    if( serialPortFd < 0 )
    {
        perror(devcePath);
        printf("%s open failed\n",devcePath);
        return (-1);
    }

    if(zbSocUartConfig(115200,8,'N',1)<0)
        return -1;
    return serialPortFd;
}
/*
******************************************************************************************
** 函数名称 ： void zbSocClose(void)
** 函数功能 ： 关闭串口                       
** 入口参数 ： void		   
** 出口参数 ： void
** 函数说明 ： 关闭串口并将串口中的数据清空
******************************************************************************************
*/
void zbSocClose(void)
{
    tcflush(serialPortFd,TCOFLUSH);
    close(serialPortFd);
    return;
}

/*
******************************************************************************************
** 函数名称 ： void calcFcs(unsigned char *msg,int size)
** 函数功能 ： FCS校验                        
** 入口参数 ： msg：需要校验数据的数组
**			   size: 数据大小		   
** 出口参数 ： void
** 函数说明 ： 生成FCS校验数据并填入msg最后位置
******************************************************************************************
*/
void calcFcs(unsigned char *msg,int size)
{
    uint8_t result =0;
    int idx = 1;
    int len = (size -1);
    while((len--) != 0)
    {
        result ^=msg[idx++];
    }

    msg[(size -1)] = result;
}

/*
******************************************************************************************
** 函数名称 ： static void zbSocTransportWrite(unsigned char *buf,int len)
** 函数功能 ： 将buf中的len大小的数据写入到串口                        
** 入口参数 ： buf：存储数据缓存区
**			   len: 数据长度
** 出口参数 ： void
** 函数说明 ： 无
******************************************************************************************
*/
static void zbSocTransportWrite(unsigned char *buf,int len)
{
    int remain = len;
    int offset = 0;
#if 1
    while(remain > 0)
    {
        int sub = (remain >= 8 ? 8:remain);
        write(serialPortFd,buf+offset,sub);

        tcflush(serialPortFd,TCOFLUSH);
        usleep(5000);
        remain -= 8;
        offset += 8;
    }
#else
    write(serialPortFd,buf,len);
 //   tcflush(serialPortFd,TCIFLUSH);
#endif

    return;
}
/*
******************************************************************************************
** 函数名称 ： void zbSocSetDevice(uint8_t option,uint8_t state)
** 函数功能 ： 控制设备                       
** 入口参数 ： option:命令名称
**			   state:开启和关闭状态		   
** 出口参数 ： void
** 函数说明 ： 设置LED或蜂鸣器状态
******************************************************************************************
*/
void zbSocSetDevice(uint8_t option,uint8_t state)
{
    unsigned char cmd[] = {
        0xfe,1,//数据长度
        option,//控制命令CMD0
        0x00,//控制命令CMD1
		state,//参数
        0x00//FCS校验
    };
	
   	calcFcs(cmd,sizeof(cmd));
    zbSocTransportWrite(cmd,sizeof(cmd));
}
/*
******************************************************************************************
** 函数名称 ： void zbSocGetDeviceData(uint8_t option)
** 函数功能 ： 发送获取数据命令                        
** 入口参数 ： option:GET控制命令		   
** 出口参数 ： void
** 函数说明 ： 用于发送GET命令给zigbee获取传感器数据
******************************************************************************************
*/
void zbSocGetDeviceData(uint8_t option)
{
	    unsigned char cmd[] = {
        0xfe,0,//数据长度
        option,//控制命令CMD0
        0x00,//控制命令CMD1
        0x00//FCS校验
    };
	
   	calcFcs(cmd,sizeof(cmd));
    zbSocTransportWrite(cmd,sizeof(cmd));
}
/*
******************************************************************************************
** 函数名称 ： void zbSocProcessRpc(void)
** 函数功能 ： 处理串口返回的数据                        
** 入口参数 ： void		   
** 出口参数 ： void
** 函数说明 ： 
******************************************************************************************
*/
void zbSocProcessRpc(void)
{
	uint8_t rpcLen,bytesRead,sofByte,*rpcBuff=NULL,rpcBuffIndex,Len;
	static uint8_t retryAttempts = 0;
	int rtn = 0;
	//读取 SOF头标示0xff
	read(serialPortFd,&sofByte,1);
    printf("sof:%x\n",sofByte);
	if(sofByte == MT_RPC_SOF)
	{
		retryAttempts = 0;;
		
		//读取Data域的长度
		bytesRead = read(serialPortFd,&rpcLen,1);
		Len = rpcLen;
		printf("bytesRead = %d\n",bytesRead);
		if(bytesRead == 1)
		{
			//数据域的长度+cmd0+cmd1+FCS校验位
			rpcLen += 3;
			rpcBuff = (uint8_t*)malloc(rpcLen+2);
			if(rpcBuff!=NULL)
			{
				rpcBuffIndex = 2;
				rpcBuff[0] = MT_RPC_SOF;
				rpcBuff[1] = Len;
				//非阻塞读取，所以需要等待串口有数据后在读取
				while(rpcLen > 0)
				{
					//读取数据
					bytesRead = read(serialPortFd,&(rpcBuff[rpcBuffIndex]),rpcLen);
					
					//检测错误
					if(bytesRead > rpcLen)
					{
						printf("zbSocProcessRpc: read of %d bytes failed - %s\n", rpcLen,
								strerror(errno));
						
						if(retryAttempts++ < 5)
						{
							//睡眠10ms
							usleep(10000);
							//再次读取
							bytesRead = 0;
						}else{
							//something went wrong.
							printf("zbSocProcessRpc: failed\n");
							free(rpcBuff);
							return;
						}
					}
					rpcLen -= bytesRead;
					rpcBuffIndex +=bytesRead;
				}//end while
				//将数据写入到所有的socket
				rtn = socketSeverSendAllclients(rpcBuff,6);
				if(rtn < 0)
				{
					printf("ERROR writing to socket\n");
				}
				
				for(Len=0;Len<(rpcBuff[2]+5);Len++)
				{
					printf("%x ",rpcBuff[Len]);
				}
				printf("\n");
				
				free(rpcBuff);
			}
		}else{
			printf("zbSocProcessRpc: No valid Start Of Frame found\n");
		}
	}
	return;
}


/*
******************************************************************************************
** 函数名称 ： int zbSocUartConfig (int nSpeed, int nBits, char nEvent, int nStop) 
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
int zbSocUartConfig (int nSpeed, int nBits, char nEvent, int nStop) 
{ 
	struct termios newtio, oldtio; 
	/*tcgetattr(serialPortFd,&options)得到与SerialFd指向对象的相关参数，
	并将它们保存于oldtio,该函数,还可以测试配置是否正确，该串口是否
	可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1.*/
	if (tcgetattr (serialPortFd, &oldtio) != 0)
    {      
		perror ("SetupSerial 1");      
		return -1;   
	}
  
	bzero (&newtio, sizeof (newtio)); 
	//CLOCAL:修改控制模式，保证程序不会占用串口
	//CREAD:修改控制模式，使得能够从串口中读取输入数据
	newtio.c_cflag = CLOCAL | CREAD ;//; 
	
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
       // newtio.c_iflag = IGNPAR & ~ICRNL;
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
//	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
//	newtio.c_oflag  &= ~OPOST;   /*Output*/
	
    newtio.c_lflag = 0;
    newtio.c_oflag = 0;

	//如果发生数据溢出，接收数据，但是不再读取
	tcflush (serialPortFd, TCIFLUSH);
	//激活配置 (将修改后的termios数据设置到串口中）
	if ((tcsetattr (serialPortFd, TCSANOW, &newtio)) != 0)   
    {     
		perror ("com set error");			  
		return -1;
	} 
	Printf("set done!\n"); 
	return 0;
}


