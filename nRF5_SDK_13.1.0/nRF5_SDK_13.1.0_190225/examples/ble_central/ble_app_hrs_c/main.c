/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/**
 * @brief BLE Heart Rate Collector application main file.
 *
 * This file contains the source code for a sample heart rate collector.
 */

#include "uart.h"
#include "strdeal.h"
#include "app_uart.h"
#include "datadeal.h"
#include "ble_rscs_c.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_db_discovery.h"
#include "softdevice_handler.h"
#include "app_util.h"
#include "app_error.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "peer_manager.h"
#include "ble_hrs_c.h"
#include "ble_bas_c.h"
#include "app_util.h"
#include "app_timer.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "fds.h"
#include "fstorage.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "identificator.h"

#include "mytime.h"

/*************APP***********************/
#include "wdt.h"
#include "app_adv.h"
#include "app_rtc.h"

#include "includes.h"
#include "ble_bracelet_dfu_c.h"
/*************APP***********************/

/*************DPF***********************/
#include "ble_dfu.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
/*************DPF***********************/

#define CENTRAL_LINK_COUNT          1                                   /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT       1                                   /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/
#define CONN_CFG_TAG                1                                   /**< A tag that refers to the BLE stack configuration we set with @ref sd_ble_cfg_set. Default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */

#define SEC_PARAM_BOND              1                                   /**< Perform bonding. */
#define SEC_PARAM_MITM              0                                   /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC              0                                   /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS          0                                   /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES   BLE_GAP_IO_CAPS_NONE                /**< No I/O capabilities. */
#define SEC_PARAM_OOB               0                                   /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE      7                                   /**< Minimum encryption key size in octets. */
#define SEC_PARAM_MAX_KEY_SIZE      16                                  /**< Maximum encryption key size in octets. */

//SCAN_INTERVAL 50ms  SCAN_WINDOW 25ms
#define SCAN_INTERVAL               160                                 /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                 144                                  /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_TIMEOUT                0x0000                              /**< Timout when scanning. 0x0000 disables timeout. */

#define MIN_CONNECTION_INTERVAL     MSEC_TO_UNITS(7.5, UNIT_1_25_MS)    /**< Determines minimum connection interval in millisecond. */
#define MAX_CONNECTION_INTERVAL     MSEC_TO_UNITS(30, UNIT_1_25_MS)     /**< Determines maximum connection interval in millisecond. */
#define SLAVE_LATENCY               0                                   /**< Determines slave latency in counts of connection events. */
#define SUPERVISION_TIMEOUT         MSEC_TO_UNITS(4000, UNIT_10_MS)     /**< Determines supervision time-out in units of 10 millisecond. */

#define TARGET_UUID                 BLE_UUID_HEART_RATE_SERVICE         /**< Target device name that application is looking for. */

/**************DPU******************/
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

//#define APP_ADV_INTERVAL              300                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms).300 */
#define APP_ADV_INTERVAL                3200                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms).300 */

#define APP_ADV_TIMEOUT_IN_SECONDS      0                                           /**< The advertising timeout in units of seconds. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)            /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)            /**< Maximum acceptable connection interval (0.2 second). */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */

#define DEVICE_NAME                     "R1"                                        /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "NordicSemiconductor"                       /**< Manufacturer. Will be passed to Device Information Service. */

static ble_dfu_t      m_dfus;     

// YOUR_JOB: Use UUIDs for service(s) used in your application.
static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}};

