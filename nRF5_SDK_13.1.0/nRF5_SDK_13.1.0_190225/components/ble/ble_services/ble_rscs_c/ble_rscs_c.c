/*
 * Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

/**@cond To Make Doxygen skip documentation generation for this file.
 * @{
 */
#include "strdeal.h"
#include "stdio.h"
#include <string.h>
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_RSCS_C)
#include "ble_rscs_c.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"

#define NRF_LOG_MODULE_NAME "BLE_RSCS_C"
#include "nrf_log.h"

#include "identificator.h"

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */

extern ble_rscs_c_t          m_ble_rscs_c;  

typedef enum
{
    READ_REQ,  /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ  /**< Type identifying that this tx_message is a write request. */
} tx_request_t;

/**@brief Structure for writing a message to the peer, i.e. CCCD.
 */
typedef struct
{
    uint8_t                  gattc_value[WRITE_MESSAGE_LENGTH];  /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                       /**< GATTC parameters for this message. */
} write_params_t;

/**@brief Structure for holding data to be transmitted to the connected central.
 */
typedef struct
{
    uint16_t     conn_handle;  /**< Connection handle to be used when transmitting this message. */
    tx_request_t type;         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t       read_handle;  /**< Read request message. */
        write_params_t write_req;    /**< Write request message. */
    } req;
} tx_message_t;


static tx_message_t   m_tx_buffer[TX_BUFFER_SIZE];  /**< Transmit buffer for messages to be transmitted to the central. */
static uint32_t       m_tx_insert_index = 0;        /**< Current index in the transmit buffer where the next message should be inserted. */
static uint32_t       m_tx_index = 0;               /**< Current index in the transmit buffer from where the next message to be transmitted resides. */


/**@brief Function for passing any pending request from the buffer to the stack.
 */
static void tx_buffer_process(void)
{
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;

        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            NRF_LOG_INFO("SD Read/Write API returns Success.\r\n");
            m_tx_index++;
            m_tx_index &= TX_BUFFER_MASK;
        }
        else
        {
            NRF_LOG_INFO("SD Read/Write API returns error. This message sending will be "
                "attempted again..\r\n");
        }
    }
}


/**@brief     Function for handling write response events.
 *
 * @param[in] p_ble_rscs_c Pointer to the Running Speed and Cadence Client structure.
 * @param[in] p_ble_evt    Pointer to the BLE event received.
 */
