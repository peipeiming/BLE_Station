#ifndef APP_ADV_H
#define APP_ADV_H

#include "stdint.h"
#include "app_uart.h"
#include "identificator.h"

#define ADV_MAX_TX_COUNT  210
#define ADV_RX_BUF_SIZE		20*1024
	
/* GAP FIFO �ṹ�� */
typedef struct
{
	uint8_t *pRxBuf;			     /* ���ջ����� */
	volatile uint16_t usRxBufSize;		   /* ���ջ�������С */
	
	volatile uint16_t usRxWrite;	 /* ���ջ�����дָ�� */
	volatile uint16_t usRxRead;		 /* ���ջ�������ָ�� */
	volatile uint16_t usRxCount;	 /* ��δ��ȡ�������ݸ��� */

}ADV_T;

extern ADV_T adv_fifo;

void adv_init(void);
void adv_readbuf(uint8_t *p);
void adv_writebuf(uint8_t *p,uint8_t len);

#endif

