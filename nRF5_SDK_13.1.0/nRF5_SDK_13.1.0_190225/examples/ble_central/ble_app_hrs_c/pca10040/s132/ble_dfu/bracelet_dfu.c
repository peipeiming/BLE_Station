#include "includes.h"

Dfu_Ctr_t m_dfu;

DFU_STATE DfuState = DFU_STATE_IDLE;

uint8_t cmd_start_dfu[2] = {0x01,0x04};
uint8_t cmd_validate_firmware[12] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

uint8_t cmd_init_dfu_start[2] = {0x02,0x00};
uint8_t cmd_init_dfu_end[2] = {0x02,0x01};
uint8_t cmd_firmware_start[1] = {0x03};
uint8_t cmd_check_firmware[1] = {0x04};
uint8_t cmd_end_dfu[1] = {0x05};

void Dfu_bracelet(void)
{
	switch(DfuState)
	{
		case DFU_STATE_START:
			link_time = 0;
		  DfuState = DFU_STATE_LINK_BRA;
	    NRF_LOG_INFO("> Start link bracelet ");
		  NRF_LOG_INFO("%02X %02X %02X %02X %02X %02X\r\n",m_target_periph_addr.addr[5],m_target_periph_addr.addr[4],m_target_periph_addr.addr[3],
		  m_target_periph_addr.addr[2],m_target_periph_addr.addr[1],m_target_periph_addr.addr[0]);
			break;
		
		case DFU_STATE_LINK_BRA:
			if(link_time > 15)
			{
		  	NRF_LOG_INFO("> Link bracelet out time try to link DfuTarg\r\n");
				DfuState = DFU_STATE_LINK_TAG_START;  /*没有找到需要升级的手环 尝试连接 DfuTarg*/                
			}
			break;
			
		case DFU_STATE_LINK_BRA_OK:
			NRF_LOG_INFO("> Link bracelet ok\r\n");
			if(NRF_SUCCESS != ble_dfu_bracelet_start())
			{
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Enter bracelet dfu fail\r\n");
				m_target_periph_addr.addr[0] += 1; 
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_TAG);
				break;
			}
		  DfuState = DFU_STATE_LINK_TAG_START;
		  NRF_LOG_INFO("> Enter dfu mode\r\n");
			break;
		
		case DFU_STATE_LINK_TAG_START:  /*连接 DfuTarg*/
			link_time = 0;
		  is_connect_per_addr = true;
		  m_target_periph_addr.addr[0] += 1; 
		
		  NRF_LOG_INFO("> Start link Dfutarg\r\n");
			DfuState = DFU_STATE_LINK_TAG;
			break;

		case DFU_STATE_LINK_TAG:
			if(link_time > 15)
			{
				NRF_LOG_INFO("> Link Dfutarg out time\r\n");
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_LINK);
				DfuState = DFU_STATE_IDLE;      /*超时退出*/
			}
			break;
		
		case DFU_STATE_LINK_TAG_OK:
			NRF_LOG_INFO("> Link Dfutarg ok\r\n");
		  DfuState = DFU_STATE_VALIDATE_FIRMWARE;
			break;
		
		case DFU_STATE_VALIDATE_FIRMWARE:   /*验证固件*/
			link_time = 0;
			NRF_LOG_INFO("> DFU validate firmware\r\n");
		  m_dfu.respon_flag = DFU_WRITE_NO_RESPON;
		  if(NRF_SUCCESS != ble_dfus_c_write_cmd(&m_ble_dfus_c,cmd_start_dfu,2))
			{
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_WRITE);
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Write validate firmware cmd error\r\n");
				break;
			}
			
			/*发送固件长度*/
			memcpy(&cmd_validate_firmware[8],(uint8_t *)&m_dfu.firm_size,4);
		  if(NRF_SUCCESS != ble_dfus_c_write_data(&m_ble_dfus_c,cmd_validate_firmware,12))
			{
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_WRITE);
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Write validate firmware cmd error\r\n");
				break;
			}			
		  DfuState = DFU_STATE_WAIT_VALIDATE;
			break;

		case DFU_STATE_WAIT_VALIDATE:
			if(link_time > 10)
			{
				NRF_LOG_INFO("> Wait validate firmware out time\r\n");
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_WRITE);
				DfuState = DFU_STATE_IDLE;
				break;
			}
			
			if(m_dfu.respon_flag == DFU_WRITE_RESPON)
			{
				if(m_dfu.respon[1] != 0x01 || m_dfu.respon[2] != 0x01)
				{
					if(m_dfu.respon[1] == 0x01 || m_dfu.respon[2] == 0x02)
					{
					  /*无效的状态，尝试初始化DFU参数*/	
						DfuState = DFU_STATE_INIT_DFU;   	
						NRF_LOG_INFO("> Validate firmware response invalid state\r\n");
						NRF_LOG_INFO("> Try to init Dfu\r\n");
						break;
					}
					
					NRF_LOG_INFO("> Validate firmware not right response\r\n");
					ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,((m_dfu.respon[2] == 0x01)?DFU_ERROR_RESPON:m_dfu.respon[2]));
					DfuState = DFU_STATE_IDLE;	
				}
				else
				{
					NRF_LOG_INFO("> Validate firmware ok\r\n");
					DfuState = DFU_STATE_INIT_DFU;					
				}
			}
			break;

		case DFU_STATE_INIT_DFU:   /*初始化DFU参数*/
			link_time = 0;
			NRF_LOG_INFO("> DFU init parameters\r\n");
		  m_dfu.respon_flag = DFU_WRITE_NO_RESPON;
		  if(NRF_SUCCESS != ble_dfus_c_write_cmd(&m_ble_dfus_c,cmd_init_dfu_start,2))
			{
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_WRITE);
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Write init parameters start cmd error\r\n");
				break;
			}
			
		  if(NRF_SUCCESS != ble_dfus_c_write_data(&m_ble_dfus_c,m_dfu.dfu_init_key,14)) /*发送密钥*/
			{
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_WRITE);
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Write init parameters cmd error\r\n");
				break;
			}			

			nrf_delay_ms(100);   
		  if(NRF_SUCCESS != ble_dfus_c_write_cmd(&m_ble_dfus_c,cmd_init_dfu_end,2))   
			{
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_WRITE);
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Write init parameters end cmd error\r\n");
				break;
			}					
			
			DfuState = DFU_STATE_WAIT_INIT;
			break;

		case DFU_STATE_WAIT_INIT:
			if(link_time > 5)
			{
				NRF_LOG_INFO("> Wait init dfu out time\r\n");
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,DFU_ERROR_WRITE);
				DfuState = DFU_STATE_IDLE;
				break;
			}
			
			if(m_dfu.respon_flag == DFU_WRITE_RESPON)
			{
				if(m_dfu.respon[1] != 0x02 || m_dfu.respon[2] != 0x01)
				{
					NRF_LOG_INFO("> Init Dfu not right response\r\n");
					ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,((m_dfu.respon[2] == 0x01)?DFU_ERROR_RESPON:m_dfu.respon[2]));
					DfuState = DFU_STATE_IDLE;	
					break;
				}
				else
				{
					NRF_LOG_INFO("> Init dfu ok\r\n");
					DfuState = DFU_STATE_TRAN_FIRMWARE_START;	
          ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_START,0x01);			
          nrf_delay_ms(100);   					
				}
			}
			break;

		case DFU_STATE_TRAN_FIRMWARE_START:
			link_time = 0;
		  m_dfu.respon_flag = DFU_WRITE_NO_RESPON;
		  
			if(NRF_SUCCESS != ble_dfus_c_write_cmd(&m_ble_dfus_c,cmd_firmware_start,1))
			{
				ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_DATA,0x02);
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Write start firmware cmd error\r\n");
				break;
			}	
			
			NRF_LOG_INFO("> Start firmware transfer\r\n");
			DfuState = DFU_STATE_TRAN_FIRMWARE;
			m_dfu.txcompleteflag = DFU_FIR_TX_COMPL;
			m_dfu.startflag = DFU_FIR_TX_END;
			break;

		case DFU_STATE_TRAN_FIRMWARE:
			if(m_dfu.txcompleteflag == DFU_FIR_TX_COMPL && m_dfu.startflag == DFU_FIR_TX_START)
			{
				uint8_t tx_len = 20;
				
				link_time = 0;
			  m_dfu.txcompleteflag = DFU_FIR_TX_CLEAR;
				
				if((m_dfu.current_tx_num + 1)*20 > m_dfu.curr_firm_len)  /*计算发送长度*/
				{
					tx_len = m_dfu.curr_firm_len - m_dfu.current_tx_num*20;
				}
				
				if(NRF_SUCCESS != ble_dfus_c_write_data(&m_ble_dfus_c,&m_dfu.firmware[m_dfu.current_tx_num*20],tx_len))
				{
					ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_DATA,DFU_ERROR_WRITE);
					DfuState = DFU_STATE_IDLE;
					NRF_LOG_INFO("> Write firmware error\r\n");
					break;
				}	
				
				m_dfu.pack_count++;        /*发送总包数递增1*/
				m_dfu.current_tx_num++;    /*当前分包 发送包数递增1*/
				
				if(((m_dfu.current_tx_num - 1)*20 + tx_len) == m_dfu.curr_firm_len)  /*分包发送完毕*/
				{
					m_dfu.startflag = DFU_FIR_TX_END;  
					ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_DATA,0x01);
					
					NRF_LOG_INFO("> Transfer %d bytes\r\n",(m_dfu.pack_count - 1)*20 + tx_len);
					
					if(m_dfu.pack_count == m_dfu.pack_num && (m_dfu.pack_count - 1)*20 + tx_len == m_dfu.firm_size)
					{
						link_time = 0;
						DfuState = DFU_STATE_WAIT_TRAN;
						NRF_LOG_INFO("> Transfer over,wait transfer response\r\n");
					}
				}
			}
			break;
		
		case DFU_STATE_WAIT_TRAN:
			if(link_time > 15)
			{
				NRF_LOG_INFO("> Wait transfer data out time\r\n");
				DfuState = DFU_STATE_IDLE;
				break;
			}
			
			if(m_dfu.respon_flag == DFU_WRITE_RESPON)
			{
				if(m_dfu.respon[1] != 0x03 || m_dfu.respon[2] != 0x01)
				{
					NRF_LOG_INFO("> Transfer data not right response\r\n");
					DfuState = DFU_STATE_IDLE;
				}
				else
				{
					NRF_LOG_INFO("> Transfer firmware over!\r\n");
          DfuState = DFU_STATE_CHECK;					
				}
			}			
			break;
		
		case DFU_STATE_CHECK:
			link_time = 0;
		  m_dfu.respon_flag = DFU_WRITE_NO_RESPON;
			if(NRF_SUCCESS != ble_dfus_c_write_cmd(&m_ble_dfus_c,cmd_check_firmware,1))
			{
				DfuState = DFU_STATE_IDLE;
				NRF_LOG_INFO("> Write check firmware cmd error\r\n");
				break;
			}			  
			DfuState = DFU_STATE_WAIT_CHECK;					
			break;

		case DFU_STATE_WAIT_CHECK:
			if(link_time > 15)
			{
				NRF_LOG_INFO("> Wait check firmware out time\r\n");
				DfuState = DFU_STATE_IDLE;
				break;
			}
			
			if(m_dfu.respon_flag == DFU_WRITE_RESPON)
			{
				if(m_dfu.respon[1] != 0x04 || m_dfu.respon[2] != 0x01)
				{
					NRF_LOG_INFO("> Check firmware not right response\r\n");
					ble_dfu_bracelet_ackResponse(m_target_periph_addr.addr,DFU_STEPS_DATA,((m_dfu.respon[2] == 0x01)?DFU_ERROR_RESPON:m_dfu.respon[2]));
					DfuState = DFU_STATE_IDLE;
				}
				else
				{
					NRF_LOG_INFO("> Check firmware ok!\r\n");
          DfuState = DFU_STATE_END_DFU;					
				}
			}			
			break;
		
		case DFU_STATE_END_DFU:
			if(NRF_SUCCESS == ble_dfus_c_write_cmd(&m_ble_dfus_c,cmd_end_dfu,1))
			{
				NRF_LOG_INFO("> Dfu firmware ok!\r\n");
			}
		  DfuState = DFU_STATE_IDLE;					
			break;
		
		case DFU_STATE_IDLE:
			break;
			
		default:break;
	}
}