/**************DPU******************/
ble_gap_addr_t dev_mac_addr;
ble_advdata_manuf_data_t manuf_data;
uint8_t Backbuf[20]={0x5A,0x42,0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	
/**@breif Macro to unpack 16bit unsigned UUID from octet stream. */
#define UUID16_EXTRACT(DST, SRC) \
    do                           \
    {                            \
        (*(DST))   = (SRC)[1];   \
        (*(DST)) <<= 8;          \
        (*(DST))  |= (SRC)[0];   \
    } while (0)


/**@brief Variable length data encapsulation in terms of length and pointer to data */
typedef struct
{
    uint8_t  * p_data;      /**< Pointer to data. */
    uint16_t   data_len;    /**< Length of data. */
} data_t;


static ble_db_discovery_t    m_ble_db_discovery;           /**< Structure used to identify the DB Discovery module. */
//static ble_gap_scan_params_t m_scan_param;                 /**< Scan parameters requested for scanning and connection. */
static bool                  m_whitelist_disabled;         /**< True if whitelist has been temporarily disabled. */
static bool                  m_memory_access_in_progress;  /**< Flag to keep track of ongoing operations on persistent memory. */
static nrf_ble_gatt_t        m_gatt;                       /**< Structure for gatt module*/

static bool                  m_retry_db_disc;              /**< Flag to keep track of whether the DB discovery should be retried. */

/**
 * @brief Connection parameters requested for connection.
 */
static const ble_gap_conn_params_t m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,  /**< Minimum connection. */
    (uint16_t)MAX_CONNECTION_INTERVAL,  /**< Maximum connection. */
    (uint16_t)SLAVE_LATENCY,            /**< Slave latency. */
    (uint16_t)SUPERVISION_TIMEOUT       /**< Supervision time-out. */
};

/** @brief Scan parameters requested for scanning and connection. */
static const ble_gap_scan_params_t m_scan_param =
{
    .active   = 1,       //主动扫描，捕获数据包的同时也可以捕获响应的数据包
    .interval = SCAN_INTERVAL,
    .window   = SCAN_WINDOW,
    .timeout  = SCAN_TIMEOUT,

    #if (NRF_SD_BLE_API_VERSION == 2)
        .selective   = 0,
        .p_whitelist = NULL,
    #endif

    #if (NRF_SD_BLE_API_VERSION == 3)
        .use_whitelist  = NULL,
        .adv_dir_report = 0,
    #endif
};

void scan_start(void);


/**@brief Function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for handling database discovery events.
 *
 * @details This function is callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function should forward the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
//    ble_hrs_on_db_disc_evt(&m_ble_hrs_c, p_evt);
//     ble_bas_on_db_disc_evt(&m_ble_bas_c, p_evt);
		  ble_rscs_on_db_disc_evt(&m_ble_rscs_c, p_evt);
			ble_dfu_bracelet_on_db_disc_evt(&m_ble_bracelet_c,p_evt);
	    ble_dfus_c_on_db_disc_evt(&m_ble_dfus_c,p_evt);
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    ret_code_t err_code;

    switch (p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
        {
            NRF_LOG_INFO("Connected to a previously bonded device.\r\n");
        } break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
        {
            NRF_LOG_INFO("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.\r\n",
                         ble_conn_state_role(p_evt->conn_handle),
                         p_evt->conn_handle,
                         p_evt->params.conn_sec_succeeded.procedure);
        } break;

        case PM_EVT_CONN_SEC_FAILED:
        {
            /* Often, when securing fails, it shouldn't be restarted, for security reasons.
             * Other times, it can be restarted directly.
             * Sometimes it can be restarted, but only after changing some Security Parameters.
             * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
             * Sometimes it is impossible, to secure the link, or the peer device does not support it.
             * How to handle this error is highly application dependent. */
        } break;

        case PM_EVT_CONN_SEC_CONFIG_REQ:
        {
            // Reject pairing request from an already bonded peer.
            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
        } break;

        case PM_EVT_STORAGE_FULL:
        {
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
            {
                // Retry.
            }
            else
            {
                APP_ERROR_CHECK(err_code);
            }
        } break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
        {
            // Bonds are deleted. Start scanning.
            scan_start();
        } break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
        {
            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
        } break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
        } break;

        case PM_EVT_PEER_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
        } break;

        case PM_EVT_PEERS_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
        } break;

        case PM_EVT_ERROR_UNEXPECTED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
        } break;

        case PM_EVT_CONN_SEC_START:
        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        case PM_EVT_PEER_DELETE_SUCCEEDED:
        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
        case PM_EVT_SERVICE_CHANGED_IND_SENT:
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
        default:
            break;
    }
}


