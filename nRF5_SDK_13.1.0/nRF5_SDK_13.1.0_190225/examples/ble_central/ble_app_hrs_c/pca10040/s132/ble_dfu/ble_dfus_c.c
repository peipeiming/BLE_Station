#include "includes.h"

uint8_t last_id = 0;

ble_dfus_c_t m_ble_dfus_c;

void ble_dfus_c_on_db_disc_evt(ble_dfus_c_t * p_ble_dfus_c, ble_db_discovery_evt_t * p_evt)
{
    ble_dfus_c_evt_t dfus_c_evt;
    memset(&dfus_c_evt,0,sizeof(ble_dfus_c_evt_t));

    ble_gatt_db_char_t * p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the dfus was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_DFUS_SERVICE &&
        p_evt->params.discovered_db.srv_uuid.type == p_ble_dfus_c->uuid_type)
    {

        uint32_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_chars[i].characteristic.uuid.uuid)
            {
                case BLE_UUID_DFUS_DP_CHARACTERISTIC:
                    dfus_c_evt.handles.dfus_dp_handle = p_chars[i].characteristic.handle_value;
//								    printf("dfus_dp_handle:%04x\r\n",p_chars[i].characteristic.handle_value);
//								    printf("dfus_dp_cccd_handle:%04x\r\n",p_chars[i].cccd_handle);
                    break;

                case BLE_UUID_DFUS_DCP_CHARACTERISTIC:
                    dfus_c_evt.handles.dfus_dcp_handle      = p_chars[i].characteristic.handle_value;
                    dfus_c_evt.handles.dfus_dcp_cccd_handle = p_chars[i].cccd_handle;
                  							
//										printf("dfus_dcp_handle:%04x\r\n",p_chars[i].characteristic.handle_value);
//								    printf("dfus_dcp_cccd_handle:%04x\r\n",p_chars[i].cccd_handle);
                    break;

                default:
                    break;
            }
        }
        if (p_ble_dfus_c->evt_handler != NULL)
        {
					  DfuState = DFU_STATE_LINK_TAG_OK;
            dfus_c_evt.conn_handle = p_evt->conn_handle;
            dfus_c_evt.evt_type    = BLE_DFUS_C_EVT_DISCOVERY_COMPLETE;
            p_ble_dfus_c->evt_handler(p_ble_dfus_c, &dfus_c_evt);
        }
    }
}

static void on_hvx(ble_dfus_c_t * p_ble_dfus_c, const ble_evt_t * p_ble_evt)
{
    // HVX can only occur from client sending.
    if ((p_ble_dfus_c->handles.dfus_dcp_handle != BLE_GATT_HANDLE_INVALID)
            && (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_dfus_c->handles.dfus_dcp_handle)
            && (p_ble_dfus_c->evt_handler != NULL)
       )
    {
        ble_dfus_c_evt_t ble_dfus_c_evt;

        ble_dfus_c_evt.evt_type = BLE_DFUS_C_EVT_DFUS_DCP_EVT;
        ble_dfus_c_evt.p_data   = (uint8_t *)p_ble_evt->evt.gattc_evt.params.hvx.data;
        ble_dfus_c_evt.data_len = p_ble_evt->evt.gattc_evt.params.hvx.len;

        p_ble_dfus_c->evt_handler(p_ble_dfus_c, &ble_dfus_c_evt);
    }
}

static uint32_t ble_dfus_c_init(ble_dfus_c_t * p_ble_dfus_c, ble_dfus_c_init_t * p_ble_dfus_c_init)
{
    uint32_t      err_code;
    ble_uuid_t    uart_uuid;
    ble_uuid128_t dfus_base_uuid = DFUS_BASE_UUID;

    VERIFY_PARAM_NOT_NULL(p_ble_dfus_c);
    VERIFY_PARAM_NOT_NULL(p_ble_dfus_c_init);

    err_code = sd_ble_uuid_vs_add(&dfus_base_uuid, &p_ble_dfus_c->uuid_type);
    VERIFY_SUCCESS(err_code);

    uart_uuid.type = p_ble_dfus_c->uuid_type;
    uart_uuid.uuid = BLE_UUID_DFUS_SERVICE;

    p_ble_dfus_c->conn_handle             = BLE_CONN_HANDLE_INVALID;
    p_ble_dfus_c->evt_handler             = p_ble_dfus_c_init->evt_handler;
    p_ble_dfus_c->handles.dfus_dcp_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_dfus_c->handles.dfus_dp_handle  = BLE_GATT_HANDLE_INVALID;

    return ble_db_discovery_evt_register(&uart_uuid);
}

static uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable)
{
    uint8_t buf[BLE_CCCD_VALUE_LEN];

    buf[0] = enable ? BLE_GATT_HVX_NOTIFICATION : 0;
    buf[1] = 0;

    const ble_gattc_write_params_t write_params = {
        .write_op = BLE_GATT_OP_WRITE_CMD,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = cccd_handle,
        .offset   = 0,
        .len      = sizeof(buf),
        .p_value  = buf
    };

    return sd_ble_gattc_write(conn_handle, &write_params);
}

static uint32_t ble_dfus_c_notif_enable(ble_dfus_c_t * p_ble_dfus_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_dfus_c);

    if ( (p_ble_dfus_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_dfus_c->handles.dfus_dcp_cccd_handle == BLE_GATT_HANDLE_INVALID)
       )
    {
        return NRF_ERROR_INVALID_STATE;
    }
		
		
    return cccd_configure(p_ble_dfus_c->conn_handle,p_ble_dfus_c->handles.dfus_dcp_cccd_handle, true);
}

