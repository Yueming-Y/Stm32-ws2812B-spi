#ifndef __WS2812B__H__
#define __WS2812B__H__

#include "stm32f0xx_hal.h"

#define LED_NUM 8           //灯个数

#define WRITE_LOW  0xC0    //低电平 1100 0000  
#define WRITE_HIGH   0xF8    // 高电平1111 1000 



//协议相关长度
//协议相关长度
#define HEAD_LENGTH_ws 0x03
#define GROUPMGR_ADDR_LENGTH 0x02
#define DEVICE_ADDR_LENGTH  0x02
#define PORT_LENGTH 0x01
#define FUNCODE_LENGTH 0x01
#define LED_TYPE_LENGTH 0x01
#define RETAIN_LENTH 0x02
#define DATA_LENGTH 0x02
#define EXTEND_LENGTH 0x02



//根据协议解析串口指定
#define HEAD_1 0xdd                       //协议头
#define HEAD_2 0x55                       //协议头
#define HEAD_3 0xEE                       //协议头
#define FUNCODE_QUERY_UUID 0x8F           //功能码 查询控制器唯一序列号
#define FUNCODE_UUIDQUERY_ADDR 0x90       //功能码 通过控制器唯一序列号查询控制器设备地址
#define FUNCODE_QUERY_MODIFY_ADDR 0x91    //功能码 通过控制器唯一序列号修改控制器设备地址
#define FUNCODE_QUERY_MATCH 0x92          //功能码 查询总线上有没有与指令中组地址和设备地址匹配的控制器
#define FUNCODE_QUERY_GROUP_ADDR 0x93     //功能码 查询控制器组地址
#define FUNCODE_QUERY_ADDR 0x94           //功能码 查询控制器设备地址
#define FUNCODE_MODIFY_RATE 0x95          //功能码 修改控制器通信波特率
#define FUNCODE_MODIFY_GROUP_ADDR 0x96    //功能码 修改控制器组地址
#define FUNCODE_MODIFY_ADDR 0x97          //功能码 修改控制器设备地址
#define EXTEND 0x98                       //功能码 扩展
#define FUNCODE_SHOW_COLOR 0x99           //功能码 显示颜色
#define FUNCODE_POWERON 0x9B              //功能码 设置上电显示的数据
#define FUNCODE_CLOSE_POWERON 0x9C        //功能码 关闭上电显示功能

#define LIGHT_TPYE  0x01                  //灯带类型

#define END_1 0xAA                        //指令尾
#define END_2 0xBB                        //指令尾


#define LED_TYPE 0x01


//extern TIM_HandleTypeDef g_htim2;

extern uint8_t breathFlag;

typedef struct RGB_color {  

    uint8_t R;
    uint8_t G;
    uint8_t B;

}Rgb_color,*PRgb_color; 

// typedef struct ProtocolInfo {  

//     uint16_t g_groupmgr_add ;
//     uint16_t g_device_addr ;

// }protocolInfo,*PProtocolInfo; 




//通过串口解析pwm,路数,频率等功能相关
//*****************************************
#define HEAD_LENGTH  2 //头长度
#define NUM_LENGTH   2 //路数长度
#define PWM_LENGTH   2 //PWM值长度
#define RATE_LENGTH  4 //波特率长度



//*****************************************


/* Description:
 *				创建ws2812b识别的高低电平数据
 * Parameters:
 * 				index 第几颗灯, reg r,g,b值 
 * Return:
 * 				
 * Notes:
 *
 */
void creatColorData(uint8_t index,Rgb_color reg,uint8_t *colorSendBuff);

/* Description:
 *				初始化数据空间
 * Parameters:
 * 				index 第几颗灯, reg r,g,b值 
 * Return:
 * 				-
 * Notes:
 *
 */
void init_Config();  
void ws2812b_init(void);  
//将灯初始化白色

void getpwmValue(uint8_t *receivebuff);//获取pwm值 5A A5 02 00 01 02 00 64 02 4E 02 AA BB
void ws2812b_operation(uint8_t *receivebuff,uint8_t Size);              //接收ws2812B数据 处理  DD 55 EE 11 64 00 00 00 99 01 00 00 00 06 00 0F 00 00 FF 00 00 FF AA BB

void ws2812b_send(uint8_t *sendBuff,uint8_t *lednum);
void sendWaringMessage(uint8_t index); //发送错误信息
void WS2812B_BreathEffect();  //呼吸灯显示
void WS2812B_Close(); //呼吸灯关闭

void breatheLED();
#endif
