#include "nrf_log.h"
#include "datadeal.h"
#include "app_uart.h"
#include "peer_manager.h"

#include "includes.h"

extern uint8_t Backbuf[12];
extern ble_rscs_c_t   m_ble_rscs_c;  

/**@brief Battery level Collector Handler.
 */
//void bas_c_evt_handler(ble_bas_c_t * p_bas_c, ble_bas_c_evt_t * p_bas_c_evt)
//{
//    ret_code_t err_code;

//    switch (p_bas_c_evt->evt_type)
//    {
//        case BLE_BAS_C_EVT_DISCOVERY_COMPLETE:
//        {
//            err_code = ble_bas_c_handles_assign(p_bas_c,
//                                                p_bas_c_evt->conn_handle,
//                                                &p_bas_c_evt->params.bas_db);
//            APP_ERROR_CHECK(err_code);

//            // Initiate bonding.
//            err_code = pm_conn_secure(p_bas_c_evt->conn_handle, false);
//            if (err_code != NRF_ERROR_INVALID_STATE)
//            {
//                APP_ERROR_CHECK(err_code);
//            }

//						#if 0
//            // Batttery service discovered. Enable notification of Battery Level.
//            NRF_LOG_DEBUG("Battery Service discovered. Reading battery level.\r\n");

//            err_code = ble_bas_c_bl_read(p_bas_c);
//            APP_ERROR_CHECK(err_code);

//            NRF_LOG_DEBUG("Enabling Battery Level Notification.\r\n");
//            err_code = ble_bas_c_bl_notif_enable(p_bas_c);
//            APP_ERROR_CHECK(err_code);
//						#endif

//        } break;

//        case BLE_BAS_C_EVT_BATT_NOTIFICATION:
//            NRF_LOG_INFO("Battery Level received %d %%.\r\n", p_bas_c_evt->params.battery_level);
//            break;

//        case BLE_BAS_C_EVT_BATT_READ_RESP:
//            printf("Battery Level Read as %d %%.\r\n", p_bas_c_evt->params.battery_level);
//            break;

//        default:
//            break;
//    }
//}

/**@brief Runnign Speed and Cadence Collector Handler.
 */
void rscs_c_evt_handler(ble_rscs_c_t * p_rscs_c, ble_rscs_c_evt_t * p_rscs_c_evt)
{
	  uint8_t i;
    uint32_t err_code;
	 
    switch (p_rscs_c_evt->evt_type)
    {
			case BLE_RSCS_C_EVT_DISCOVERY_COMPLETE:
			{
				err_code = ble_rscs_c_handles_assign(p_rscs_c,
																		p_rscs_c_evt->conn_handle,
																		&p_rscs_c_evt->params.rscs_db);
				
				m_ble_rscs_c.conn_handle=p_rscs_c_evt->conn_handle;
				
				// Initiate bonding.
//				err_code = pm_conn_secure(p_rscs_c_evt->conn_handle, false);
//				if (err_code != NRF_ERROR_INVALID_STATE)
//				{
//						APP_ERROR_CHECK(err_code);
//				}	
//				APP_ERROR_CHECK(err_code);
				
				if(DfuState == DFU_STATE_IDLE)    
				{
					for(i=0;i<20;i++)
					{
						app_uart_put(Backbuf[i]);
					}
				}
					
#if 0				
				printf("Enabling Notification.\r\n");
				err_code = ble_rscs_c_rsc_notif_enable(p_rscs_c);   //取消心率使能通知
				APP_ERROR_CHECK(err_code);
				printf("Running Speed and Cadence Service discovered.\r\n");
#endif
				
			}break;
			
			case BLE_RSCS_C_EVT_RSC_NOTIFICATION:
			{
#if 0
				 printf("notification evnt data:");
				 for(i=0;i<20;i++)
				  printf("%x ",p_rscs_c_evt->params.databuffer[i]);
				 printf("\r\n");
         Motion_parameter_deal(p_rscs_c_evt);
#endif       
			}break;
			
			case BLE_RSCS_C_EVT_MOTION_PARAMETER_READ_RESP:                       //读响应事件
			{
	   #if 0
				printf("read evnt data:");
				for(i=0;i<18;i++)
				  printf("%x ",p_rscs_c_evt->params.databuffer[i]);
				printf("\r\n");
		 #endif
			}break;
			
      default:
        break;
    }
}

