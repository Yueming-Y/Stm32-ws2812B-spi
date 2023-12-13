#include "ws2812B.h"
#include "dma.h"
#include "spi.h"
//#include "tim.h"
#include "usart.h"
#include "gpio.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define LED_COUNT 24

uint8_t ledColors[LED_COUNT * 24];  // RGB数据，每个LED占3个字节  24个高低电屏
uint8_t *g_sendBuff = NULL;
uint8_t g_recUARTbuff[255] = {0};
//定义错误信息
const uint8_t message[10][10] = {
	{"HERR\r\n"},   //指令头错误
	{"GERR\r\n"},   //组地址错误（超出最大值 1024）
	{"AERR\r\n"},   //设备地址错误（超出最大值 4096）
	{"PERR\r\n"},   //端口错误（超出最大值 30）
	{"CERR\r\n"},   //功能码错误
	{"IERR\r\n"},   //灯带类型错误
	{"LERR\r\n"},   //数据长度错误
	{"RERR\r\n"},   //扩展次数错误
	{"TERR\r\n"},   //指令尾部错误
	{"DERR\r\n"},   //数据长度与颜色数据字节数不符
};


extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

extern TIM_HandleTypeDef htim3;

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

uint8_t breathFlag = 0;

//获取pwm值函数
void getpwmValue(uint8_t *receivebuff) {

	uint16_t wayNum = 0;
	uint16_t pwmValue = 0;
	uint16_t frequency = 0;
  uint8_t length = 0;
  if(receivebuff[0] == 0x5A && receivebuff[1] == 0xA5 && receivebuff[11] == 0xAA && receivebuff[12] == 0xBB) {
    if(receivebuff[2] == 0x02) 
      //获取路数值
      length += HEAD_LENGTH + 1;  //3
      wayNum = (uint16_t)receivebuff[length]<<8 | receivebuff[length+1];
      if(receivebuff[5] == 0x02) {
				pwmValue = 0;
        //获取pwm值
        length += NUM_LENGTH + 1;  //6
        pwmValue =(uint16_t)receivebuff[length] <<8 | receivebuff[length+1];

        //在这发你想发的pwm值
				__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_1,pwmValue);//设置PWM占空比值
				
				//串口回显上位机 测试显示
				uint8_t temp[10] = {0};
				temp[0] = receivebuff[length];
				temp[1] = receivebuff[length+1];
				HAL_UART_Transmit(&huart1,temp,4,0xFFFF); 
        if(receivebuff[8] == 0x02) {
          //获取频率值
          length += PWM_LENGTH + 1;  //9     
          frequency = (uint16_t)receivebuff[length] <<8 | receivebuff[length+1];
        }
      }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)	{

	if(huart == &huart1) {
		if(g_recUARTbuff[0] == 0x5A && g_recUARTbuff[1] == 0xA5 && g_recUARTbuff[Size-1] == 0xBB && g_recUARTbuff[Size-2] == 0xAA ) { //发送pwm值
			getpwmValue(g_recUARTbuff);
		}                       
		else if (g_recUARTbuff[0] == 0xDD && g_recUARTbuff[1] == 0x55 && g_recUARTbuff[2] == 0xEE ){ //ws2812B灯 指令解析  && g_recUARTbuff[Size-1] == 0xBB && g_recUARTbuff[Size-2] == 0xAA )
			if(g_recUARTbuff[Size-1] == 0xBB && g_recUARTbuff[Size-2] == 0xAA ) {
						ws2812b_operation(g_recUARTbuff,Size);
			} else {
				sendWaringMessage(8);
			}
		}
	  else if (g_recUARTbuff[0] == 0xAA && g_recUARTbuff[1] == 0xBB && g_recUARTbuff[2] == 0xFF) {
			breathFlag = g_recUARTbuff[3]; //0x00关闭 0x01 打开 
			if(0x00 == g_recUARTbuff[3]) {
				WS2812B_Close();
			}
		}
		else {                     
			sendWaringMessage(0);   
		}
		memset(g_recUARTbuff,0,sizeof(g_recUARTbuff));												
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1,g_recUARTbuff,sizeof(g_recUARTbuff));   //暂时使用中断,后期优化为DMA，节省cpu占用
		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx,DMA_IT_HT);

	}
				

}

void WS2812B_SetColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
		uint8_t buffer[24] = {0};
		
    for(uint8_t  i = 0 ; i < 8 ; i++ ) {
        buffer[7-i] = g & 0x01 ? WRITE_HIGH : WRITE_LOW;
        g = g >> 1 ;
    }
    for(uint8_t i = 0 ; i < 8 ; i++ ) {
        buffer[15-i] = r & 0x01 ? WRITE_HIGH : WRITE_LOW;
        r = r >> 1 ;
    }
    for(uint8_t i = 0 ; i < 8 ; i++ ) {
        buffer[23-i] = b & 0x01 ? WRITE_HIGH : WRITE_LOW;
        b = b >> 1 ;
    }
		//HAL_UART_Transmit_It(&huart1, buffer, 10);
    memcpy(ledColors+(index*24),buffer,sizeof(buffer)); 	
    return;
}


