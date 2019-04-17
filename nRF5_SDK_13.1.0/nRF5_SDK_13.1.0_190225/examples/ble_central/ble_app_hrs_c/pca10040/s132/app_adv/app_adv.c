#include <string.h>
#include "app_adv.h"

static ADV_T adv;
static uint8_t adv_rxbuf[ADV_RX_BUF_SIZE];		/* ���ջ����� */

/*
*********************************************************************************************************
 * ��������adv_init
 * ����  ������GAP���ݵ�FIFO��ʼ��������дFIFO��ַָ�롢��FIFO��ַָ�롢FIFO��С�Ȳ���
 * ����  ����
 * ����  : ��
*********************************************************************************************************
 */
void adv_init(void)
{
	adv.pRxBuf      = adv_rxbuf;
	adv.usRxBufSize = ADV_RX_BUF_SIZE;
	adv.usRxCount		= 0;
	adv.usRxRead		=	0;
	adv.usRxWrite		= 0;
}

/*
*********************************************************************************************************
 * ��������adv_writebuf
 * ����  ���㲥���ݱ��滺��
 * ����  ��p�����洢������
 *       : len�����洢���ݳ���
 * ����  : ��
*********************************************************************************************************
 */
void adv_writebuf(uint8_t *p,uint8_t len)
{
	uint8_t i;
	for(i=0;i<len;i++)
	{
		/*�������ȡ����������С��FIFO�Ĵ�С,��д��*/
		if (adv.usRxCount < adv.usRxBufSize)
		{
			adv.pRxBuf[adv.usRxWrite]=p[i];
			if (++adv.usRxWrite >= adv.usRxBufSize)
			{
				adv.usRxWrite = 0;
			}
			adv.usRxCount++;
		}
	}
}

/*
*********************************************************************************************************
 * ��������adv_readbuf
 * ����  ���㲥�������ݱ���
 * ����  ��*p,��Ϣ������������ȡĿ��ʶ�����㲥����ʶ����ID
 * ����  : ��
**********************************************************************************************************
 */
void adv_readbuf(uint8_t *p)
{
	uint16_t i;
	
	if(0 != memcmp(&p[4],station_addr,8))
	{
		return;
	}
	
	/*����ȡ�����ݴ���256�ֽڣ���ÿ�η���256�ֽڣ���֤���߲���ʱ�䱻ռ��*/
	if(adv.usRxCount > ADV_MAX_TX_COUNT)
	{
		for(i=0;i < ADV_MAX_TX_COUNT;i++)
		{
			app_uart_put(adv.pRxBuf[adv.usRxRead]);
			if (++adv.usRxRead >= adv.usRxBufSize)
			{
				adv.usRxRead=0;
			}
		}
		adv.usRxCount -= ADV_MAX_TX_COUNT;
  }
	else
  {
		for(i=0;i<adv.usRxCount;i++)
		{
			app_uart_put(adv.pRxBuf[adv.usRxRead]);
			if (++adv.usRxRead >= adv.usRxBufSize)
			{
				adv.usRxRead=0;
			}
		}
		adv.usRxCount=0;
	}
}


