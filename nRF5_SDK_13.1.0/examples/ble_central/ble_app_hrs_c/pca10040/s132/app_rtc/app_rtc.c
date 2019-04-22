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

_calendar_obj calendar;//时钟结构体 
uint8_t  gap_data[9]={0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x00};  //信标号+低电压标志

const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(2); /**< Declaring an instance of nrf_drv_rtc for RTC2. */

extern void advertising_init(void);

/** RTC2中断处理函数
 * Triggered on TICK and COMPARE0 match.
 */
static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
	if (int_type == NRF_DRV_RTC_INT_COMPARE0)
	{	
		timecount+=2;
		link_time += 2;
    RTC_Get();            //更新时间   
		nrf_drv_rtc_counter_clear(&rtc);
		
		memcpy(&gap_data[1],calendar.data,5);
		gap_data[0]=calendar.timeparameter.w_year%2000;
		
		#if 1
		gap_data[7]=(is_connect_per_addr)?0x01:0x00;
		gap_data[8]=(connect_flag)?0x01:0x00;
		#endif
		
		if(!connect_dfu)   /*判断主机是否连接*/
		{
			if(advsatrt==1)  /*广播打开*/
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

/** 低频时钟配置。RTC使用的是LFCLK低频时钟
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
	
    //初始化RT实例。初始化完成后，RTC处于power off 状态
    err_code = nrf_drv_rtc_init(&rtc, &config , rtc_handler);
    APP_ERROR_CHECK(err_code);
  
    err_code = nrf_drv_rtc_cc_set(&rtc,0,8*2,true);
    APP_ERROR_CHECK(err_code);
	  
    //启动RTC
    nrf_drv_rtc_enable(&rtc);
}

//判断是否是闰年函数
//月份   1  2  3  4  5  6  7  8  9  10 11 12
//闰年   31 29 31 30 31 30 31 31 30 31 30 31
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
uint8_t Is_Leap_Year(uint16_t year)
{			  
	if(year%4==0) //必须能被4整除
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1;//如果以00结尾,还要能被400整除 	   
			else return 0;   
		}else return 1;   
	}else return 0;	
}	 

//设置时钟
//把输入的时钟转换为秒钟
//以1970年1月1日为基准
//1970~2099年为合法年份
//返回值:0,成功;其他:错误代码.
//月份数据表											 
uint8_t const table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5}; //月修正数据表	  
//平年的月份日期表
const uint8_t mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};
uint8_t RTC_Set(uint16_t syear,uint8_t smon,uint8_t sday,uint8_t hour,uint8_t min,uint8_t sec)
{
	uint16_t t;
	uint32_t seccount=0;
	if(syear<1970||syear>2099)return 1;	   
	for(t=1970;t<syear;t++)	//把所有年份的秒钟相加
	{
		if(Is_Leap_Year(t))seccount+=31622400;//闰年的秒钟数
		else seccount+=31536000;			  //平年的秒钟数
	}
	smon-=1;
	for(t=0;t<smon;t++)	   //把前面月份的秒钟数相加
	{
		seccount+=(uint32_t)mon_table[t]*86400;//月份秒钟数相加
		if(Is_Leap_Year(syear)&&t==1)seccount+=86400;//闰年2月份增加一天的秒钟数	   
	}
	seccount+=(uint32_t)(sday-1)*86400;//把前面日期的秒钟数相加 
	seccount+=(uint32_t)hour*3600;//小时秒钟数
  seccount+=(uint32_t)min*60;	 //分钟秒钟数
	seccount+=sec;//最后的秒钟加上去
  
	timecount=seccount;
	return 0;	    
}

//得到当前的时间
//返回值:0,成功;其他:错误代码.
uint8_t RTC_Get(void)
{
	static uint16_t daycnt=0;
	uint32_t temp=0;
	uint16_t temp1=0;	 
 	temp=timecount/86400;   //得到天数(秒钟数对应的)
	if(daycnt!=temp)//超过一天了
	{	  
		daycnt=temp;
		temp1=1970;	//从1970年开始
		while(temp>=365)
		{				 
			if(Is_Leap_Year(temp1))//是闰年
			{
				if(temp>=366)temp-=366;//闰年的秒钟数
				else {temp1++;break;}  
			}
			else temp-=365;	  //平年 
			temp1++;  
		}   
		calendar.timeparameter.w_year=temp1;//得到年份
		temp1=0;
		while(temp>=28)//超过了一个月
		{
			if(Is_Leap_Year(calendar.timeparameter.w_year)&&temp1==1)//当年是不是闰年/2月份
			{
				if(temp>=29)temp-=29;//闰年的秒钟数
				else break; 
			}
			else 
			{
				if(temp>=mon_table[temp1])temp-=mon_table[temp1];//平年
				else break;
			}
			temp1++;  
		}
		calendar.timeparameter.w_month=temp1+1;	//得到月份
		calendar.timeparameter.w_date=temp+1;  	//得到日期 
	}
	temp=timecount%86400;     		//得到秒钟数   	   
	calendar.timeparameter.hour=temp/3600;     	//小时
	calendar.timeparameter.min=(temp%3600)/60; 	//分钟	
	calendar.timeparameter.sec=(temp%3600)%60; 	//秒钟
	calendar.timeparameter.week=RTC_Get_Week(calendar.timeparameter.w_year,calendar.timeparameter.w_month,calendar.timeparameter.w_date);//获取星期   
	return 0;
}	 
//获得现在是星期几
//功能描述:输入公历日期得到星期(只允许1901-2099年)
//输入参数：公历年月日 
//返回值：星期号																						 
uint8_t RTC_Get_Week(uint16_t year,uint8_t month,uint8_t day)
{	
	uint16_t temp2;
	uint8_t yearH,yearL;
	
	yearH=year/100;	yearL=year%100; 
	// 如果为21世纪,年份数加100  
	if (yearH>19)yearL+=100;
	// 所过闰年数只算1900年之后的  
	temp2=yearL+yearL/4;
	temp2=temp2%7; 
	temp2=temp2+day+table_week[month-1];
	if (yearL%4==0&&month<3)temp2--;
	return(temp2%7);
}	

