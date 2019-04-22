#ifndef IDENTIFICATOR_H
#define IDENTIFICATOR_H

#include "stdint.h"
#include "ble_gap.h"
#include "mble_dfu.h"
#include "ble_bas_c.h"
#include "ble_rscs_c.h"

#define  REGISTER                        0x01
#define  UNREGISTER                      0x00

#define  DFU_BRACELET_SUCCESS            

/**********************************自定义宏***************************************/
#define  DISCONNECTBLE                   0x40        //断开BLE连接Msg_id
#define  GETBLEDATA                      0x41        //获取手环数据Msg_id
#define  CONNECTBLE                      0x42        //连接手环Msg_id
#define  SEND_MESSAGE                    0x43        //发送消息
#define  GET_STATION                     0x44        //获取基站
#define  RESET                           0x45        //复位基站
#define  UPDATA_STATION                  0x46        //升级蓝牙
#define  SET_TIME                        0x47        //设置RTC时间
#define  SET_RSSI                        0x48        //设置手环扫描信标过滤值
#define  UPDATA_BRACELET  							 0x49        //升级蓝牙手环
/*********************************************************************************/

typedef struct
{	
	uint8_t txlen;
	uint8_t txcount;
	uint8_t txtotalcount;
	uint8_t txfifo[512];
	uint8_t txstartflag;
  uint8_t txcompleteflag;
}app_messend;


extern volatile uint32_t link_time;
extern app_messend nrf_messend;

extern uint16_t  m_conn_handle; 
extern uint16_t m_pending_db_disc_conn;
extern ble_bas_c_t   m_ble_bas_c; 
extern ble_rscs_c_t  m_ble_rscs_c;

extern volatile bool do_connect;
extern volatile bool connect_flag;

extern uint8_t dfu_bracelet_err_code;

extern uint8_t station_addr[8];
extern const char m_target_periph_name[]; 
extern volatile bool is_connect_per_addr; 
extern ble_gap_addr_t m_target_periph_addr;

void disconnect_ble_link(void);
void gap_info_save(ble_gap_evt_t const *m_gap_evt_t);
void uart_data_deal(uint8_t *buff,uint16_t buff_len);
void MiBand_info_save(ble_gap_evt_t const *m_gap_evt_t);

#endif

