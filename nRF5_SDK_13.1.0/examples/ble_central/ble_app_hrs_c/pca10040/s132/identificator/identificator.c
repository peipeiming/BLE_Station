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
 * 函数名：app_ble_diconnect
 * 描述  ：断开蓝牙基站蓝牙连接
 * 输入  ：*p,主控控制数据
 * 返回  : 无
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
 * 函数名：app_ble_connectbracelet
 * 描述  ：连接蓝牙手环
 * 输入  ：*p,主控控制数据
 * 返回  : 无
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
	
	link_time = 0;               /*开始计时连接时间*/
	is_connect_per_addr = true;
	memcpy(m_target_periph_addr.addr,&p[12],6);
}

/*断开当前蓝牙连接,用于扫描广播模式下检查蓝牙连接*/
void disconnect_ble_link(void)  
{
	uint32_t err_code;	
	
	if(m_pending_db_disc_conn == BLE_CONN_HANDLE_INVALID)
	{
		return;
	}	
	
	if(advsatrt==1) /*关闭广播*/
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
 * 函数名：MiBand_info_save
 * 描述  ：小米手环数据处理上报
 * 输入  ：*m_gap_evt_t,广播信息结构体
 * 返回  : 无
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
  
	memcpy((uint8_t *)&total_steps,&m_gap_evt_t->params.adv_report.data[19],2);   /*计步*/
	total_calorie = total_steps*35.0*0.0006564;  /*卡路里*/
	total_distance = total_steps*140*0.45*0.01;  /*总距离*/
	electricity = m_gap_evt_t->params.adv_report.data[24]; /*手环电量*/
	
	memset(&gap_info[25],0xaa,4); 							 /*定位数据空*/
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
 * 函数名：gap_info_save
 * 描述  ：保存广播包数据
 * 输入  ：*m_gap_evt_t,广播信息结构体
 * 返回  : 无
**********************************************************************************************************
 */
void gap_info_save(ble_gap_evt_t const *m_gap_evt_t)              	 
{
	uint8_t i = 0;
	uint8_t check_results = 0;
	uint8_t gap_info[41]={0x5a,0x41,0x00,0x23};
	
	memcpy(&gap_info[4],station_addr,8);
	memcpy(&gap_info[12],m_gap_evt_t->params.adv_report.peer_addr.addr,6);
	
	gap_info[18] = -m_gap_evt_t->params.adv_report.rssi;              //RSSI值写入

	for(i=0;i<m_gap_evt_t->params.adv_report.dlen;i++)                //广播包数据  
  {	
	  if(0xFF == m_gap_evt_t->params.adv_report.data[i])
	  {
			memcpy(&gap_info[19],&m_gap_evt_t->params.adv_report.data[i+3],20);
		  break;
	  }
	}
	
  check_results=gap_info[1];
	for(i=2;i<(gap_info[3]+2+3-1);i++)     						                 //CMD字段到数据字段进行校验
	{
		check_results^=gap_info[i];
	}
	
  gap_info[i]=check_results;                                         //校验值
	gap_info[i+1]=0xca;                                                //结束符
	
	adv_writebuf(gap_info,sizeof(gap_info));
}
/*
*********************************************************************************************************
 * 函数名：app_ble_messagesend
 * 描述  ：向手环发送信息
 * 输入  ：*p,信息机发出包含连接目标手环的MAC地址和信息
 * 返回  : 无
**********************************************************************************************************
 */
void static app_ble_messagesend(uint8_t *p)
{
	uint32_t err;

	char switchtable[10]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};
  
	if(!connect_flag)
	{
		return;   /*未连接状态返回*/
	}
	
	if(0 != memcmp(&p[4],station_addr,8))
	{
		return;
	}
	
	switchtable[3]=0x01;
	err=ble_rscs_string_send(&m_ble_rscs_c,switchtable,m_ble_rscs_c.message_push_switch_handle);     //使能消息接收   
	if(err!=NRF_SUCCESS)
	{
		return;
	}
	
	nrf_messend.txcount=0;	
	nrf_messend.txstartflag=1;
	nrf_messend.txcompleteflag=1;
	nrf_messend.txlen=p[2]*256+p[3]-8;   								   /*消息长度*/
  memcpy(nrf_messend.txfifo,&p[12],nrf_messend.txlen); /*拷贝消息*/ 
	nrf_messend.txtotalcount=nrf_messend.txlen/17;
	if(nrf_messend.txlen%17!=0)
	{
		nrf_messend.txtotalcount++;
	}
}							

