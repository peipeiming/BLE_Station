#ifndef APP_ADV_H
#define APP_ADV_H

#include "stdint.h"
#include "app_uart.h"
#include "identificator.h"

#define ADV_MAX_TX_COUNT  210
#define ADV_RX_BUF_SIZE		20*1024
	
/* GAP FIFO 结构体 */
typedef struct
{
	uint8_t *pRxBuf;			     /* 接收缓冲区 */
	volatile uint16_t usRxBufSize;		   /* 接收缓冲区大小 */
	
	volatile uint16_t usRxWrite;	 /* 接收缓冲区写指针 */
	volatile uint16_t usRxRead;		 /* 接收缓冲区读指针 */
	volatile uint16_t usRxCount;	 /* 还未读取的新数据个数 */

}ADV_T;

extern ADV_T adv_fifo;

void adv_init(void);
void adv_readbuf(uint8_t *p);
void adv_writebuf(uint8_t *p,uint8_t len);

#endif