void WS2812B_Show() {
		//HAL_UART_Transmit(&huart1,ledColors,);
		HAL_SPI_Transmit_DMA(&hspi1,ledColors,LED_COUNT * 24);
}

void WS2812B_Close() {
	for (int i = 0; i < LED_COUNT; ++i) {
		uint8_t r = 0;
		uint8_t g = 0;
		uint8_t b = 0;
		WS2812B_SetColor(i, r, g, b);
	}
	WS2812B_Show();
}


// 定义呼吸灯的周期和步数
const double breathPeriod = 2.0; // 呼吸灯周期，单位秒
const int breathSteps = 100;     // 呼吸灯步数

// 计算呼吸灯的亮度值
float calculateBreathBrightness(int step) {
    // 使用正弦函数模拟呼吸灯效果 
    double angle = (double)(step) / breathSteps * breathPeriod * 3.1415926;
    double brightness = 0.5 * (1 + sinf(angle));
    
    return brightness;
}



void breatheLED() {
    for (int step = 0; step < breathSteps; ++step) {
        // 计算当前步数下的亮度值
        float brightness = calculateBreathBrightness(step);

        for (int i = 0; i < LED_COUNT; ++i) {
            uint8_t r =(uint8_t)(brightness * 0xF0);  //F0
            uint8_t g =  (uint8_t)(brightness * 0x28);   //28
            uint8_t b = 0;
            WS2812B_SetColor(i, r, g, b);
        }
				if(0 == breathFlag)  //关闭呼吸灯
				{
					break;  //退出呼吸
				}
        WS2812B_Show();
        // 休眠一小段时间，控制呼吸速度
        // 可以根据需要调整休眠时间
        HAL_Delay(30);
    }
}

void creatColorData(uint8_t index,Rgb_color reg,uint8_t *colorSendBuff) {
    uint8_t buffer[24] = {0};
		
    for(uint8_t  i = 0 ; i < 8 ; i++ ) {
        buffer[7-i] = reg.G & 0x01 ? WRITE_HIGH : WRITE_LOW;
        reg.G = reg.G >> 1 ;
    }
    for(uint8_t i = 0 ; i < 8 ; i++ ) {
        buffer[15-i] = reg.R & 0x01 ? WRITE_HIGH : WRITE_LOW;
        reg.R = reg.R >> 1 ;
    }
    for(uint8_t i = 0 ; i < 8 ; i++ ) {
        buffer[23-i] = reg.B & 0x01 ? WRITE_HIGH : WRITE_LOW;
        reg.B = reg.B >> 1 ;
    }
		//HAL_UART_Transmit_It(&huart1, buffer, 10);
    memcpy(colorSendBuff+(index*24),buffer,sizeof(buffer)); 	
    return;
}

void init_Config() {
  HAL_Delay(100);
	//设置time 
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);
  ws2812b_init();  //初始化8个灯为白色显示
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1,g_recUARTbuff,sizeof(g_recUARTbuff));//使用空闲中断函数,接收完成发送的数据之后,会调用HAL_UARTEx_RxEventCallback 回调函数
	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx,DMA_IT_HT);
}

int init_sendbuff(uint8_t lednum) {
	g_sendBuff = (uint8_t*)malloc(sizeof(uint8_t)*30*24);
	if(g_sendBuff == NULL) {
		return 0;
	}

	memset(g_sendBuff,0x01,lednum*24);
	return 1;
}

void ws2812b_init() {
	uint8_t resetbuff[50];
//	memset(resetbuff,0x00,sizeof(resetbuff));
	uint8_t sendbuff[24*2];
	Rgb_color reg;
	reg.R = 0x00;
	reg.G = 0x00;
	reg.B = 0x00;
	for(int i = 0 ;i<2;i++) {
		creatColorData(i,reg,sendbuff);
	}
	HAL_SPI_Transmit_DMA(&hspi1, sendbuff,48);
	memset(resetbuff,0x00,sizeof(resetbuff));
	  

}


void ws2812b_send(uint8_t *sendBuff,uint8_t *lednum) {
	uint8_t index = 0;
	uint8_t colorSendBuff[(*lednum)*24];
	//memset(colorSendBuff,0,(*lednum)*24*sizeof(uint8_t)); 
  for(int i = 0 ;i<*lednum;i++) {
    Rgb_color reg;    
		reg.R = sendBuff[index];  //0
		index++;
		reg.G = sendBuff[index];  //1
		index++; 
		reg.B = sendBuff[index];  //2
		index++;
		creatColorData(i,reg,colorSendBuff);
	}
	HAL_UART_Transmit(&huart1,colorSendBuff,sizeof(colorSendBuff),0xffff);
	HAL_SPI_Transmit_DMA(&hspi1,colorSendBuff,*lednum*24);
	
	//每次发送之后进行复位操作
	uint8_t resetbuff[50];
  memset(resetbuff,0x00,sizeof(resetbuff));
  HAL_SPI_Transmit_DMA(&hspi1,resetbuff,sizeof(resetbuff));
}

