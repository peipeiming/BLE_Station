#include "app_fds.h"
#include "stdio.h"
#include "string.h"

uint32_t *mymac;
		
volatile uint8_t write_flag = 0;
volatile uint8_t delete_flag = 0;
volatile uint8_t fds_init_flag = 0;
volatile uint8_t uart_send_fifo_flag = 0;


/*
*********************************************************************************************************
 * 函数名：app_fds_evt_handler
 * 描述  ：FDS事件处理函数
 * 输入  ：*p_fds_evt,fds协议栈上抛事件结构体
 * 返回  : 无
**********************************************************************************************************
*/
static void app_fds_evt_handler(fds_evt_t const * const p_fds_evt)
{
    switch (p_fds_evt->id)
    {
        case FDS_EVT_INIT:         /*初始化事件*/
            if (p_fds_evt->result == FDS_SUCCESS)
            {
							  fds_init_flag = 1;
            }
            break;
				case FDS_EVT_WRITE:       /*写记录事件*/
						if (p_fds_evt->result == FDS_SUCCESS)
						{
							  write_flag=1;
						}
						break;
						
			  case FDS_EVT_DEL_RECORD:  /*删除记录事件*/
						if (p_fds_evt->result == FDS_SUCCESS)
						{
							  delete_flag=1;
						}
						break;
						
        default:
            break;
    }
}

/*
*********************************************************************************************************
 * 函数名：app_fds_Init
 * 描述  ：FDS初始化函数,向协议栈注册事件句柄;初始化FDS模块
 * 输入  ：无
 * 返回  : 
 *       ：NRF_SUCCESS,成功  不等于NRF_SUCCESS则失败
**********************************************************************************************************
*/
ret_code_t app_fds_init (void)
{
		//注册FDS事件句柄，在调用fds_init()函数之前，一定要先注册
	  ret_code_t ret = fds_register(app_fds_evt_handler);
		if (ret != FDS_SUCCESS)
		{
				return ret;	
		}
		ret = fds_init();
		if (ret != FDS_SUCCESS)
		{
				return ret;
		}
		
		return NRF_SUCCESS;		
}


/*
*********************************************************************************************************
 * 函数名：app_fds_FindRecord
 * 描述  ：FDS查找记录,查找写入过的记录
 * 输入  ：无
 * 返回  : 
 *       ：NRF_SUCCESS,成功  不等于NRF_SUCCESS则失败
**********************************************************************************************************
*/
ret_code_t app_fds_FindRecord (void)
{
		fds_record_desc_t   record_desc;
		fds_find_token_t    ftok;
	
		ftok.page=0;
		ftok.p_addr=NULL;
	  if(fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok) == FDS_SUCCESS)
		{
				#if MYFDS_DEBUG
			  printf("Find record \r\n");
			  printf("ID: %d \r\n",record_desc.record_id);
				#endif
		}
		else
		{
			  #if MYFDS_DEBUG
			  printf("record is not finded  \r\n");
			  #endif
		}
	  #if MYFDS_DEBUG
		printf("\r\n");
		#endif
		return NRF_SUCCESS;
}

/*
*********************************************************************************************************
 * 函数名：app_fds_FindAndDelete
 * 描述  ：FDS查找并删除相似记录
 * 输入  ：无
 * 返回  : 
 *       ：NRF_SUCCESS,成功  不等于NRF_SUCCESS则失败
**********************************************************************************************************
*/
ret_code_t app_fds_FindAndDelete (void)
{
		fds_record_desc_t   record_desc;
		fds_find_token_t    ftok;
	
		ftok.page=0;
		ftok.p_addr=NULL;
		// Loop and find records with same ID and rec key and mark them as deleted. 
		while (fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok) == FDS_SUCCESS)
		{
			delete_flag=0;
			fds_record_delete(&record_desc);
			
			#if MYFDS_DEBUG
			printf("Deleted record ID: %d \r\n",record_desc.record_id);
			#endif
			
			while (delete_flag==0);
		}
		// call the garbage collector to empty them, don't need to do this all the time, this is just for demonstration
		ret_code_t ret = fds_gc();
		if (ret != FDS_SUCCESS)
		{
				return ret;
		}
		return NRF_SUCCESS;
}

