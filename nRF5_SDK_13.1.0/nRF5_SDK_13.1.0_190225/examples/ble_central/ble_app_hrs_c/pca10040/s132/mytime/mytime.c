#include "mytime.h"
#include "app_uart.h"
#include "identificator.h"

//������ʱ��ʾ�� ������Ӧ�ö�ʱ������
APP_TIMER_DEF(m_timer_id);

//������ʱ��ʾ�� ����ʱ����ʱ�¼��ص�����
void m_timeout_handler(void * p_context)
{	
	uint32_t err_code;
	uint8_t i,check_results;
  uint8_t Send_Info[14]={0x5A,0x10,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0xCA};
	
	UNUSED_PARAMETER(p_context);
	
	if(registflag)  /*����ͷ�Ƿ�ע�ᣬregistflag=1ʱΪע�ᣬ�ر�Ӧ�ö�ʱ��*/
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
			Send_Info[4+i]=identificator_id.mac_8Byte[i]; //ʶ����MAC��ַ
		}
		
		check_results=Send_Info[1];   
		for(i=2;i<(Send_Info[3]+2+3-1);i++)             //CMD�ֶε������ֶν���У��
			check_results^=Send_Info[i];
		Send_Info[i]=check_results;                     //У��ֵ
		Send_Info[i+1]=0xca;                            //������
		
		for(i=0;i<sizeof(Send_Info);i++)                //��������
		{
			app_uart_put(Send_Info[i]);	
		}
	}
} 

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 * ��ʼ����ʱ����������ʱ����
 * ������Ҫ���ǹ��ĵ��ǣ������Լ��Ķ�ʱ����
 * ע�⣺��ʱ���񴴽�֮�󣬲�����������Ҫ�ں�������Լ���Ҫ�Ķ�ʱ���������ʱ����
 */
void m_timers_init(void)
{
    uint32_t err_code;
	  
    // ��ʼ����ʱ��ģ��
//    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    //�������������Ҫ�����Ķ�ʱ����
    /* YOUR_JOB: Create any timers to be used by the application.
                 Below is an example of how to create a timer.
                 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
                 one.
       uint32_t err_code;
       err_code = app_timer_create(&m_app_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
       APP_ERROR_CHECK(err_code); */
	  //������ʱ��ʾ�� ��������ʱ��    
    err_code = app_timer_create(&m_timer_id, APP_TIMER_MODE_REPEATED, m_timeout_handler);
    APP_ERROR_CHECK(err_code); 
	
}

/**@brief Function for starting timers.
���ǰ�洴����Ӧ�õĶ�ʱ����������ڴ�������ʱ��
 */
void application_timers_start(void)
{
		uint32_t err_code;
	  uint32_t timeout=0;
    /* YOUR_JOB: Start your timers. below is an example of how to start a timer.
       uint32_t err_code;
       err_code = app_timer_start(m_app_timer_id, TIMER_INTERVAL, NULL);
       APP_ERROR_CHECK(err_code); */
	
	  //������ʱ��ʾ�� ��������ʱ��
	  
	  #if 0
		srand(_ARMABI);          //����һ������
		timeout=rand();
	  timeout%=20000;
	  #else
	  timeout=5000;
	  #endif
	
//		printf("rand val is:%d",timeout);
    err_code = app_timer_start(m_timer_id,  APP_TIMER_TICKS(timeout) , NULL);
    APP_ERROR_CHECK(err_code);
}

