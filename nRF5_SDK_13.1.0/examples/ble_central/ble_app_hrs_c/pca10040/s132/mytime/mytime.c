#include "mytime.h"
#include "app_uart.h"
#include "identificator.h"

//创建定时器示例 ：定义应用定时器变量
APP_TIMER_DEF(m_timer_id);

//创建定时器示例 ：定时器超时事件回调函数
void m_timeout_handler(void * p_context)
{	
	uint32_t err_code;
	uint8_t i,check_results;
  uint8_t Send_Info[14]={0x5A,0x10,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0xCA};
	
	UNUSED_PARAMETER(p_context);
	
	if(registflag)  /*检查读头是否注册，registflag=1时为注册，关闭应用定时器*/
	{
		app_timer_stop(m_timer_id);
	}
	else
	{
		for(i=0;i<8;i++)
		{
			#if 0
			printf("%2x ",identificator_id.mac_8Byte[i]);
			#endif
			Send_Info[4+i]=identificator_id.mac_8Byte[i]; //识别器MAC地址
		}
		
		check_results=Send_Info[1];   
		for(i=2;i<(Send_Info[3]+2+3-1);i++)             //CMD字段到数据字段进行校验
			check_results^=Send_Info[i];
		Send_Info[i]=check_results;                     //校验值
		Send_Info[i+1]=0xca;                            //结束符
		
		for(i=0;i<sizeof(Send_Info);i++)                //发送数据
		{
			app_uart_put(Send_Info[i]);	
		}
	}
} 

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 * 初始化定时器，创建定时任务
 * 这里需要我们关心的是：创建自己的定时任务
 * 注意：定时任务创建之后，不会启动，需要在后面根据自己需要的定时间隔启动定时任务
 */
void m_timers_init(void)
{
    uint32_t err_code;
	  
    // 初始化定时器模块
//    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    //这里添加我们需要创建的定时任务
    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.
       uint32_t err_code;
       err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
       APP_ERROR_CHECK(err_code); */
	  //创建定时器示例 ：创建定时器    
    err_code = app_timer_create(&m_timer_id, APP_TIMER_MODE_REPEATED, m_timeout_handler);
    APP_ERROR_CHECK(err_code); 
	
}

/**@brief Function for starting timers.
如果前面创建了应用的定时器，则可以在此启动定时器
 */
void application_timers_start(void)
{
		uint32_t err_code;
	  uint32_t timeout=0;
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.
       uint32_t err_code;
       err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
       APP_ERROR_CHECK(err_code); */
	
	  //创建定时器示例 ：启动定时器
	  
	  #if 0
		srand(_ARMABI);          //产生一个种子
		timeout=rand();
	  timeout%=20000;
	  #else
	  timeout=5000;
	  #endif
	
//		printf("rand val is:%d",timeout);
    err_code = app_timer_start(m_timer_id,  APP_TIMER_TICKS(timeout) , NULL);
    APP_ERROR_CHECK(err_code);
}

