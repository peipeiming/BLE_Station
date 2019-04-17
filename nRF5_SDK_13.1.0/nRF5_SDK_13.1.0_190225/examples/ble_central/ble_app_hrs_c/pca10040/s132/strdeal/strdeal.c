#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "strdeal.h"
#include "app_uart.h"
#include "ble_gattc.h"
#include "sdk_macros.h"
#include "identificator.h"

/*******����תhex*******/
uint8_t c2i(char ch)  
{  
  //��������֣��������ֵ�ASCLLֵ��ȥ48
  if(isdigit(ch))  
    return ch - 48;  
	else return ch;
} 

/***************************************************
��������ble_rscs_string_send
����������ֵд��
������
     p_rscs_c_t���ͻ��˽ṹ��ָ��
		 p_sting��д���ַ�����������20�ֽڣ�
		 characteristic_handle���������
���أ�
		 ���ʹ�����
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
��������ble_rscs_message_send
��������Ϣ����
������
     p_rscs_c_t���ͻ��˽ṹ��ָ��
		 p_sting��д���ַ���
		 message_type����Ϣ����
���أ�
		 ���ʹ�����
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
	else if(nrf_messend.txcount==nrf_messend.txtotalcount-1)  /*���һ������*/ 
	{
		sendlen=nrf_messend.txlen-17*nrf_messend.txcount;
		memcpy(&message[3],&nrf_messend.txfifo[nrf_messend.txcount*17],sendlen);
		for(i=0;i<20-sendlen-3;i++)
		{
			message[sendlen+3+i]=0x00;															/*�����һ��������ӿ��ַ�*/
		}
	}

	#if 0
  for(i=0;i<20;i++)
	{
		printf("%2x ",message[i]);
	}
	printf("\r\n");
	#endif 
	  
	const ble_gattc_write_params_t write_params = {											//���ṹ��
			.write_op = BLE_GATT_OP_WRITE_CMD,
			.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
			.handle   = m_ble_rscs_c.message_push_handle,
			.offset   = 0,
			.len      = 20,
			.p_value  = (uint8_t *)message
	};
	
	/*��������*/
	err=sd_ble_gattc_write(m_ble_rscs_c.conn_handle, &write_params);
	if(NRF_SUCCESS!=err)          
	{
		nrf_messend.txstartflag=0;   /*����ʧ��*/
		return err;
	}
	
	if(nrf_messend.txcount==nrf_messend.txtotalcount-1)
	{ 
		/*���ط��ͳɹ�*/
		memcpy(&Back_info[4],station_addr,8);
		
		Back_info[12]=0x01;                       //���ͳɹ�

		check_results=Back_info[1];   
		for(i=2;i<(Back_info[3]+2+3-1);i++)       //CMD�ֶε������ֶν���У��
			check_results^=Back_info[i];
		Back_info[i]=check_results;               //У��ֵ
		Back_info[i+1]=0xca;                      //������
		
		for(i=0;i<sizeof(Back_info);i++)          //��������
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