/*
*********************************************************************************************************
 * 函数名：app_fds_ReadRecord
 * 描述  ：读记录
 * 输入  ：无
 * 返回  : 
 *       ：NRF_SUCCESS,成功  不等于NRF_SUCCESS则失败
**********************************************************************************************************
*/
ret_code_t app_fds_ReadRecord(void)
{
		fds_flash_record_t  flash_record;
		fds_record_desc_t   record_desc;
		fds_find_token_t    ftok ={0};   //初始化为0，从头开始查找。
		uint32_t err_code;
		
		//使用给定的记录key查找给定文件中的所有匹配的记录
		while (fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok) == FDS_SUCCESS)
		{
				//读之前要先打开记录
			  err_code = fds_record_open(&record_desc, &flash_record);
				if ( err_code != FDS_SUCCESS)
				{
					printf("error in read!\r\n");
					return err_code;		
				}
				
				/*获取记录数据*/				
				mymac = (uint32_t *)flash_record.p_data;    
				
				#if MYFDS_DEBUG
				printf("Found Record ID = %d\r\n",record_desc.record_id);
				#endif
				
				// 读完后，一定记得关闭记录
				err_code = fds_record_close(&record_desc);
				if (err_code != FDS_SUCCESS)
				{
					return err_code;	
				}
		}
		return NRF_SUCCESS;
}

/*
*********************************************************************************************************
 * 函数名：app_fds_ReadRecord
 * 描述  ：写记录
 * 输入  ：无
 * 返回  : 
 *       ：NRF_SUCCESS,成功  不等于NRF_SUCCESS则失败
**********************************************************************************************************
*/
static ret_code_t app_fds_WriteRecord(devmac mac)
{
		static uint32_t dat[3] = {0x00000000,0x00000000};
		
		fds_record_t        record;
		fds_record_desc_t   record_desc;
		fds_record_chunk_t  record_chunk;
			
		dat[0]=mac.mac_2word[0];
		dat[1]=mac.mac_2word[1];
				
		//写入的数据：包含2个部分，设置数据指针指向待写入的数据，设置数据长度
		record_chunk.p_data         = dat;
		record_chunk.length_words   = 2;
		
		//写入的记录：需要设置文件ID、记录key和数据块
		record.file_id              = FILE_ID;
		record.key              		= REC_KEY;
		record.data.p_chunks        = &record_chunk;
		record.data.num_chunks      = 1;
		
		/*写入记录*/
		ret_code_t ret = fds_record_write(&record_desc, &record);
		if (ret != FDS_SUCCESS)
		{
				return ret;
		}
		
		#if MYFDS_DEBUG
		printf("Writing Record ID = %d \r\n",record_desc.record_id);
		printf("\r\n");
		#endif
		
		return NRF_SUCCESS;
}

/*
*********************************************************************************************************
 * 函数名：app_fds_writedeviceaddress
 * 描述  ：写入都头设备8Byte ID地址 
 * 输入  ：addr，写入地址
 * 返回  : 
 *       ：NRF_SUCCESS,成功  NRF_ERROR_BUSY，失败
*********************************************************************************************************
 */
ret_code_t app_fds_WriteDeviceAddress(uint8_t *addr)
{
		uint8_t i;
		devmac mac;
		devmac writemac;
	  uint32_t err_code;
	  uint32_t *readmac;
		fds_record_desc_t   record_desc;
    fds_flash_record_t  flash_record;	
	  
		memcpy(writemac.mac_8Byte,addr,8);
		
		#if MYFDS_DEBUG
		printf("FDS delete record.\r\n");
		#endif
		
	  //写记录前删除相似的记录ID
	  err_code = app_fds_FindAndDelete();
		APP_ERROR_CHECK(err_code);
	
		#if MYFDS_DEBUG
		printf("FDS Write a record.\r\n");
		#endif
	  err_code = app_fds_WriteRecord(writemac);
		APP_ERROR_CHECK(err_code);
		
		if(err_code!=FDS_SUCCESS)
		{
		  printf("%x",err_code);
		}
	  //等待写操作完成 
		while (write_flag==0);
			
		#if MYFDS_DEBUG
		printf("FDS read a record.\r\n");
		#endif
		err_code = app_fds_ReadRecord();
		APP_ERROR_CHECK(err_code); 
		
		if(NRF_SUCCESS==err_code)
		{ 
			mac.mac_2word[0]=mymac[0];
			mac.mac_2word[1]=mymac[1];
			
			for(uint8_t i=0;i<8;i++)
			{
				if(mac.mac_8Byte[i]!=addr[i])
				{
					return NRF_ERROR_BUSY;
				}
			}
			
			#if 1
			printf("Set device MAC:\r\n");
			for(uint8_t i=0;i<8;i++)
			{
				printf("%2x ",mac.mac_8Byte[i]);
			}
			printf("\r\nSet device MAC success.\r\n");
			#endif
			
			return NRF_SUCCESS;
		}
		else
		{
			printf("%x",err_code);
		}
		
	  return NRF_ERROR_BUSY;
}

