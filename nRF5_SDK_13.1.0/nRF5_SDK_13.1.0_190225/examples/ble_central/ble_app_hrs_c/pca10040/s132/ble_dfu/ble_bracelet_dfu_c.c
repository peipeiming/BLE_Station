#include "app_uart.h"
#include "ble_types.h"

#include "includes.h"

ble_dfu_bracelet_c_t m_ble_bracelet_c;

void ble_dfu_bracelet_c_init(void)
{
		uint8_t   uuid_type;
    uint32_t  err_code;
	
    ble_uuid_t dfu_bracelet_uuid;

    ble_uuid128_t dfu_bracelet_base_uuid = DFU_BRACELET_BASE_UUID;
	
    err_code = sd_ble_uuid_vs_add(&dfu_bracelet_base_uuid, &uuid_type);
    APP_ERROR_CHECK(err_code);
	
    dfu_bracelet_uuid.type = uuid_type;
    dfu_bracelet_uuid.uuid = BLE_UUID_DFU_BRACELET_SERVICE;
	
	  err_code = ble_db_discovery_evt_register(&dfu_bracelet_uuid);
}

uint32_t ble_dfu_bracelet_start(void)
{ 
	uint32_t err_code;
	uint8_t p_sting[20] = {0x00};
	
	if ( m_ble_bracelet_c.conn_handle == BLE_CONN_HANDLE_INVALID)
	{
			return 1;
	}
		
	p_sting[0] = 0x7F;
	
	const ble_gattc_write_params_t write_params = {
			.write_op = BLE_GATT_OP_WRITE_CMD,
			.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
			.handle   = m_ble_bracelet_c.enter_dfu_handle,
			.offset   = 0,
			.len      = 1,
			.p_value  = (uint8_t *)p_sting
	};
	
	return sd_ble_gattc_write(m_ble_bracelet_c.conn_handle, &write_params); 
}

/*
*********************************************************************************************************
 * 函数名：ble_dfu_bracelet_ackResponse
 * 描述  ：升级蓝牙手环错误返回
 * 输入  ：*bracelet,手环MAC
 * 			 : steps,升级步骤
 * 			 : err_code,错误码
 * 返回  : 无
**********************************************************************************************************
 */
void ble_dfu_bracelet_ackResponse(uint8_t *bracelet, uint8_t steps, uint8_t err_code)
{
	uint8_t temp;
	uint8_t check_results = 0;
	uint8_t response[14] = {0x5a,0x49,0x00,0x08};
	
	uint8_t bra_temp[6] = {0};
	
	memcpy(bra_temp,bracelet,6);
	bra_temp[0] -= 1;
	
	for(uint8_t i = 0; i < 3; i++)
	{
		temp = bra_temp[i];
		bra_temp[i] = bra_temp[5-i];
		bra_temp[5-i] = temp;
	}		
	
	memcpy(&response[4],&steps,1);
	memcpy(&response[5],bra_temp,6);
	memcpy(&response[11],&err_code,1);
	
	/*校验数据*/
	check_results = response[1];  
	for(uint8_t i = 2; i < response[3] + 4; i++)  
	{
		check_results ^= response[i];
	}
	response[response[3]+4] = check_results;
	response[response[3]+5] = 0xca;
	
	
  /*返回回执数据*/
	for(uint8_t i = 0; i < sizeof(response); i++)  
	{
		app_uart_put(response[i]);
	}
}

void ble_dfu_bracelet_on_db_disc_evt(ble_dfu_bracelet_c_t * p_ble_bracelet_c, const ble_db_discovery_evt_t * p_evt)
{
	if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
			p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_DFU_BRACELET_SERVICE &&
			p_evt->params.discovered_db.srv_uuid.type == BLE_UUID_TYPE_BLE)
	{
		p_ble_bracelet_c->conn_handle = p_evt->conn_handle;
		
		for (uint8_t i = 0; i < p_evt->params.discovered_db.char_count; i++)
		{
			if(p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid == BLE_UUID_DFU_BRACELET_CHARACTERISTIC)
			{
				p_ble_bracelet_c->enter_dfu_handle = p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
				if(DfuState == DFU_STATE_LINK_BRA)
				{
					DfuState = DFU_STATE_LINK_BRA_OK;
				}
			}
		}
	}
}
