#include "uart.h"
#include "strdeal.h"
#include "app_uart.h"
#include "pca10040.h"
#include "ble_rscs_c.h"

#include "includes.h"

uart_data_state nState = UART_DATA_STATE_IDL;
uint8_t data_buff[RX_BUF_SIZE];
extern ble_rscs_c_t           m_ble_rscs_c;   


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' i.e '\r\n' (hex 0x0D) or if the string has reached a length of
 *          @ref NUS_MAX_DATA_LENGTH.
 */
void uart_event_handle(app_uart_evt_t * p_event)
{
    uint8_t ch;
	  uint8_t check_results = 0;
    static uint16_t index = 0;
	
    switch (p_event->evt_type)
    {
        /**@snippet [Handling data from UART] */
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&ch));
				
				    if( ch == 0x5a && nState == UART_DATA_STATE_IDL)
						{
							index = 0;
							data_buff[index++] = ch;
							nState = UART_DATA_STATE_START;
						}
						else if(nState == UART_DATA_STATE_START)
						{
							data_buff[index++] = ch;
							//NRF_LOG_INFO("%02x ",data_buff[index - 1]);
							if(index == 4)
							{
								/*检查数据帧长度*/
								if(data_buff[2]*256 + data_buff[3] > MAX_BLE_RESPONSE_LEN)
								{
									nState = UART_DATA_STATE_IDL;
									NRF_LOG_INFO("> Uart data len error\r\n");
									return;
								}
								
								/*长度正常，准备接收数据区数据*/
							  nState = UART_DATA_STATE_DATA;
							}
						}
						else if(nState == UART_DATA_STATE_DATA)
						{
							data_buff[index++] = ch;
							if(index == data_buff[2]*256 + data_buff[3] + 6)
							{								
								/*校验数据*/
								check_results = data_buff[1];   
								for(uint16_t i = 2; i < data_buff[2]*256 + data_buff[3] + 4; i++)   //CMD字段到数据字段进行校验
								{
									check_results ^= data_buff[i];
								}
								if(check_results == data_buff[data_buff[2]*256 + data_buff[3] + 4])
                {
									uart_data_deal(data_buff,index);
								}
								else
								{
									NRF_LOG_INFO("> Uart check error\r\n");
								}
                nState = UART_DATA_STATE_IDL;								
							}
						}				    
            break;
					
				#if 0
        /**@snippet [Handling data from UART] */
        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;
								
        case APP_UART_FIFO_ERROR:
					
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;
        #endif

        default:
            break;
    }
}


/**@brief  Function for initializing the UART module.
 */
/**@snippet [UART Initialization] */
void uart_init(void)
{
    uint32_t                     err_code;
    const app_uart_comm_params_t comm_params =
    {
        .rx_pin_no    = RX_PIN_NUMBER,
        .tx_pin_no    = TX_PIN_NUMBER,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
        .baud_rate    = UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}