/**
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data.
 * @param[in]  Advertisement report length and pointer to report.
 * @param[out] If data type requested is found in the data report, type data length and
 *             pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
static uint32_t adv_report_parse(uint8_t type, data_t * p_advdata, data_t * p_typedata)
{
    uint32_t  index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->data_len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type   = p_data[index + 1];

        if (field_type == type)
        {
            p_typedata->p_data   = &p_data[index + 2];
            p_typedata->data_len = field_length - 1;
            return NRF_SUCCESS;
        }
        index += field_length + 1;
    }
    return NRF_ERROR_NOT_FOUND;
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for searching a given name in the advertisement packets.
 *
 * @details Use this function to parse received advertising data and to find a given
 * name in them either as 'complete_local_name' or as 'short_local_name'.
 *
 * @param[in]   p_adv_report   advertising data to parse.
 * @param[in]   name_to_find   name to search.
 * @return   true if the given name was found, false otherwise.
 */
static bool find_adv_name(const ble_gap_evt_adv_report_t *p_adv_report, const char * name_to_find)
{
    ret_code_t err_code;
    data_t     adv_data;
    data_t     dev_name;

    // Initialize advertisement report for parsing
    adv_data.p_data     = (uint8_t *)p_adv_report->data;
    adv_data.data_len   = p_adv_report->dlen;

    //search for advertising names
    err_code = adv_report_parse(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
                                &adv_data,
                                &dev_name);
    if (err_code == NRF_SUCCESS)
    {
        if (memcmp(name_to_find, dev_name.p_data, dev_name.data_len )== 0)
        {
            return true;
        }
    }
    else
    {
        // Look for the short local name if it was not found as complete
        err_code = adv_report_parse(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME,
                                    &adv_data,
                                    &dev_name);
        if (err_code != NRF_SUCCESS)
        {
            return false;
        }
        if (memcmp(name_to_find, dev_name.p_data, dev_name.data_len )== 0)
        {
            return true;
        }
    }
    return false;
}


/**@brief Function for searching a given addr in the advertisement packets.
 *
 * @details Use this function to parse received advertising data and to find a given
 * addr in them.
 *
 * @param[in]   p_adv_report   advertising data to parse.
 * @param[in]   p_addr   name to search.
 * @return   true if the given name was found, false otherwise.
 */
static bool find_peer_addr(const ble_gap_evt_adv_report_t *p_adv_report, const ble_gap_addr_t * p_addr)
{
    if (p_addr->addr_type == p_adv_report->peer_addr.addr_type)
    {
        if (memcmp(p_addr->addr, p_adv_report->peer_addr.addr, sizeof(p_adv_report->peer_addr.addr)) == 0)
        {
            return true;
        }
    }
    return false;
}


/**@brief Function for searching a UUID in the advertisement packets.
 *
 * @details Use this function to parse received advertising data and to find a given
 * UUID in them.
 *
 * @param[in]   p_adv_report   advertising data to parse.
 * @param[in]   uuid_to_find   UUIID to search.
 * @return   true if the given UUID was found, false otherwise.
 */
//static bool find_adv_uuid(const ble_gap_evt_adv_report_t *p_adv_report, const uint16_t uuid_to_find)
//{
//    ret_code_t err_code;
//    data_t     adv_data;
//    data_t     type_data;

//    // Initialize advertisement report for parsing.
//    adv_data.p_data     = (uint8_t *)p_adv_report->data;
//    adv_data.data_len   = p_adv_report->dlen;

//    err_code = adv_report_parse(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE,
//                                &adv_data,
//                                &type_data);

//    if (err_code != NRF_SUCCESS)
//    {
//        // Look for the services in 'complete' if it was not found in 'more available'.
//        err_code = adv_report_parse(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,
//                                    &adv_data,
//                                    &type_data);

//        if (err_code != NRF_SUCCESS)
//        {
//            // If we can't parse the data, then exit.
//            return false;
//        }
//    }