static void on_write_rsp(ble_rscs_c_t * p_ble_rscs_c, const ble_evt_t * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_ble_rscs_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
    // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}

///**@brief     Function for handling read response events.
// *
// * @details   This function will validate the read response and raise the appropriate
// *            event to the application.
// *
// * @param[in] p_rscs_c   Pointer to the Running Speed and Cadence Client Structure.
// * @param[in] p_ble_evt  Pointer to the SoftDevice event.
// */
//static void on_read_rsp(ble_rscs_c_t * p_rscs_c, const ble_evt_t * p_ble_evt)
//{	
//	  uint8_t i;
//		const ble_gattc_evt_read_rsp_t * p_response;

//    // Check if the event if on the link for this instance
//    if (p_rscs_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
//    {
//        return;
//    }

//    p_response = &p_ble_evt->evt.gattc_evt.params.read_rsp;
//    
//		ble_rscs_c_evt_t evt;

//		evt.conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
//		evt.evt_type = BLE_RSCS_C_EVT_MOTION_PARAMETER_READ_RESP;
//		evt.characteristic_handle=p_response->handle;
//		
//		for(i=0;i<p_ble_evt->evt.gattc_evt.params.read_rsp.len;i++)            //获取数据
//		{
//			evt.params.databuffer[i]=p_response->data[i];
//		}
//		
//	#if 0
//		printf("p_response data buff data:");
//		for(i=0;i<20;i++)
//			printf("%x ",p_response->data[i]);
//		printf("\r\n");
//		
//		printf("evt params databuffer:");
//		for(i=0;i<20;i++)
//			printf("%d ",evt.params.databuffer[i]);
//		printf("\r\n");
//		
//		printf("Heart rate value is:%d\r\n",evt.params.databuffer[16]);
//		printf("creat read response events.\r\n");
//	#endif
//	
//		p_rscs_c->evt_handler(p_rscs_c, &evt);
//		
//    // Check if there is any buffered transmissions and send them.
//    tx_buffer_process();
//}

/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function will uses the Handle Value Notification received from the SoftDevice
 *            and checks if it is a notification of the Running Speed and Cadence measurement from
 *            the peer. If it is, this function will decode the Running Speed measurement and send it
 *            to the application.
 *
 * @param[in] p_ble_rscs_c Pointer to the Running Speed and Cadence Client structure.
 * @param[in] p_ble_evt    Pointer to the BLE event received.
 */
static void on_hvx(ble_rscs_c_t * p_ble_rscs_c, const ble_evt_t * p_ble_evt)
{
		uint32_t   index = 0;
    const ble_gattc_evt_hvx_t * p_notif = &p_ble_evt->evt.gattc_evt.params.hvx;

    // Check if the event if on the link for this instance
    if (p_ble_rscs_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
		
		ble_rscs_c_evt_t ble_rscs_c_evt;
		ble_rscs_c_evt.evt_type    = BLE_RSCS_C_EVT_RSC_NOTIFICATION;
		ble_rscs_c_evt.conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
		ble_rscs_c_evt.characteristic_handle=p_ble_evt->evt.gattc_evt.params.hvx.handle;
		
		for(index=0;index<20;index++)
		   ble_rscs_c_evt.params.databuffer[index]=p_notif->data[index];
		
#if 0
    // Check if this is a Running Speed and Cadence notification.
    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_rscs_c->peer_db.rsc_handle)
    {
        uint32_t         index = 0;
        ble_rscs_c_evt_t ble_rscs_c_evt;
        ble_rscs_c_evt.evt_type    = BLE_RSCS_C_EVT_RSC_NOTIFICATION;
        ble_rscs_c_evt.conn_handle = p_ble_evt->evt.gattc_evt.conn_handle;
        ble_rscs_c_evt.characteristic_handle=p_ble_evt->evt.gattc_evt.params.hvx.handle;
			
        //lint -save -e415 -e416 -e662 "Access of out of bounds pointer" "Creation of out of bounds pointer"

			// Flags field
        ble_rscs_c_evt.params.rsc.is_inst_stride_len_present = p_notif->data[index] >> BLE_RSCS_INSTANT_STRIDE_LEN_PRESENT    & 0x01;
        ble_rscs_c_evt.params.rsc.is_total_distance_present  = p_notif->data[index] >> BLE_RSCS_TOTAL_DISTANCE_PRESENT        & 0x01;
        ble_rscs_c_evt.params.rsc.is_running                 = p_notif->data[index] >> BLE_RSCS_WALKING_OR_RUNNING_STATUS_BIT & 0x01;
        index++;
			
			  // Instantaneous Speed
        ble_rscs_c_evt.params.rsc.inst_speed = uint16_decode(&p_notif->data[index]);
        index += sizeof(uint16_t);

        // Instantaneous Cadence
        ble_rscs_c_evt.params.rsc.inst_cadence = p_notif->data[index];
        index++;

        // Instantaneous Stride Length
        if (ble_rscs_c_evt.params.rsc.is_inst_stride_len_present == true)
        {
            ble_rscs_c_evt.params.rsc.inst_stride_length = uint16_decode(&p_notif->data[index]);
            index += sizeof(uint16_t);
        }

        // Total distance field
        if (ble_rscs_c_evt.params.rsc.is_total_distance_present == true)
        {
            ble_rscs_c_evt.params.rsc.total_distance = uint32_decode(&p_notif->data[index]);
            //index += sizeof(uint32_t);
        }

        p_ble_rscs_c->evt_handler(p_ble_rscs_c, &ble_rscs_c_evt);

        //lint -restore
		    //		printf("rscs on_hvx");

			   // Flags field
			  ble_rscs_c_evt.params.rsc.Motion_state_flag = p_notif->data[index];
        index++;
				
				// Instantaneous Speed
        ble_rscs_c_evt.params.rsc.inst_speed = p_notif->data[index];
        index++;
				
				// Instantaneous total steps
        ble_rscs_c_evt.params.rsc.total_steps = uint16_decode(&p_notif->data[index]);
        index += sizeof(uint16_t);
				
				// Instantaneous total distance
        ble_rscs_c_evt.params.rsc.total_distance = uint16_decode(&p_notif->data[index]);
        index += sizeof(uint16_t);

				// Instantaneous Movement time
        ble_rscs_c_evt.params.rsc.Movement_time = uint16_decode(&p_notif->data[index]);
        index += sizeof(uint16_t);
			
			  // Instantaneous deep sleep time
        ble_rscs_c_evt.params.rsc.deep_sleep_time = uint16_decode(&p_notif->data[index]);
        index += sizeof(uint16_t);

			  // Instantaneous shallowly sleep time
        ble_rscs_c_evt.params.rsc.shallowly_sleep_time = uint16_decode(&p_notif->data[index]);
        index += sizeof(uint16_t);

			  // Instantaneous rest time
        ble_rscs_c_evt.params.rsc.rest_time = uint16_decode(&p_notif->data[index]);
        index += sizeof(uint16_t);

			  // Instantaneous Heart rate value
        ble_rscs_c_evt.params.rsc.Heart_rate = uint16_decode(&p_notif->data[index]);
        index ++;
				// Instantaneous Blood oxygen value
        ble_rscs_c_evt.params.rsc.Blood_oxygen = uint16_decode(&p_notif->data[index]);

#endif
			  p_ble_rscs_c->evt_handler(p_ble_rscs_c, &ble_rscs_c_evt);
}


/**@brief     Function for handling events from the database discovery module.
 *
 * @details   This function will handle an event from the database discovery module, and determine
 *            if it relates to the discovery of heart rate service at the peer. If so, it will
 *            call the application's event handler indicating that the Running Speed and Cadence
 *            service has been discovered at the peer. It also populates the event with the service
 *            related information before providing it to the application.
 *
 * @param[in] p_evt Pointer to the event received from the database discovery module.
 *
 */
void ble_rscs_on_db_disc_evt(ble_rscs_c_t * p_ble_rscs_c, const ble_db_discovery_evt_t * p_evt)
{
    // Check if the Heart Rate Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_RUNNING_SPEED_AND_CADENCE &&
        p_evt->params.discovered_db.srv_uuid.type == BLE_UUID_TYPE_BLE)
    {

        ble_rscs_c_evt_t evt;
        evt.conn_handle = p_evt->conn_handle;

        // Find the CCCD Handle of the Running Speed and Cadence characteristic.
        uint32_t i;
        			
        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_RSC_MEASUREMENT_CHAR)
            {
                // Found Running Speed and Cadence characteristic. Store CCCD handle and break.
                evt.params.rscs_db.rsc_cccd_handle =
                    p_evt->params.discovered_db.charateristics[i].cccd_handle;
                evt.params.rscs_db.rsc_handle      =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
//							  printf("measurement.\r\n");
            }
					  else if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==                //保存消息推送句柄
                BLE_UUID_RSC_MESSAGE_PUSH_CHAR)
            {
                m_ble_rscs_c.message_push_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
//								printf("message push.\r\n");
            }
					  else if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==                //保存消息推送开关句柄
                BLE_UUID_RSC_MESSAGE_PUSH_SWITCH_CHAR)
            {
                m_ble_rscs_c.message_push_switch_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
//								printf("message push switch.\r\n");
//							  printf("nrf OK!.\r\n");
            }
						else if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==                //保存消息推送开关句柄
                BLE_UUID_RSC_TIME_SYNC_CHAR)
            {
                m_ble_rscs_c.Time_Sync_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
//								printf("Time Sync.\r\n");
            }
						else if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==                //保存消息推送开关句柄
                BLE_UUID_RSC_MEASUREMENT_HISTORY_CHAR)
            {
                m_ble_rscs_c.message_history_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
//								printf("measurement history.\r\n");
            }
						else if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==                //保存消息推送开关句柄
                BLE_UUID_RSC_BLOOD_MEASUREMENT_CHAR)
            {
                m_ble_rscs_c.blood_measurement_handle =
                    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
//								printf("blood measurement.\r\n");
            }
        }

        //If the instance has been assigned prior to db_discovery, assign the db_handles
        if (p_ble_rscs_c->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if ((p_ble_rscs_c->peer_db.rsc_cccd_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_ble_rscs_c->peer_db.rsc_handle == BLE_GATT_HANDLE_INVALID))
            {
                p_ble_rscs_c->peer_db = evt.params.rscs_db;
            }
        }

        evt.evt_type = BLE_RSCS_C_EVT_DISCOVERY_COMPLETE;

        p_ble_rscs_c->evt_handler(p_ble_rscs_c, &evt);
    }
}