/*
*********************************************************************************************************
 * 函数名：app_ble_settime
 * 描述  ：校准蓝牙基站网络时间
 * 输入  ：*p,时间参数
 * 返回  : 无
**********************************************************************************************************
 */
void static app_ble_settime(uint8_t *p)
{
	uint16_t year;
	
	year = p[4];
	year += 2000;
  RTC_Set(year,p[5],p[6],p[7],p[8],p[9]);    /*设置RTC时间*/
}

/*
*********************************************************************************************************
 * 函数名：app_ble_register
 * 描述  ：设备注册
 * 输入  ：*p,注册请求
 * 返回  : 无
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
		for(uint8_t i = 2; i < (reqdata[3] + 4); i++)   //CMD字段到数据字段进行校验
		{
			check_results^=reqdata[i];
		}
		reqdata[12] = check_results;     //校验值
		
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
 * 函数名：app_ble_bootloaderenter
 * 描述  ：进入串口升级boot
 * 输入  ：*p,数据缓存指针
 * 返回  : 无
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
函数名：app_ble_rssiset
描述：设置手环扫描信标B1过滤值
参数：*p：数据缓存指针
返回：无
***************************************************/
void static app_ble_rssiset(uint8_t *p)
{
	if(p[11] >= 50 && p[11] <= 100)
	{
		gap_data[6] = p[11];
	}
}

/***************************************************
函数名：app_ble_updatabracelet
描述：升级蓝牙手环
参数：*p：数据缓存指针
返回：无
***************************************************/
void static app_ble_updatabracelet(uint8_t *p)
{
	uint8_t target_periph_addr[6] = {0x00}; 
	
  if(p[4] == 0x01 && DfuState == DFU_STATE_IDLE)        /*开始升级指令*/
	{
		DfuState = DFU_STATE_START;
	   		
		sd_ble_gap_disconnect( m_pending_db_disc_conn , BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		
		is_connect_per_addr = true;
		memcpy(m_target_periph_addr.addr,&p[5],6);
		
		memcpy((uint8_t *)&m_dfu.firm_size,&p[11],4);       /*固件长度*/
		memcpy((uint8_t *)&m_dfu.dfu_init_key,&p[15],14);   /*固件长度*/
		
		/*计算升级固件总包数*/
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
		m_dfu.startflag = DFU_FIR_TX_START;    /*开始固件传输*/
		
		NRF_LOG_INFO("> Receive firmware data. len:%d\r\n",m_dfu.curr_firm_len);
	}
}

/***************************************************
函数名：uart_data_deal
描述：串口数据处理
参数：*p：数据缓存指针
返回：无
***************************************************/
void uart_data_deal(uint8_t *buff,uint16_t buff_len)
{
	switch(buff[1])
	{
		case DISCONNECTBLE:
			app_ble_diconnect(buff);     		  //断开连接
			break;
		
		case GETBLEDATA:
			adv_readbuf(buff);         		    //获取广播包数据
			break;
		
		case CONNECTBLE:
			app_ble_connectbracelet(buff);    //连接目标
			break;

		case SEND_MESSAGE:
			app_ble_messagesend(buff);        //发送消息        	   
	    break;
		
	  case SET_TIME:		                  //校准基站时间
			app_ble_settime(buff);
			break;

	  case GET_STATION:		                //注册
			app_ble_register(buff);
			break;
		
	  case RESET:		                      //复位机制
			app_ble_reset(buff);
			break;

		case UPDATA_STATION:                //串口升级进入boot
			app_ble_bootloaderenter(buff);
			break;
		
		case SET_RSSI:                      //设置手环扫描信标RSSI
			app_ble_rssiset(buff);
			break;
		
		case UPDATA_BRACELET:               //升级手环固件
			app_ble_updatabracelet(buff);
			break;
		
		default:break;
	}
}




