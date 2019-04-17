#ifndef BRACELET_DFU_H
#define BRACELET_DFU_H

#include "stdint.h"

#define DFU_WRITE_NO_RESPON          0x00
#define DFU_WRITE_RESPON             0x01

#define DFU_FIR_TX_CLEAR             0x00
#define DFU_FIR_TX_COMPL             0x01  

#define DFU_FIR_TX_START             0x00
#define DFU_FIR_TX_END               0x01 

#define DFU_ERROR_LINK               0x07   /*连接失败*/
#define DFU_ERROR_TAG                0x08   /*进入DFU模式失败*/
#define DFU_ERROR_WRITE              0x09   /*写入手环数据异常*/
#define DFU_ERROR_RESPON             0x0a   /*不正确的回执*/

typedef enum
{
	DFU_STATE_START = 1,
	DFU_STATE_LINK_BRA,
	DFU_STATE_LINK_BRA_OK,
	DFU_STATE_LINK_TAG_START,
	DFU_STATE_LINK_TAG,
	DFU_STATE_LINK_TAG_OK,
	DFU_STATE_VALIDATE_FIRMWARE,
	DFU_STATE_WAIT_VALIDATE,
	DFU_STATE_INIT_DFU,
	DFU_STATE_WAIT_INIT,
	DFU_STATE_TRAN_FIRMWARE_START,
	DFU_STATE_TRAN_FIRMWARE,
	DFU_STATE_WAIT_TRAN,
	DFU_STATE_CHECK,
	DFU_STATE_WAIT_CHECK,
	DFU_STATE_END_DFU,
	DFU_STATE_IDLE
}DFU_STATE;

typedef struct
{
	uint32_t firm_size;
	uint16_t pack_num;       /*固件总包数据*/
	uint16_t pack_count;     /*发送固件包计数*/
	uint16_t curr_firm_len;
  uint16_t current_tx_num;
	
	uint8_t  startflag;
	uint8_t  respon_flag;
	uint8_t  respon[20];
	uint8_t  txcompleteflag;
	uint8_t  dfu_init_key[14];
	uint8_t  firmware[1200];
}Dfu_Ctr_t;
	
extern Dfu_Ctr_t m_dfu;
extern DFU_STATE DfuState;

void Dfu_bracelet(void);

#endif