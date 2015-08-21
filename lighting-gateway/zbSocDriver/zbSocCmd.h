#ifndef _ZBSOCCMD_H_
#define _ZBSOCCMD_H_
#include<inttypes.h>

#define DEBUG
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

#define MT_RPC_SOF	0xFE

//设备控制定义
typedef enum{
	DEVICEOFF 	= 0x0, //设备开启
	DEVICEON 	= 0x01//设备关闭
}mtRpcDeviceTyep_t;

//定义设备现有的功能
 typedef enum{
     MT_RPC_CMD_SET_LED     = 0x01,//LED灯控制
	 MT_RPC_CMD_SET_BFF     = 0x02,//蜂鸣器控制
     MT_RPC_CMD_GET_TEMP    = 0x03,//获取温度
     MT_RPC_CMD_GET_WARING  = 0X04,//烟雾报警器状态
     MT_RPC_CMD_GET_HUM     = 0x05,//获取湿度
     MT_RPC_CMD_GET_WSPEED  = 0x06,//获取风速
     MT_RPC_CMD_GET_WDIRC   = 0x07,//获取风向
     MT_RPC_CMD_GET_LIGHTSENSER = 0x08//获取光强度
 }mtRpcCmdType_t;

int32_t zbSocOpen(const char *devicePath);
int zbSocUartConfig (int nSpeed, int nBits, char nEvent, int nStop) ;
void zbSocClose(void);
void calcFcs(uint8_t *msg,int size);
void zbSocSetDevice(uint8_t option,uint8_t state);
void zbSocProcessRpc(void);
void zbSocGetDeviceData(uint8_t option);

#endif
