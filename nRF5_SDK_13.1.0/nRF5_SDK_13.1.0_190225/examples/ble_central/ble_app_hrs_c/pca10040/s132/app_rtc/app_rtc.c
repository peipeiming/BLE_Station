#include "app_rtc.h"
#include "app_error.h"
#include "nrf_drv_rtc.h"
#include "nrf_drv_clock.h"
#include "identificator.h"
#include "ble_advertising.h"

uint32_t err_code;
volatile uint8_t advsatrt=0;
volatile uint8_t connect_dfu=0;
uint32_t timecount=0; 

_calendar_obj calendar;//ʱ�ӽṹ�� 
uint8_t  gap_data[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x00};  //�ű��+�͵�ѹ��־

const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(2); /**< Declaring an instance of nrf_drv_rtc for RTC2. */

extern void advertising_init(void);

/** RTC2�жϴ�����
 * Triggered on TICK and COMPARE0 match.
 */
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
	if (int_type == NRF_DRV_RTC_INT_COMPARE0)
	{	
		timecount+=2;
		link_time += 2;
    RTC_Get();            //����ʱ��   
		nrf_drv_rtc_counter_clear(&rtc);
		
		memcpy(&gap_data[1],calendar.data,5);
		gap_data[0]=calendar.timeparameter.w_year%2000;
		
		#if 1
		gap_data[7]=(is_connect_per_addr)?0x01:0x00;
		gap_data[8]=(connect_flag)?0x01:0x00;
		#endif
		
		if(!connect_dfu)   /*�ж������Ƿ�����*/
		{
			if(advsatrt==1)  /*�㲥��*/
			{
				sd_ble_gap_adv_stop();
				advsatrt=0;
			}	
			
			advertising_init();
			if(!advsatrt)
			{
				err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
				APP_ERROR_CHECK(err_code);
				if(NRF_SUCCESS==err_code)
				{
					advsatrt=1;
				}
			}
		}
	}
}

/** ��Ƶʱ�����á�RTCʹ�õ���LFCLK��Ƶʱ��
 */
void lfclk_config(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
}

/** @brief Function initialization and configuration of RTC driver instance.
 */
void rtc_config(void)
{
    uint32_t err_code;

	  nrf_drv_rtc_config_t config=NRF_DRV_RTC_DEFAULT_CONFIG;
		config.prescaler=4095;
	
    //��ʼ��RTʵ������ʼ����ɺ�RTC����power off ״̬
    err_code = nrf_drv_rtc_init(&rtc, &config , rtc_handler);
    APP_ERROR_CHECK(err_code);
  
    err_code = nrf_drv_rtc_cc_set(&rtc,0,8*2,true);
    APP_ERROR_CHECK(err_code);
	  
    //����RTC
    nrf_drv_rtc_enable(&rtc);
}

//�ж��Ƿ������꺯��
//�·�   1  2  3  4  5  6  7  8  9  10 11 12
//����   31 29 31 30 31 30 31 31 30 31 30 31
//������ 31 28 31 30 31 30 31 31 30 31 30 31
//����:���
//���:������ǲ�������.1,��.0,����
uint8_t Is_Leap_Year(uint16_t year)
{			  
	if(year%4==0) //�����ܱ�4����
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1;//�����00��β,��Ҫ�ܱ�400���� 	   
			else return 0;   
		}else return 1;   
	}else return 0;	
}	 

//����ʱ��
//�������ʱ��ת��Ϊ����
//��1970��1��1��Ϊ��׼
//1970~2099��Ϊ�Ϸ����
//����ֵ:0,�ɹ�;����:�������.
//�·����ݱ�											 
uint8_t const table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5}; //���������ݱ�	  
//ƽ����·����ڱ�
const uint8_t mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};
uint8_t RTC_Set(uint16_t syear,uint8_t smon,uint8_t sday,uint8_t hour,uint8_t min,uint8_t sec)
{
	uint16_t t;
	uint32_t seccount=0;
	if(syear<1970||syear>2099)return 1;	   
	for(t=1970;t<syear;t++)	//��������ݵ��������
	{
		if(Is_Leap_Year(t))seccount+=31622400;//�����������
		else seccount+=31536000;			  //ƽ���������
	}
	smon-=1;
	for(t=0;t<smon;t++)	   //��ǰ���·ݵ����������
	{
		seccount+=(uint32_t)mon_table[t]*86400;//�·����������
		if(Is_Leap_Year(syear)&&t==1)seccount+=86400;//����2�·�����һ���������	   
	}
	seccount+=(uint32_t)(sday-1)*86400;//��ǰ�����ڵ���������� 
	seccount+=(uint32_t)hour*3600;//Сʱ������
  seccount+=(uint32_t)min*60;	 //����������
	seccount+=sec;//�������Ӽ���ȥ
  
	timecount=seccount;
	return 0;	    
}

//�õ���ǰ��ʱ��
//����ֵ:0,�ɹ�;����:�������.
uint8_t RTC_Get(void)
{
	static uint16_t daycnt=0;
	uint32_t temp=0;
	uint16_t temp1=0;	 
 	temp=timecount/86400;   //�õ�����(��������Ӧ��)
	if(daycnt!=temp)//����һ����
	{	  
		daycnt=temp;
		temp1=1970;	//��1970�꿪ʼ
		while(temp>=365)
		{				 
			if(Is_Leap_Year(temp1))//������
			{
				if(temp>=366)temp-=366;//�����������
				else {temp1++;break;}  
			}
			else temp-=365;	  //ƽ�� 
			temp1++;  
		}   
		calendar.timeparameter.w_year=temp1;//�õ����
		temp1=0;
		while(temp>=28)//������һ����
		{
			if(Is_Leap_Year(calendar.timeparameter.w_year)&&temp1==1)//�����ǲ�������/2�·�
			{
				if(temp>=29)temp-=29;//�����������
				else break; 
			}
			else 
			{
				if(temp>=mon_table[temp1])temp-=mon_table[temp1];//ƽ��
				else break;
			}
			temp1++;  
		}
		calendar.timeparameter.w_month=temp1+1;	//�õ��·�
		calendar.timeparameter.w_date=temp+1;  	//�õ����� 
	}
	temp=timecount%86400;     		//�õ�������   	   
	calendar.timeparameter.hour=temp/3600;     	//Сʱ
	calendar.timeparameter.min=(temp%3600)/60; 	//����	
	calendar.timeparameter.sec=(temp%3600)%60; 	//����
	calendar.timeparameter.week=RTC_Get_Week(calendar.timeparameter.w_year,calendar.timeparameter.w_month,calendar.timeparameter.w_date);//��ȡ����   
	return 0;
}	 
//������������ڼ�
//��������:���빫�����ڵõ�����(ֻ����1901-2099��)
//������������������� 
//����ֵ�����ں�																						 
uint8_t RTC_Get_Week(uint16_t year,uint8_t month,uint8_t day)
{	
	uint16_t temp2;
	uint8_t yearH,yearL;
	
	yearH=year/100;	yearL=year%100; 
	// ���Ϊ21����,�������100  
	if (yearH>19)yearL+=100;
	// ����������ֻ��1900��֮���  
	temp2=yearL+yearL/4;
	temp2=temp2%7; 
	temp2=temp2+day+table_week[month-1];
	if (yearL%4==0&&month<3)temp2--;
	return(temp2%7);
}	