uint32_t ble_rscs_c_init(ble_rscs_c_t * p_ble_rscs_c, ble_rscs_c_init_t * p_ble_rscs_c_init)
{
    VERIFY_PARAM_NOT_NULL(p_ble_rscs_c);
    VERIFY_PARAM_NOT_NULL(p_ble_rscs_c_init);

    ble_uuid_t rscs_uuid;
	
    rscs_uuid.type = BLE_UUID_TYPE_BLE;
    rscs_uuid.uuid = BLE_UUID_RUNNING_SPEED_AND_CADENCE;

    p_ble_rscs_c->evt_handler             = p_ble_rscs_c_init->evt_handler;
    p_ble_rscs_c->conn_handle             = BLE_CONN_HANDLE_INVALID;
    p_ble_rscs_c->peer_db.rsc_cccd_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_rscs_c->peer_db.rsc_handle      = BLE_GATT_HANDLE_INVALID;

    return ble_db_discovery_evt_register(&rscs_uuid);
}

uint32_t ble_rscs_c_handles_assign(ble_rscs_c_t *    p_ble_rscs_c,
                                   uint16_t         conn_handle,
                                   ble_rscs_c_db_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_rscs_c);
    p_ble_rscs_c->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_rscs_c->peer_db = *p_peer_handles;
    }

    return NRF_SUCCESS;
}


