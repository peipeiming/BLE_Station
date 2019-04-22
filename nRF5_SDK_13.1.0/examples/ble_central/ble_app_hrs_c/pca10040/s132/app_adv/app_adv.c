#include <string.h>
#include "app_adv.h"

static ADV_T adv;
static uint8_t adv_rxbuf[ADV_RX_BUF_SIZE];		/* 接收缓冲区 */

/*
*********************************************************************************************************
 * 函数名：adv_init
 * 描述  ：保存GAP数据的FIFO初始化：设置写FIFO地址指针、读FIFO地址指针、FIFO大小等参数
 * 输入  ：无
 * 返回  : 无
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
 * 函数名：adv_writebuf
 * 描述  ：广播数据保存缓存
 * 输入  ：p，待存储的数据
 *       : len，待存储数据长度
 * 返回  : 无
*********************************************************************************************************
 */
void adv_writebuf(uint8_t *p,uint8_t len)
{
	uint8_t i;
	for(i=0;i<len;i++)
	{
		/*如果待读取的数据总数小于FIFO的大小,则写入*/
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
 * 函数名：adv_readbuf
 * 描述  ：广播缓存数据保存
 * 输入  ：*p,信息机发出包含获取目标识别器广播包的识别器ID
 * 返回  : 无
**********************************************************************************************************
 */
void adv_readbuf(uint8_t *p)
{
	uint16_t i;
	
	if(0 != memcmp(&p[4],station_addr,8))
	{
		return;
	}
	
	/*待读取的数据大于256字节，则每次发送256字节，保证总线不长时间被占用*/
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


