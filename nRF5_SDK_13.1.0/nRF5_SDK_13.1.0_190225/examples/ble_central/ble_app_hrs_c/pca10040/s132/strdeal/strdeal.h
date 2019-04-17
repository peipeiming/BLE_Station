#ifndef STRDEAL_H
#define STRDEAL_H

#include "stdint.h"
#include "ble_rscs_c.h"

#define   QQ_MESSAGE         1        /**< QQ message. */             
#define   WECHAT_MESSAGE  	 2        /**< wechat message. */ 
#define		SMS_MESSAGE   		 3		    /**< sms message. */ 
#define		SKYPE_MESSAGE   	 4				/**< skype message. */ 
#define		WHATAPP_MESSAGE    5	      /**< whatapp message. */ 
#define		FACEBOOK_MESSAGE   6	      /**< facebook message. */ 
#define		TWITER_MESSAGE     7        /**< twiter message. */ 
#define	  LINE_MESSAGE       8        /**< line message. */ 

#define   CALL_REMIND_ENABLEBIT       2
#define   SMS_REMIND_ENABLEBIT        3
#define   WECHAT_REMIND_ENABLEBIT     4
#define   QQ_REMIND_ENABLEBIT  				5
#define   SKYPE_REMIND_ENABLEBIT  		6
#define   WHATAPP_REMIND_ENABLEBIT  	7
#define   FACEBOOK_REMIND_ENABLEBIT  	8
#define   TWITER_REMIND_ENABLEBIT     9
#define   LINE_REMIND_ENABLEBIT       10

extern uint8_t Mes_start;
extern uint8_t Txcompleteflag;

uint8_t c2i(char ch);
uint32_t ble_rscs_message_send(void);
uint32_t ble_rscs_string_send(ble_rscs_c_t *p_rscs_c_t,char *p_sting,uint16_t characteristic_handle);
//uint32_t ble_rscs_message_send(ble_rscs_c_t *p_rscs_c_t,char *p_sting,uint8_t message_type);

#endif

