#include "boards.h"
#include "mytime.h"
#include "ble_dfu.h"
#include "strdeal.h"
#include "ble_hci.h"
#include "app_adv.h"
#include "mble_dfu.h"
#include "core_cm4.h"
#include "app_uart.h"
#include "app_rtc.h"
#include "ble_advertising.h"

#include "includes.h"

volatile uint32_t link_time = 0;

uint8_t station_addr[8] = {0};

static uint8_t register_flag = UNREGISTER;

volatile bool do_connect = false;
volatile bool connect_flag = false;

uint8_t dfu_bracelet_err_code = NRF_SUCCESS;

app_messend nrf_messend;

extern void scan_start(void);

ble_bas_c_t                  m_ble_bas_c;                  /**< Structure used to identify the Battery Service client module. */
ble_rscs_c_t                 m_ble_rscs_c;                 /**< Structure used to identify the Runnign Speed and Cadence client module. */

uint16_t                     m_conn_handle;                /**< Current connection handle. */
uint16_t              			 m_pending_db_disc_conn=BLE_CONN_HANDLE_INVALID;

/**@brief Names which the central applications will scan for, and which will be advertised by the peripherals.
 *  if these are set to empty strings, the UUIDs defined below will be used
 */

const char m_target_periph_name[] = "S1";     /**< If you want to connect to a peripheral using a given advertising name, type its name here. */
volatile bool  is_connect_per_addr = false;                /**< If you want to connect to a peripheral with a given address, set this to true and put the correct address in the variable below. */
ble_gap_addr_t m_target_periph_addr =
{
    /* Possible values for addr_type:
       BLE_GAP_ADDR_TYPE_PUBLIC,
       BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
       BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE,
       BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE. */
      .addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,
 	    .addr      = {0x1B,0x00,0x00,0x1F,0x31,0xEF}
};

/*
*********************************************************************************************************
 * ��������app_ble_diconnect
 * ����  ���Ͽ�������վ��������
 * ����  ��*p,���ؿ�������
 * ����  : ��
**********************************************************************************************************
 */
