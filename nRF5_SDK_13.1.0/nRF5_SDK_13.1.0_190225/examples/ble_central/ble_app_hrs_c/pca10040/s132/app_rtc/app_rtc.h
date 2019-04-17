#ifndef APP_RTC_H
#define APP_RTC_H

#include "stdint.h"

//时间结构体
typedef struct 
{	
	volatile uint8_t  w_month;
	volatile uint8_t  w_date;
	volatile uint8_t hour;
	volatile uint8_t min;
	volatile uint8_t sec;		
	
	//公历日月年周
	volatile uint16_t w_year;
	volatile uint8_t  week;		 
}Time;		

typedef union
{
	Time timeparameter;
  uint8_t data[8];
}_calendar_obj;

extern volatile uint8_t connect_dfu;
extern volatile uint8_t advsatrt;
extern uint8_t  gap_data[9];




uint8_t RTC_Get(void);
void rtc_config(void);
void lfclk_config(void);
uint8_t Is_Leap_Year(uint16_t year);
uint8_t RTC_Get_Week(uint16_t year,uint8_t month,uint8_t day);
uint8_t RTC_Set(uint16_t syear,uint8_t smon,uint8_t sday,uint8_t hour,uint8_t min,uint8_t sec);

#endif