static uint32_t ble_dfus_c_handles_assign(ble_dfus_c_t * p_ble_dfus,
                                  const uint16_t conn_handle,
                                  const ble_dfus_c_handles_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_dfus);

    p_ble_dfus->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_dfus->handles.dfus_dcp_cccd_handle = p_peer_handles->dfus_dcp_cccd_handle;
        p_ble_dfus->handles.dfus_dcp_handle      = p_peer_handles->dfus_dcp_handle;
        p_ble_dfus->handles.dfus_dp_handle      = p_peer_handles->dfus_dp_handle;
    }
    return NRF_SUCCESS;
}

static void ble_dfus_c_evt_handler(ble_dfus_c_t * p_ble_dfus_c, const ble_dfus_c_evt_t * p_ble_dfus_evt)
{
    ret_code_t err_code;

    switch (p_ble_dfus_evt->evt_type)
    {
        case BLE_DFUS_C_EVT_DISCOVERY_COMPLETE:
            //printf("Discovery complete.\r\n");
            err_code = ble_dfus_c_handles_assign(p_ble_dfus_c, p_ble_dfus_evt->conn_handle, &p_ble_dfus_evt->handles);
            APP_ERROR_CHECK(err_code);

            err_code = ble_dfus_c_notif_enable(p_ble_dfus_c);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_DFUS_C_EVT_DFUS_DCP_EVT:
					  memcpy(m_dfu.respon,p_ble_dfus_evt->p_data,p_ble_dfus_evt->data_len);
				    m_dfu.respon_flag = DFU_WRITE_RESPON;
				    NRF_LOG_INFO("> notification:%02X %02X %02X\r\n",p_ble_dfus_evt->p_data[0],p_ble_dfus_evt->p_data[1],p_ble_dfus_evt->p_data[2]);
            break;

        case BLE_DFUS_C_EVT_DISCONNECTED:
					
            break;
    }
}

void dfus_c_init(void)
{
    ret_code_t       err_code;
    ble_dfus_c_init_t init;

    init.evt_handler = ble_dfus_c_evt_handler;

    err_code = ble_dfus_c_init(&m_ble_dfus_c, &init);
    APP_ERROR_CHECK(err_code);
}

void ble_dfus_c_on_ble_evt(ble_dfus_c_t * p_ble_dfus_c, const ble_evt_t * p_ble_evt)
{	
    if ((p_ble_dfus_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    if ( (p_ble_dfus_c->conn_handle != BLE_CONN_HANDLE_INVALID)
       &&(p_ble_dfus_c->conn_handle != p_ble_evt->evt.gap_evt.conn_handle)
       )
    {
        return;
    }
		
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
					  //printf("hvx evt\r\n");
            on_hvx(p_ble_dfus_c, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            if (p_ble_evt->evt.gap_evt.conn_handle == p_ble_dfus_c->conn_handle
                    && p_ble_dfus_c->evt_handler != NULL)
            {
                ble_dfus_c_evt_t dfus_c_evt;

                dfus_c_evt.evt_type = BLE_DFUS_C_EVT_DISCONNECTED;

                p_ble_dfus_c->conn_handle = BLE_CONN_HANDLE_INVALID;
                p_ble_dfus_c->evt_handler(p_ble_dfus_c, &dfus_c_evt);
            }
            break;
        
				case BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE:
					  //printf("BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE\r\n");
					  m_dfu.txcompleteflag = DFU_FIR_TX_COMPL;
					  break;
					
        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_dfus_c_write_data(ble_dfus_c_t * p_ble_dfus_c, uint8_t * data, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_dfus_c);

    if (length > BLE_DFUS_MAX_DATA_LEN)
    {
        //NRF_LOG_WARNING("Content too long.\r\n");
        return NRF_ERROR_INVALID_PARAM;
    }
    if (p_ble_dfus_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        //NRF_LOG_WARNING("Connection handle invalid.\r\n");
        return NRF_ERROR_INVALID_STATE;
    }

    ble_gattc_write_params_t const write_params = {
        .write_op = BLE_GATT_OP_WRITE_CMD,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = p_ble_dfus_c->handles.dfus_dp_handle,
        .offset   = 0,
        .len      = length,
        .p_value  = data
    };

    return sd_ble_gattc_write(p_ble_dfus_c->conn_handle, &write_params);
}

uint32_t ble_dfus_c_write_cmd(ble_dfus_c_t * p_ble_dfus_c, uint8_t * cmd, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_dfus_c);

    if (length > BLE_DFUS_MAX_DATA_LEN)
    {
        //NRF_LOG_WARNING("Content too long.\r\n");
        return NRF_ERROR_INVALID_PARAM;
    }
    if (p_ble_dfus_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        //NRF_LOG_WARNING("Connection handle invalid.\r\n");
        return NRF_ERROR_INVALID_STATE;
    }

    ble_gattc_write_params_t const write_params = {
        .write_op = BLE_GATT_OP_WRITE_REQ,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = p_ble_dfus_c->handles.dfus_dcp_handle,
        .offset   = 0,
        .len      = length,
        .p_value  = cmd
    };

    return sd_ble_gattc_write(p_ble_dfus_c->conn_handle, &write_params);
}