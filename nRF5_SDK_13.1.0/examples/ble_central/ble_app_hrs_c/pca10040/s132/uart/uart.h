#ifndef UART_H
#define UART_H

#ifndef  APP_DEBUG
#define  APP_DEBUG 0
#endif

#define MAX_BLE_RESPONSE_LEN            (RX_BUF_SIZE - 6)

#define RX_BUF_SIZE                     1024

#define UART_TX_BUF_SIZE                256         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256         /**< UART RX buffer size. */

typedef enum 
{
	UART_DATA_STATE_START = 0, 
	UART_DATA_STATE_LEN,
	UART_DATA_STATE_DATA,
	UART_DATA_STATE_OK,
	UART_DATA_STATE_IDL
}uart_data_state;

void uart_init(void);

#endif



