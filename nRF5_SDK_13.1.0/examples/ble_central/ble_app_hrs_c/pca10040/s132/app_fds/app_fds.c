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
 * ��������app_fds_evt_handler
 * ����  ��FDS�¼�������
 * ����  ��*p_fds_evt,fdsЭ��ջ�����¼��ṹ��
 * ����  : ��
**********************************************************************************************************
*/
static void app_fds_evt_handler(fds_evt_t const * const p_fds_evt)
{
    switch (p_fds_evt->id)
    {
        case FDS_EVT_INIT:         /*��ʼ���¼�*/
            if (p_fds_evt->result == FDS_SUCCESS)
            {
							  fds_init_flag = 1;
            }
            break;
				case FDS_EVT_WRITE:       /*д��¼�¼�*/
						if (p_fds_evt->result == FDS_SUCCESS)
						{
							  write_flag=1;
						}
						break;
						
			  case FDS_EVT_DEL_RECORD:  /*ɾ����¼�¼�*/
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
 * ��������app_fds_Init
 * ����  ��FDS��ʼ������,��Э��ջע���¼����;��ʼ��FDSģ��
 * ����  ����
 * ����  : 
 *       ��NRF_SUCCESS,�ɹ�  ������NRF_SUCCESS��ʧ��
**********************************************************************************************************
*/
ret_code_t app_fds_init (void)
{
		//ע��FDS�¼�������ڵ���fds_init()����֮ǰ��һ��Ҫ��ע��
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
 * ��������app_fds_FindRecord
 * ����  ��FDS���Ҽ�¼,����д����ļ�¼
 * ����  ����
 * ����  : 
 *       ��NRF_SUCCESS,�ɹ�  ������NRF_SUCCESS��ʧ��
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
 * ��������app_fds_FindAndDelete
 * ����  ��FDS���Ҳ�ɾ�����Ƽ�¼
 * ����  ����
 * ����  : 
 *       ��NRF_SUCCESS,�ɹ�  ������NRF_SUCCESS��ʧ��
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
 * ��������app_fds_ReadRecord
 * ����  ������¼
 * ����  ����
 * ����  : 
 *       ��NRF_SUCCESS,�ɹ�  ������NRF_SUCCESS��ʧ��
**********************************************************************************************************
*/
ret_code_t app_fds_ReadRecord(void)
{
		fds_flash_record_t  flash_record;
		fds_record_desc_t   record_desc;
		fds_find_token_t    ftok ={0};   //��ʼ��Ϊ0����ͷ��ʼ���ҡ�
		uint32_t err_code;
		
		//ʹ�ø����ļ�¼key���Ҹ����ļ��е�����ƥ��ļ�¼
		while (fds_record_find(FILE_ID, REC_KEY, &record_desc, &ftok) == FDS_SUCCESS)
		{
				//��֮ǰҪ�ȴ򿪼�¼
			  err_code = fds_record_open(&record_desc, &flash_record);
				if ( err_code != FDS_SUCCESS)
				{
					printf("error in read!\r\n");
					return err_code;		
				}
				
				/*��ȡ��¼����*/				
				mymac = (uint32_t *)flash_record.p_data;    
				
				#if MYFDS_DEBUG
				printf("Found Record ID = %d\r\n",record_desc.record_id);
				#endif
				
				// �����һ���ǵùرռ�¼
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
 * ��������app_fds_ReadRecord
 * ����  ��д��¼
 * ����  ����
 * ����  : 
 *       ��NRF_SUCCESS,�ɹ�  ������NRF_SUCCESS��ʧ��
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
				
		//д������ݣ�����2�����֣���������ָ��ָ���д������ݣ��������ݳ���
		record_chunk.p_data         = dat;
		record_chunk.length_words   = 2;
		
		//д��ļ�¼����Ҫ�����ļ�ID����¼key�����ݿ�
		record.file_id              = FILE_ID;
		record.key              		= REC_KEY;
		record.data.p_chunks        = &record_chunk;
		record.data.num_chunks      = 1;
		
		/*д���¼*/
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
 * ��������app_fds_writedeviceaddress
 * ����  ��д�붼ͷ�豸8Byte ID��ַ 
 * ����  ��addr��д���ַ
 * ����  : 
 *       ��NRF_SUCCESS,�ɹ�  NRF_ERROR_BUSY��ʧ��
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
		
	  //д��¼ǰɾ�����Ƶļ�¼ID
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
	  //�ȴ�д������� 
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