void sendWaringMessage(uint8_t index) {
	HAL_UART_Transmit(&huart1, (uint8_t*)message[0], strlen((const char*)message[index]), 0xffff);
}

void ws2812b_operation(uint8_t *receivebuff,uint8_t Size) {

	uint16_t extendNum = 0;
	uint8_t lednum = 0;
	uint8_t *colorBuff = NULL;
	uint8_t length = 0;
	
	uint16_t g_groupmgr_add = 0;
	uint16_t g_device_addr = 0;
	uint8_t port_num = 0;
	uint16_t RETAIN = 0;
	
  uint8_t funcode = 0;
  uint16_t DataLength = 0;
	uint8_t m_index = 0;
	if(length != 0){
		memset(colorBuff,0,length);
	}
  if(receivebuff[0] == 0xDD && receivebuff[1] == 0x55 && receivebuff[2] == 0xEE) {
		m_index+=HEAD_LENGTH_ws;
		
    //组地址
    g_groupmgr_add = (uint16_t)receivebuff[m_index]<<8 | receivebuff[m_index+1];
    m_index +=GROUPMGR_ADDR_LENGTH; //5
		
    //设备地址
    g_device_addr = (uint16_t)receivebuff[m_index]<<8 | receivebuff[m_index+1];
    m_index+=DEVICE_ADDR_LENGTH; //7
		
    //端口号
    port_num = receivebuff[m_index];
    m_index += PORT_LENGTH; // 8
		
    //功能码
    funcode = receivebuff[m_index];
    m_index+=FUNCODE_LENGTH; //9
		
    if(receivebuff[m_index] == LED_TYPE) { //灯带类型为0x01  WS2812B类型
			
      m_index+=LED_TYPE_LENGTH; //10 
      //保留部分数据
      RETAIN = (uint16_t)receivebuff[m_index]<<8 | receivebuff[m_index+1];
			
      m_index += RETAIN_LENTH; //12
      //数据长度
      DataLength = (uint16_t)receivebuff[m_index]<<8 | receivebuff[m_index+1];

      //判断数据是否为3的倍数 
      if(DataLength%3 != 0) {
				sendWaringMessage(5);
        return;
      }
			lednum = DataLength/3;

      m_index += DATA_LENGTH;  //14
      //扩展次数
      extendNum = receivebuff[m_index]<<8 | receivebuff[m_index+1];
			
			lednum = lednum * extendNum;
      m_index += EXTEND_LENGTH;  //16 //数据起始地址
      //判断数据长度是否准确
      if(receivebuff[m_index+DataLength] == 0xAA && receivebuff[m_index+DataLength+1] == 0xBB ) {
        
				
        colorBuff = (uint8_t*)malloc(sizeof(uint8_t)*extendNum*DataLength); //动态创建数据长度的数组

				length = extendNum*DataLength;
        for(uint8_t i = 0 ;i<extendNum; i++) {
					memcpy(colorBuff+i*DataLength,receivebuff+m_index,DataLength);
				}				
	
				//测试发送数据是否正确
				HAL_UART_Transmit(&huart1,colorBuff,extendNum*DataLength,0xffff);
        switch (funcode)
        {
        case 0x8F: //查询控制器唯一序列号
          /* code */
          break;
        case 0x90: //通过控制器唯一序列号查询控制器设备地址
          /* code */
          break;
        case 0x91: //通过控制器唯一序列号修改控制器设备地址
          /* code */
          break;
        case 0x92: //查询总线上有没有与指令中组地址和设备地址匹配的控制器
          /* code */
          break;
        case 0x93: //查询控制器组地址
          /* code */
          break;
        case 0x94: //查询控制器设备地址
          /* code */
          break;
        case 0x95: //修改控制器通信波特率
          /* code */
          break;
        case 0x96: //修改控制器组地址
          /* code */
          break;
        case 0x97: //修改控制器设备地址
          /* code */
          break;
        case 0x98: //扩展
          /* code */
          break;
        case 0x99: //显示颜色
          ws2812b_send(colorBuff,&lednum);
					free(colorBuff);
          /* code */
          break;  
        case 0x9B: //设置上电显示的数据
          /* code */
          break;  
        case 0x9C: //关闭上电显示功能
          /* code */
          break;  
        default:
					sendWaringMessage(4);
          break;
        }


        //memcpy(buff,receivebuff+index,DataLength);
      }
    } else {
			sendWaringMessage(9);
		}

  }
  
}