/**@brief     Function for handling Disconnected event received from the SoftDevice.
 *
 * @details   This function check if the disconnect event is happening on the link
 *            associated with the current instance of the module, if so it will set its
 *            conn_handle to invalid.
 *
 * @param[in] p_ble_rscs_c Pointer to the RSC Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_disconnected(ble_rscs_c_t * p_ble_rscs_c, const ble_evt_t * p_ble_evt)
{
    if (p_ble_rscs_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_ble_rscs_c->conn_handle             = BLE_CONN_HANDLE_INVALID;
        p_ble_rscs_c->peer_db.rsc_cccd_handle = BLE_GATT_HANDLE_INVALID;
        p_ble_rscs_c->peer_db.rsc_handle      = BLE_GATT_HANDLE_INVALID;
    }
}


void ble_rscs_c_on_ble_evt(ble_rscs_c_t * p_ble_rscs_c, const ble_evt_t * p_ble_evt)
{
    if ((p_ble_rscs_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    if ( (p_ble_rscs_c->conn_handle != BLE_CONN_HANDLE_INVALID)
       &&(p_ble_rscs_c->conn_handle != p_ble_evt->evt.gap_evt.conn_handle)
       )
    {
        return;
    }
		
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_rscs_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_ble_rscs_c, p_ble_evt);
            break;
				
        case BLE_GATTC_EVT_READ_RSP:
            break;
				
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnected(p_ble_rscs_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE:
            nrf_messend.txcompleteflag = 1;
				    //printf("HVN_TX_COMPLETE\r\n");
            break;
				
        default:
            break;
    }
}


/**@brief Function for creating a message for writing to the CCCD.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
 //   printf("Configuring CCCD. CCCD Handle = %d, Connection Handle = %d\r\n",
 //       handle_cccd, conn_handle);

    tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_GATT_HVX_NOTIFICATION : 0;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB_16(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB_16(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}

uint32_t ble_rscs_c_rsc_notif_enable(ble_rscs_c_t * p_ble_rscs_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_rscs_c);

    if (p_ble_rscs_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    return cccd_configure(p_ble_rscs_c->conn_handle, p_ble_rscs_c->peer_db.rsc_cccd_handle, true);
}

uint32_t ble_rscs_c_bl_read(ble_rscs_c_t * p_ble_rscs_c,uint16_t read_handle)
{
    VERIFY_PARAM_NOT_NULL(p_ble_rscs_c);
    if (p_ble_rscs_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    tx_message_t * msg;

    msg                  = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index   &= TX_BUFFER_MASK;

    msg->req.read_handle = read_handle;
    msg->conn_handle     = p_ble_rscs_c->conn_handle;
    msg->type            = READ_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}

/** @}
 *  @endcond
 */
#endif // NRF_MODULE_ENABLED(BLE_RSCS_C)