//    // Verify if any UUID match the given UUID.
//    for (uint32_t u_index = 0; u_index < (type_data.data_len / sizeof(uint16_t)); u_index++)
//    {
//        uint16_t extracted_uuid;

//        UUID16_EXTRACT(&extracted_uuid, &type_data.p_data[u_index * sizeof(uint16_t)]);

//        if (extracted_uuid == uuid_to_find)
//        {
//            return true;
//        }
//    }
//    return false;
//}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
	  uint8_t i;
		uint8_t check_results=0;
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
	    
	  #if 0
	  static uint16_t count;
		char table[6]={0x34,0x08,0x5a,0xab,0x08,0xe3};
	  #endif
		
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
        {   
					  connect_flag=true;
					  if(do_connect == false)   /*手机发起的连接*/
						{
							link_time = 0;               /*主机连接计时*/
							is_connect_per_addr = true;  /*禁止断开标志*/
						}
            m_pending_db_disc_conn = p_ble_evt->evt.gap_evt.conn_handle;
            //printf("Connected.%d\r\n",m_pending_db_disc_conn); 
			  		m_retry_db_disc = false;
            // Discover peer's services.
            err_code = ble_db_discovery_start(&m_ble_db_discovery, m_pending_db_disc_conn);
            if (err_code == NRF_ERROR_BUSY)
            {
 //               printf("ble_db_discovery_start() returned busy, will retry later.\r\n");
                m_retry_db_disc = true;
            }
            else
            {
                APP_ERROR_CHECK(err_code);
            }

            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);

            if (ble_conn_state_n_centrals() < NRF_BLE_CENTRAL_LINK_COUNT)
            {
                scan_start();
            }
        } break;

        case BLE_GAP_EVT_ADV_REPORT:
        {	
					  do_connect=false;
            if (is_connect_per_addr)
            {   
                if (find_peer_addr(&p_gap_evt->params.adv_report, &m_target_periph_addr))
                {
									  /*改变连接目标回执数据*/
									  for(i=0;i<8;i++)
										{
											Backbuf[4+i]=station_addr[i]; 
										}
										
										for(i=0;i<6;i++)
										{
											Backbuf[i+12]=p_gap_evt->params.adv_report.peer_addr.addr[i];
										}
										
										check_results=Backbuf[1];   
										for(i=2;i<(Backbuf[3]+2+3-1);i++)   //CMD字段到数据字段进行校验
										{
											check_results^=Backbuf[i];
										}
										Backbuf[i]=check_results;           //校验值
										Backbuf[i+1]=0xca;                  //结束符	
										
                    do_connect = true;
                }
            }
            else if (strlen(m_target_periph_name) != 0)
            {
                if (find_adv_name(&p_gap_evt->params.adv_report, "S1") || find_adv_name(&p_gap_evt->params.adv_report, "SS"))
                {
									gap_info_save(p_gap_evt);
                }
								else if (find_adv_name(&p_gap_evt->params.adv_report, "MI Band 2"))
								{
								  MiBand_info_save(p_gap_evt);
								}
            }
            if (do_connect)
            {
			          
							// Stop scanning.
                (void) sd_ble_gap_scan_stop();					
				  
                // Initiate connection.
                err_code = sd_ble_gap_connect(&p_gap_evt->params.adv_report.peer_addr,
                                              &m_scan_param,
                                              &m_connection_param,
                                              CONN_CFG_TAG);
             
//                m_whitelist_disabled = false;
							
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("Connection Request Failed, reason %d.\r\n", err_code);
                }
							  APP_ERROR_CHECK(err_code);
            }
        } break; // BLE_GAP_EVT_ADV_REPORT

        case BLE_GAP_EVT_DISCONNECTED:
        {
					  connect_dfu=false;
					  connect_flag=false;
            NRF_LOG_INFO("Disconnected, reason 0x%x.\r\n",
                         p_ble_evt->evt.gap_evt.params.disconnected.reason);

            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);

            // Reset DB discovery structure.
            memset(&m_ble_db_discovery, 0 , sizeof (m_ble_db_discovery));

            if (ble_conn_state_n_centrals() < NRF_BLE_CENTRAL_LINK_COUNT)
            {
                scan_start();
            }
        } break;

        case BLE_GAP_EVT_TIMEOUT:
        {
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)
            {
                NRF_LOG_DEBUG("Scan timed out.\r\n");
                scan_start();
            }
            else if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                NRF_LOG_INFO("Connection Request timed out.\r\n");
            }
        } break;
				
				/*主机连接事件*/