void static app_ble_diconnect(uint8_t *p)
{
	uint8_t i;	

	if(DfuState != DFU_STATE_IDLE)
	{
		return;
	}
	
	if(0 != memcmp(&p[4],station_addr,8))
	{
		return;
	}
  
	is_connect_per_addr = false;
	
	for(i = 0; i < 14; i++)
	{
		app_uart_put(p[i]);
	}	
	
	sd_ble_gap_disconnect( m_pending_db_disc_conn , BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
}

/*
*********************************************************************************************************
 * ��������app_ble_connectbracelet
 * ����  �����������ֻ�
 * ����  ��*p,���ؿ�������
 * ����  : ��
**********************************************************************************************************
 */
void static app_ble_connectbracelet(uint8_t *p)
{
	if(DfuState != DFU_STATE_IDLE)
	{
		return;
	}
	
	if(0 != memcmp(&p[4],station_addr,8))
	{
		return;
	}
	
	sd_ble_gap_disconnect( m_pending_db_disc_conn , BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	
	link_time = 0;               /*��ʼ��ʱ����ʱ��*/
	is_connect_per_addr = true;
	memcpy(m_target_periph_addr.addr,&p[12],6);
}

/*�Ͽ���ǰ��������,����ɨ��㲥ģʽ�¼����������*/
void disconnect_ble_link(void)  
{
	uint32_t err_code;	
	
	if(m_pending_db_disc_conn == BLE_CONN_HANDLE_INVALID)
	{
		return;
	}	
	
	if(advsatrt==1) /*�رչ㲥*/
	{
		bsp_board_led_off(BSP_BOARD_LED_2);
		sd_ble_gap_adv_stop();
		advsatrt=0;
	}	
 
	nrf_delay_ms(2000);
	err_code = sd_ble_gap_disconnect( m_pending_db_disc_conn , BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	#if 0
	if(err_code != NRF_SUCCESS)
	{
		printf("disconnect err_code:%d\r\n",err_code);
		nrf_delay_ms(1000);
	}
	#endif
	nrf_delay_ms(100);
}

/*
*********************************************************************************************************
 * ��������MiBand_info_save
 * ����  ��С���ֻ����ݴ����ϱ�
 * ����  ��*m_gap_evt_t,�㲥��Ϣ�ṹ��
 * ����  : ��
**********************************************************************************************************
 */
void MiBand_info_save(ble_gap_evt_t const *m_gap_evt_t)              	 
{
	uint8_t i = 0;
	uint8_t check_results = 0;
	uint8_t gap_info[41]={0x5a,0x41,0x00,0x23};

	uint8_t electricity = 0;
	uint16_t total_steps = 0;
	uint16_t total_calorie = 0;
	uint16_t total_distance = 0;

	
	memcpy(&gap_info[4],station_addr,8);
	memcpy(&gap_info[12],m_gap_evt_t->params.adv_report.peer_addr.addr,6);
	
	gap_info[18] = -m_gap_evt_t->params.adv_report.rssi;              
  
	memcpy((uint8_t *)&total_steps,&m_gap_evt_t->params.adv_report.data[19],2);   /*�Ʋ�*/
	total_calorie = total_steps*35.0*0.0006564;  /*��·��*/
	total_distance = total_steps*140*0.45*0.01;  /*�ܾ���*/
	electricity = m_gap_evt_t->params.adv_report.data[24]; /*�ֻ�����*/
	
	memset(&gap_info[25],0xaa,4); 							 /*��λ���ݿ�*/
	memcpy(&gap_info[29],(uint8_t *)&total_steps,2);
	memcpy(&gap_info[31],(uint8_t *)&total_calorie,2);
	memcpy(&gap_info[33],(uint8_t *)&total_distance,2);
	gap_info[37] = electricity;
	gap_info[38] = 0xff;
	
  check_results = gap_info[1];
	for(i = 2; i < gap_info[3] + 4; i++)     						                
	{
		check_results ^= gap_info[i];
	}
  gap_info[i] = check_results;                                        
	gap_info[i+1] = 0xca;                                               

	adv_writebuf(gap_info,sizeof(gap_info));
}

/*
*********************************************************************************************************
 * ��������gap_info_save
 * ����  ������㲥������
 * ����  ��*m_gap_evt_t,�㲥��Ϣ�ṹ��
 * ����  : ��
**********************************************************************************************************
 */
void gap_info_save(ble_gap_evt_t const *m_gap_evt_t)              	 
{
	uint8_t i = 0;
	uint8_t check_results = 0;
	uint8_t gap_info[41]={0x5a,0x41,0x00,0x23};
	
	memcpy(&gap_info[4],station_addr,8);
	memcpy(&gap_info[12],m_gap_evt_t->params.adv_report.peer_addr.addr,6);
	
	gap_info[18] = -m_gap_evt_t->params.adv_report.rssi;              //RSSIֵд��

	for(i=0;i<m_gap_evt_t->params.adv_report.dlen;i++)                //�㲥������  
  {	
	  if(0xFF == m_gap_evt_t->params.adv_report.data[i])
	  {
			memcpy(&gap_info[19],&m_gap_evt_t->params.adv_report.data[i+3],20);
		  break;
	  }
	}
	
  check_results=gap_info[1];
	for(i=2;i<(gap_info[3]+2+3-1);i++)     						                 //CMD�ֶε������ֶν���У��
	{
		check_results^=gap_info[i];
	}
	
  gap_info[i]=check_results;                                         //У��ֵ
	gap_info[i+1]=0xca;                                                //������
	
	adv_writebuf(gap_info,sizeof(gap_info));
}
/*
*********************************************************************************************************
 * ��������app_ble_messagesend
 * ����  �����ֻ�������Ϣ
 * ����  ��*p,��Ϣ��������������Ŀ���ֻ���MAC��ַ����Ϣ
 * ����  : ��
**********************************************************************************************************
 */
void static app_ble_messagesend(uint8_t *p)
{
	uint32_t err;

	char switchtable[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};
  
	if(!connect_flag)
	{
		return;   /*δ����״̬����*/
	}
	
	if(0 != memcmp(&p[4],station_addr,8))
	{
		return;
	}
	
	switchtable[3]=0x01;
	err=ble_rscs_string_send(&m_ble_rscs_c,switchtable,m_ble_rscs_c.message_push_switch_handle);     //ʹ����Ϣ����   
	if(err!=NRF_SUCCESS)
	{
		return;
	}
	
	nrf_messend.txcount=0;	
	nrf_messend.txstartflag=1;
	nrf_messend.txcompleteflag=1;
	nrf_messend.txlen=p[2]*256+p[3]-8;   								   /*��Ϣ����*/
  memcpy(nrf_messend.txfifo,&p[12],nrf_messend.txlen); /*������Ϣ*/ 
	nrf_messend.txtotalcount=nrf_messend.txlen/17;
	if(nrf_messend.txlen%17!=0)
	{
		nrf_messend.txtotalcount++;
	}
}							

/*
*********************************************************************************************************
 * ��������app_ble_settime
 * ����  ��У׼������վ����ʱ��
 * ����  ��*p,ʱ�����
 * ����  : ��
**********************************************************************************************************
 */
void static app_ble_settime(uint8_t *p)
{
	uint16_t year;
	
	year = p[4];
	year += 2000;
  RTC_Set(year,p[5],p[6],p[7],p[8],p[9]);    /*����RTCʱ��*/
}

/*
*********************************************************************************************************
 * ��������app_ble_register
 * ����  ���豸ע��
 * ����  ��*p,ע������
 * ����  : ��
**********************************************************************************************************
 */
void static app_ble_register(uint8_t *p)
{
	uint8_t check_results;
	uint8_t reqdata[14] = {0x5a,0x44,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4c,0xca};
	
	if( 0 == memcmp(p,reqdata,sizeof(reqdata)) && register_flag == UNREGISTER)
	{
		memcpy(&reqdata[4],station_addr,8);

		check_results = reqdata[1];   
		for(uint8_t i = 2; i < (reqdata[3] + 4); i++)   //CMD�ֶε������ֶν���У��
		{
			check_results^=reqdata[i];
		}
		reqdata[12] = check_results;     //У��ֵ
		
		for(uint8_t i = 0; i < sizeof(reqdata); i++)
		{
			app_uart_put(reqdata[i]);
		}	
	}
	else if(0 == memcmp(&p[4],station_addr,sizeof(station_addr)) && register_flag == UNREGISTER)
	{
		register_flag = REGISTER;
	}
}

void static app_ble_reset(uint8_t *p)
{
	uint8_t reqdata[14] = {0x5a,0x45,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4d,0xca};
	
	if( 0 == memcmp(p,reqdata,sizeof(reqdata)))
	{
		NVIC_SystemReset();
	}
}

/*
*********************************************************************************************************
 * ��������app_ble_bootloaderenter
 * ����  �����봮������boot
 * ����  ��*p,���ݻ���ָ��
 * ����  : ��
**********************************************************************************************************
 */
void static app_ble_bootloaderenter(uint8_t *p)
{
	if(DfuState != DFU_STATE_IDLE)
	{
		return;
	}
	
	if(0 != memcmp(&p[4],station_addr,8))
	{
		return;
	}
	
	bootloader_start();
}

/***************************************************
��������app_ble_rssiset
�����������ֻ�ɨ���ű�B1����ֵ
������*p�����ݻ���ָ��
���أ���
***************************************************/
void static app_ble_rssiset(uint8_t *p)
{
	if(p[11] >= 50 && p[11] <= 100)
	{
		gap_data[6] = p[11];
	}
}

/***************************************************
��������app_ble_updatabracelet
���������������ֻ�
������*p�����ݻ���ָ��
���أ���
***************************************************/
void static app_ble_updatabracelet(uint8_t *p)
{
	uint8_t target_periph_addr[6] = {0x00}; 
	
  if(p[4] == 0x01 && DfuState == DFU_STATE_IDLE)        /*��ʼ����ָ��*/
	{
		DfuState = DFU_STATE_START;
	   		
		sd_ble_gap_disconnect( m_pending_db_disc_conn , BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		
		is_connect_per_addr = true;
		memcpy(m_target_periph_addr.addr,&p[5],6);
		
		memcpy((uint8_t *)&m_dfu.firm_size,&p[11],4);       /*�̼�����*/
		memcpy((uint8_t *)&m_dfu.dfu_init_key,&p[15],14);   /*�̼�����*/
		
		/*���������̼��ܰ���*/
		m_dfu.pack_num = m_dfu.firm_size / 20;
		if(m_dfu.firm_size % 20 != 0)
		{
			m_dfu.pack_num++;
		}
		m_dfu.pack_count = 0;
		
		NRF_LOG_INFO("> DFU bracelet start. total firmware size:%d  packes:%d\r\n",m_dfu.firm_size,m_dfu.pack_num);
	}
	else if(p[4] == 0x02 && m_dfu.startflag == DFU_FIR_TX_END)
	{	
		memcpy(target_periph_addr,&p[5],6);
		target_periph_addr[0] += 1;
		if(0 != memcmp(target_periph_addr,m_target_periph_addr.addr,6))
		{
			NRF_LOG_INFO("> target addr error\r\n");
			return;
		}
		
		m_dfu.curr_firm_len = p[2]*256 + p[3] - 7;
		if(m_dfu.curr_firm_len > 1000)
		{
			NRF_LOG_INFO("> firm len error\r\n");
			return;
		}
		memset(m_dfu.firmware,0x00,sizeof(m_dfu.firmware));
		memcpy(m_dfu.firmware,&p[11],m_dfu.curr_firm_len);
		
		m_dfu.current_tx_num = 0;
		m_dfu.startflag = DFU_FIR_TX_START;    /*��ʼ�̼�����*/
		
		NRF_LOG_INFO("> Receive firmware data. len:%d\r\n",m_dfu.curr_firm_len);
	}
}

/***************************************************
��������uart_data_deal
�������������ݴ���
������*p�����ݻ���ָ��
���أ���
***************************************************/
void uart_data_deal(uint8_t *buff,uint16_t buff_len)
{
	switch(buff[1])
	{
		case DISCONNECTBLE:
			app_ble_diconnect(buff);     		  //�Ͽ�����
			break;
		
		case GETBLEDATA:
			adv_readbuf(buff);         		    //��ȡ�㲥������
			break;
		
		case CONNECTBLE:
			app_ble_connectbracelet(buff);    //����Ŀ��
			break;

		case SEND_MESSAGE:
			app_ble_messagesend(buff);        //������Ϣ        	   
	    break;
		
	  case SET_TIME:		                  //У׼��վʱ��
			app_ble_settime(buff);
			break;

	  case GET_STATION:		                //ע��
			app_ble_register(buff);
			break;
		
	  case RESET:		                      //��λ����
			app_ble_reset(buff);
			break;

		case UPDATA_STATION:                //������������boot
			app_ble_bootloaderenter(buff);
			break;
		
		case SET_RSSI:                      //�����ֻ�ɨ���ű�RSSI
			app_ble_rssiset(buff);
			break;
		
		case UPDATA_BRACELET:               //�����ֻ��̼�
			app_ble_updatabracelet(buff);
			break;
		
		default:break;
	}
}




