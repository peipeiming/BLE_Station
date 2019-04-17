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

/**********************************�Զ����***************************************/
#define  DISCONNECTBLE                   0x40        //�Ͽ�BLE����Msg_id
#define  GETBLEDATA                      0x41        //��ȡ�ֻ�����Msg_id
#define  CONNECTBLE                      0x42        //�����ֻ�Msg_id
#define  SEND_MESSAGE                    0x43        //������Ϣ
#define  GET_STATION                     0x44        //��ȡ��վ
#define  RESET                           0x45        //��λ��վ
#define  UPDATA_STATION                  0x46        //��������
#define  SET_TIME                        0x47        //����RTCʱ��
#define  SET_RSSI                        0x48        //�����ֻ�ɨ���ű����ֵ
#define  UPDATA_BRACELET  							 0x49        //���������ֻ�
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

