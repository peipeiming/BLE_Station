#ifndef APP_FDS_H
#define APP_FDS_H

#include "fds.h"
#include "fstorage.h"

#ifndef MYFDS_DEBUG
#define MYFDS_DEBUG 0
#endif

#define FILE_ID     0x1000
#define REC_KEY     0x2000

extern uint32_t *mymac;

extern volatile uint8_t write_flag;
extern volatile uint8_t fds_init_flag;
extern volatile uint8_t uart_send_fifo_flag;

typedef union
{ 
	uint8_t mac_8Byte[8];
	uint32_t mac_2word[2];
}devmac;



ret_code_t app_fds_init (void);
ret_code_t app_fds_ReadRecord(void);
ret_code_t app_fds_FindRecord (void);
ret_code_t app_fds_FindAndDelete (void);
ret_code_t app_fds_WriteDeviceAddress(uint8_t *addr);

#endif

