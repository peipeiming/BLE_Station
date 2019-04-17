#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "strdeal.h"
#include "app_uart.h"
#include "ble_gattc.h"
#include "sdk_macros.h"
#include "identificator.h"

/*******数字转hex*******/
uint8_t c2i(char ch)  
{  
  //如果是数字，则用数字的ASCLL值减去48
  if(isdigit(ch))  
    return ch - 48;  
	else return ch;
} 

/***************************************************
函数名：ble_rscs_string_send
描述：特征值写入
参数：
     p_rscs_c_t：客户端结构体指针
		 p_sting：写入字符串（不超过20字节）
		 characteristic_handle：特征句柄
返回：
		 发送错误码
***************************************************/
uint32_t ble_rscs_string_send(ble_rscs_c_t *p_rscs_c_t,char *p_sting,uint16_t characteristic_handle)
{
  uint8_t stlen;
//	uint8_t i;
	
	stlen=strlen(p_sting);
	
#if 0
	printf("string_send:");
	for(i=0;i<stlen;i++)
	{
		p_sting[i]=c2i(p_sting[i]);
	  printf("%x ",p_sting[i]);
	}
	printf("\r\n");
#endif
	
	VERIFY_PARAM_NOT_NULL(p_rscs_c_t);
  
	if (strlen(p_sting)> 20)
	{
			return NRF_ERROR_INVALID_PARAM;
	}
	if ( p_rscs_c_t->conn_handle == BLE_CONN_HANDLE_INVALID)
	{
			return NRF_ERROR_INVALID_STATE;
	}
		
	const ble_gattc_write_params_t write_params = {
			.write_op = BLE_GATT_OP_WRITE_CMD,
			.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
			.handle   = characteristic_handle,
			.offset   = 0,
			.len      = stlen,
			.p_value  = (uint8_t *)p_sting
	};
	return sd_ble_gattc_write(p_rscs_c_t->conn_handle, &write_params);
}

/***************************************************
函数名：ble_rscs_message_send
描述：消息推送
参数：
     p_rscs_c_t：客户端结构体指针
		 p_sting：写入字符串
		 message_type：消息类型
返回：
		 发送错误码
***************************************************/
uint32_t ble_rscs_message_send(void)
{
	uint8_t i;
	uint8_t err;
	uint8_t sendlen;
	char message[20];

  uint8_t check_results;
	
  uint8_t Back_info[15]={0x5A,0x43,0x00,0x09,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0xCA};
	
	message[0]=3;
	message[1]=nrf_messend.txtotalcount;
	message[2]=nrf_messend.txcount+1;
	
	#if 0
	printf("txcount:%d\r\n",nrf_messend.txcount);
	printf("txtotalcount:%d\r\n",nrf_messend.txtotalcount);
	printf("txlen:%d\r\n",nrf_messend.txlen);
	for(i=0;i<nrf_messend.txlen;i++)
	{
		printf("%2x ",nrf_messend.txfifo[i]);
	}
	printf("\r\n");
	#endif
	
	if(nrf_messend.txcount<nrf_messend.txtotalcount-1)
	{
		sendlen=17;
		memcpy(&message[3],&nrf_messend.txfifo[nrf_messend.txcount*17],17);
	}
	else if(nrf_messend.txcount==nrf_messend.txtotalcount-1)  /*最后一包数据*/ 
	{
		sendlen=nrf_messend.txlen-17*nrf_messend.txcount;
		memcpy(&message[3],&nrf_messend.txfifo[nrf_messend.txcount*17],sendlen);
		for(i=0;i<20-sendlen-3;i++)
		{
			message[sendlen+3+i]=0x00;															/*给最后一包数据添加空字符*/
		}
	}

	#if 0
  for(i=0;i<20;i++)
	{
		printf("%2x ",message[i]);
	}
	printf("\r\n");
	#endif 
	  
	const ble_gattc_write_params_t write_params = {											//填充结构体
			.write_op = BLE_GATT_OP_WRITE_CMD,
			.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
			.handle   = m_ble_rscs_c.message_push_handle,
			.offset   = 0,
			.len      = 20,
			.p_value  = (uint8_t *)message
	};
	
	/*发送数据*/
	err=sd_ble_gattc_write(m_ble_rscs_c.conn_handle, &write_params);
	if(NRF_SUCCESS!=err)          
	{
		nrf_messend.txstartflag=0;   /*发送失败*/
		return err;
	}
	
	if(nrf_messend.txcount==nrf_messend.txtotalcount-1)
	{ 
		/*返回发送成功*/
		memcpy(&Back_info[4],station_addr,8);
		
		Back_info[12]=0x01;                       //发送成功

		check_results=Back_info[1];   
		for(i=2;i<(Back_info[3]+2+3-1);i++)       //CMD字段到数据字段进行校验
			check_results^=Back_info[i];
		Back_info[i]=check_results;               //校验值
		Back_info[i+1]=0xca;                      //结束符
		
		for(i=0;i<sizeof(Back_info);i++)          //发送数据
		{
			app_uart_put(Back_info[i]);	
		}
	  nrf_messend.txstartflag=0;
		
		is_connect_per_addr=false;
	}
	
	nrf_messend.txcount++; 	
	nrf_messend.txcompleteflag=0;
	return NRF_SUCCESS;
}