//				case BLE_GAP_EVT_CONN_PARAM_UPDATE:
//						connect_dfu=1;
//				    //printf("BLE_GAP_EVT_CONN_PARAM_UPDATE\r\n");
//						break;
								
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
					  //printf("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST");
            // Accepting parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                    &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.\r\n");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
								
        default:
            break;
    }
}

/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
static void on_sys_evt(uint32_t sys_evt)
{
    switch (sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            /* fall through */
        case NRF_EVT_FLASH_OPERATION_ERROR:

            if (m_memory_access_in_progress)
            {
                m_memory_access_in_progress = false;
                scan_start();
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack event has
 *  been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{   
	  #if 0
	  if(p_ble_evt->header.evt_id==29)
	    printf("evt occurs evt_id:%d,count:%d\r\n",p_ble_evt->header.evt_id,++count);
		#endif
		
    // Modules which depend on ble_conn_state, like Peer Manager,
    // should have their callbacks invoked after ble_conn_state's.
    ble_conn_state_on_ble_evt(p_ble_evt);
    pm_on_ble_evt(p_ble_evt);
    ble_db_discovery_on_ble_evt(&m_ble_db_discovery, p_ble_evt);
//    ble_hrs_c_on_ble_evt(&m_ble_hrs_c, p_ble_evt);
//    ble_bas_c_on_ble_evt(&m_ble_bas_c, p_ble_evt);
	  ble_rscs_c_on_ble_evt(&m_ble_rscs_c,p_ble_evt);
    bsp_btn_ble_on_ble_evt(p_ble_evt);
    nrf_ble_gatt_on_ble_evt(&m_gatt, p_ble_evt);
    on_ble_evt(p_ble_evt);
	  ble_dfus_c_on_ble_evt(&m_ble_dfus_c,p_ble_evt);
		
	  /*******************DPU****************/
    ble_advertising_on_ble_evt(p_ble_evt);
    ble_dfu_on_ble_evt(&m_dfus, p_ble_evt);
	  /*********** *******DPU****************/
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    fs_sys_event_handler(sys_evt);
    on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{   
	  uint32_t err_code;

    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = softdevice_app_ram_start_get(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Overwrite some of the default configurations for the BLE stack.
    ble_cfg_t ble_cfg;

    // Configure the number of custom UUIDS.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 1;    //自定义UUID数量，1个DFU UUID
    err_code = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.gap_cfg.role_count_cfg.periph_role_count  = 1;   //（1个可使用外部从机进行连接的链路）
    ble_cfg.gap_cfg.role_count_cfg.central_role_count = 1;   //（1个可使用中心主机设备进行连接的链路）
    ble_cfg.gap_cfg.role_count_cfg.central_sec_count  = 0;
    err_code = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);
		
    // Configure the maximum ATT MTU. NRF_BLE_GATT_MAX_MTU_SIZE had be modify to 23
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                 = CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.gatt_conn_cfg.att_mtu = NRF_BLE_GATT_MAX_MTU_SIZE;  
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);
		
    // Configure the maximum event length.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                     = CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.gap_conn_cfg.event_length = BLE_GAP_EVENT_LENGTH_DEFAULT;
    ble_cfg.conn_cfg.params.gap_conn_cfg.conn_count   = BLE_GAP_CONN_COUNT_DEFAULT;
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &ble_cfg, ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = softdevice_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/** @brief Clear bonding information from persistent storage
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!\r\n");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for disabling the use of whitelist for scanning.
 */
static void whitelist_disable(void)
{
    if (!m_whitelist_disabled)
    {
        NRF_LOG_INFO("Whitelist temporarily disabled.\r\n");
        m_whitelist_disabled = true;
        (void) sd_ble_gap_scan_stop();
        scan_start();
    }
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            whitelist_disable();
            break;

        default:
            break;
    }
}


/**@brief Heart Rate Collector Handler.
 */
//static void hrs_c_evt_handler(ble_hrs_c_t * p_hrs_c, ble_hrs_c_evt_t * p_hrs_c_evt)
//{
//    ret_code_t err_code;

//    switch (p_hrs_c_evt->evt_type)
//    {
//        case BLE_HRS_C_EVT_DISCOVERY_COMPLETE:
//        {
//            NRF_LOG_DEBUG("Heart rate service discovered.\r\n");

//            err_code = ble_hrs_c_handles_assign(p_hrs_c,
//                                                p_hrs_c_evt->conn_handle,
//                                                &p_hrs_c_evt->params.peer_db);
//            APP_ERROR_CHECK(err_code);

//            // Initiate bonding.
//            err_code = pm_conn_secure(p_hrs_c_evt->conn_handle, false);
//            if (err_code != NRF_ERROR_INVALID_STATE)
//            {
//                APP_ERROR_CHECK(err_code);
//            }

//            // Heart rate service discovered. Enable notification of Heart Rate Measurement.
//            err_code = ble_hrs_c_hrm_notif_enable(p_hrs_c);
//            APP_ERROR_CHECK(err_code);
//        } break;

//        case BLE_HRS_C_EVT_HRM_NOTIFICATION:
//        {
//            NRF_LOG_INFO("Heart Rate = %d.\r\n", p_hrs_c_evt->params.hrm.hr_value);

//            if (p_hrs_c_evt->params.hrm.rr_intervals_cnt != 0)
//            {
//                uint32_t rr_avg = 0;
//                for (uint32_t i = 0; i < p_hrs_c_evt->params.hrm.rr_intervals_cnt; i++)
//                {
//                    rr_avg += p_hrs_c_evt->params.hrm.rr_intervals[i];
//                }
//                rr_avg = rr_avg / p_hrs_c_evt->params.hrm.rr_intervals_cnt;
//                NRF_LOG_DEBUG("rr_interval (avg) = %d.\r\n", rr_avg);
//            }
//        } break;

//        default:
//            break;
//    }
//}

/**
 * @brief Runnign Speed and Cadence collector initialization.
 */
static void rscs_c_init(void)
{
    ble_rscs_c_init_t rscs_c_init_obj;

    rscs_c_init_obj.evt_handler = rscs_c_evt_handler;

    uint32_t err_code = ble_rscs_c_init(&m_ble_rscs_c, &rscs_c_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Database discovery collector initialization.
 */
static void db_discovery_init(void)
{
    ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Retrieve a list of peer manager peer IDs.
 *
 * @param[inout] p_peers   The buffer where to store the list of peer IDs.
 * @param[inout] p_size    In: The size of the @p p_peers buffer.
 *                         Out: The number of peers copied in the buffer.
 */
//static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size)
//{
//    pm_peer_id_t peer_id;
//    uint32_t     peers_to_copy;

//    peers_to_copy = (*p_size < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) ?
//                     *p_size : BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

//    peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
//    *p_size = 0;

//    while ((peer_id != PM_PEER_ID_INVALID) && (peers_to_copy--))
//    {
//        p_peers[(*p_size)++] = peer_id;
//        peer_id = pm_next_peer_id_get(peer_id);
//    }
//}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
}


/** @brief Function for the Power manager.
 */
static void power_manage(void)
{
    ret_code_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief GATT module event handler.
 */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    switch (p_evt->evt_id)
    {
        case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED:
        {
            NRF_LOG_INFO("GATT ATT MTU on connection 0x%x changed to %d.\r\n",
                         p_evt->conn_handle,
                         p_evt->params.att_mtu_effective);
        } break;

        case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED:
        {
            NRF_LOG_INFO("Data length for connection 0x%x updated to %d.\r\n",
                         p_evt->conn_handle,
                         p_evt->params.data_length);
        } break;

        default:
            break;
    }

    if (m_retry_db_disc)
    {
        NRF_LOG_DEBUG("Retrying DB discovery.\r\n");

        m_retry_db_disc = false;

        // Discover peer's services.
        ret_code_t err_code;
        err_code = ble_db_discovery_start(&m_ble_db_discovery, m_pending_db_disc_conn);

        if (err_code == NRF_ERROR_BUSY)
        {
//            NRF_LOG_DEBUG("ble_db_discovery_start() returned busy, will retry later.\r\n");
            m_retry_db_disc = true;
        }
        else
        {
            APP_ERROR_CHECK(err_code);
        }
    }
}


/**@brief Function for initializing the timer.
 */
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}

#ifdef APP_PA_LAN
static void pa_lna_assist(uint32_t gpio_pa_pin, uint32_t gpio_lna_pin)
{
    ret_code_t err_code;
    nrf_gpio_cfg_output(gpio_pa_pin);
    nrf_gpio_pin_clear(gpio_pa_pin); 
    nrf_gpio_cfg_output(gpio_lna_pin);
    nrf_gpio_pin_clear(gpio_lna_pin); 

		nrf_gpio_cfg_output(27);        //P27 High
		nrf_gpio_pin_set(27); 
	
    static const uint32_t gpio_toggle_ch = 0;
    static const uint32_t ppi_set_ch = 0;
    static const uint32_t ppi_clr_ch = 1;

    // Configure SoftDevice PA/LNA assist
    static ble_opt_t opt;
    memset(&opt, 0, sizeof(ble_opt_t));
    // Common PA/LNA config
    opt.common_opt.pa_lna.gpiote_ch_id  = gpio_toggle_ch;        // GPIOTE channel
    opt.common_opt.pa_lna.ppi_ch_id_clr = ppi_clr_ch;            // PPI channel for pin clearing
    opt.common_opt.pa_lna.ppi_ch_id_set = ppi_set_ch;            // PPI channel for pin setting
	
    // PA config
    opt.common_opt.pa_lna.pa_cfg.active_high = 1;                // Set the pin to be active high
    opt.common_opt.pa_lna.pa_cfg.enable      = 1;                // Enable toggling
    opt.common_opt.pa_lna.pa_cfg.gpio_pin    = gpio_pa_pin;      // The GPIO pin to toggle

    // LNA config
    opt.common_opt.pa_lna.lna_cfg.active_high  = 1;              // Set the pin to be active high
    opt.common_opt.pa_lna.lna_cfg.enable       = 1;              // Enable toggling
    opt.common_opt.pa_lna.lna_cfg.gpio_pin     = gpio_lna_pin;   // The GPIO pin to toggle

    err_code = sd_ble_opt_set(BLE_COMMON_OPT_PA_LNA, &opt);
    APP_ERROR_CHECK(err_code);
}
#endif

/**@brief Function for initiating scanning.
 */
static void scan_start(void)
{
    ret_code_t err_code;

    (void) sd_ble_gap_scan_stop();

    err_code = sd_ble_gap_scan_start(&m_scan_param);
    // It is okay to ignore this error since we are stopping the scan anyway.
    if (err_code != NRF_ERROR_INVALID_STATE)
    {
        APP_ERROR_CHECK(err_code);
    }

//		err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
//		APP_ERROR_CHECK(err_code);
		
		err_code = bsp_indication_set(BSP_INDICATE_SCANNING);
    APP_ERROR_CHECK(err_code);	
}

/*********************************************for DPU********************************************/
/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

static void ble_dfu_evt_handler(ble_dfu_t * p_dfu, ble_dfu_evt_t * p_evt)
{
    switch (p_evt->type)
    {
        case BLE_DFU_EVT_INDICATION_DISABLED:
            NRF_LOG_INFO("Indication for BLE_DFU is disabled.\r\n");
            break;

        case BLE_DFU_EVT_INDICATION_ENABLED:
            NRF_LOG_INFO("Indication for BLE_DFU is enabled.\r\n");
            break;

        case BLE_DFU_EVT_ENTERING_BOOTLOADER:
            NRF_LOG_INFO("Device is requested to enter bootloader mode!\r\n");
            break;

        default:
            NRF_LOG_INFO("Unknown event from ble_dfu.\r\n");
            break;
    }
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t err_code;
    ble_dfu_init_t dfus_init;

    // Initialize the Device Firmware Update Service.
    memset(&dfus_init, 0, sizeof(dfus_init));

    dfus_init.evt_handler                               = ble_dfu_evt_handler;
    dfus_init.ctrl_point_security_req_write_perm        = SEC_SIGNED;
    dfus_init.ctrl_point_security_req_cccd_write_perm   = SEC_SIGNED;

    err_code = ble_dfu_init(&m_dfus, &dfus_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
					  //printf("adv fast.\r\n");
					  bsp_board_led_on(BSP_BOARD_LED_2);
            break;

        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;

        default:
            break;
    }
}

/**@brief Function for initializing the Advertising functionality.
 */
void advertising_init(void)
{
    uint32_t               err_code;
    ble_advdata_t          advdata;
    ble_adv_modes_config_t options;

#if 1		
	  manuf_data.company_identifier = 0x0059;
    manuf_data.data.p_data = gap_data;
#endif
	
    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

#if 1
		manuf_data.data.size = 9;
    advdata.p_manuf_specific_data   = &manuf_data;
#endif

    memset(&options, 0, sizeof(options));
    options.ble_adv_fast_enabled  = true;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    /* YOUR_JOB: Use an appearance value matching the application's use case.
       err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
       APP_ERROR_CHECK(err_code); */

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}
/*******for DPU******/		

int main(void)
{
    bool erase_bonds;
	
		/*******bsp init**************/
		log_init();
    timer_init();
		wdt_init();		
	
	  uart_init();
    buttons_leds_init(&erase_bonds);

	  #if 1
    lfclk_config(); //LFCLK低频时钟配置
    rtc_config();   //RTC配置
	  #endif
		
		/*******gatt init*************/
    ble_stack_init();
    gatt_init();
    db_discovery_init();
    peer_manager_init();
		
		/*******rscs client init******/
    rscs_c_init();
		dfus_c_init();
		ble_dfu_bracelet_c_init();
		
	  /*******DFU services init*****/
		gap_params_init(); 
		conn_params_init();
    advertising_init();
//		services_init();
		
		wdt_start();  
		
		/*******PA and LAN init******/
		pa_lna_assist(25,26);     
		
    /*******GAP fifo init********/		
		adv_init();
		
		/*******set tx power*********/
		sd_ble_gap_tx_power_set(4);
		sd_ble_gap_addr_get(&dev_mac_addr);  
    memcpy(&station_addr[2],dev_mac_addr.addr,BLE_GAP_ADDR_LEN);	
		
		// Start scanning for peripherals and initiate connection
		// with devices that advertise Heart Rate UUID.
		NRF_LOG_INFO("Heart Rate collector example started.\r\n");
		if (erase_bonds == true)
		{   
				delete_bonds();
		 // Scanning is started by PM_EVT_PEERS_DELETE_SUCCEEDED
		}
		else
		{
				scan_start();
		}
		
		for (;;)
		{   
  		  wdt_feed();
	
  		  if(nrf_messend.txstartflag == 1 && nrf_messend.txcompleteflag == 1)  /*有要发送的消息*/
				{
					ble_rscs_message_send();
				}
				
				Dfu_bracelet();  /*升级蓝牙手环*/
				
				if(is_connect_per_addr == false && connect_flag == true)
				{
					NRF_LOG_INFO("Disconnect\r\n");
				  disconnect_ble_link();
				}
				else if( is_connect_per_addr == true && link_time > 60 )
				{
					NRF_LOG_INFO("Link out time\r\n");
					is_connect_per_addr = false;
				}	
								
				if (NRF_LOG_PROCESS() == false)
				{
					power_manage();
				}
		}
}


