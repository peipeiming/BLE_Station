#ifndef BLE_DFUS_H
#define BLE_DFUS_H

#include "includes.h"

#define BLE_DFUS_MAX_DATA_LEN 20
  
#define DFUS_BASE_UUID                    {{0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}} /**< Used vendor specific UUID. */

#define BLE_UUID_DFUS_SERVICE             0x1530                     /**< The UUID of the Nordic UART Service. */
#define BLE_UUID_DFUS_DP_CHARACTERISTIC   0x1532                     /**< The UUID of the RX Characteristic. */
#define BLE_UUID_DFUS_DCP_CHARACTERISTIC  0x1531                     /**< The UUID of the TX Characteristic. */

/**@brief dfus Client event type. */
typedef enum
{
    BLE_DFUS_C_EVT_DISCOVERY_COMPLETE = 1, /**< Event indicating that the dfus service and its characteristics was found. */
    BLE_DFUS_C_EVT_DFUS_DCP_EVT,           /**< Event indicating that the central has received something from a peer. */
    BLE_DFUS_C_EVT_DISCONNECTED            /**< Event indicating that the dfus server has disconnected. */
} ble_dfus_c_evt_type_t;

/**@brief Handles on the connected peer device needed to interact with it. */
typedef struct {
    uint16_t dfus_dcp_handle;      /**< Handle of the dfus dcp characteristic as provided by a discovery. */
    uint16_t dfus_dcp_cccd_handle; /**< Handle of the CCCD of the dfus dcp characteristic as provided by a discovery. */
    uint16_t dfus_dp_handle;      /**< Handle of the dfus dp characteristic as provided by a discovery. */
} ble_dfus_c_handles_t;

/**@brief Structure containing the dfus event data received from the peer. */
typedef struct {
    ble_dfus_c_evt_type_t evt_type;
    uint16_t              conn_handle;
    uint16_t              max_data_len;
    uint8_t               *p_data;
    uint8_t               data_len;
    ble_dfus_c_handles_t  handles;     /**< Handles on which the Nordic Uart service characteristics was discovered on the peer device. This will be filled if the evt_type is @ref BLE_dfus_C_EVT_DISCOVERY_COMPLETE.*/
} ble_dfus_c_evt_t;

// Forward declaration of the ble_dfus_t type.
typedef struct ble_dfus_c_s ble_dfus_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module to receive events.
 */
typedef void (* ble_dfus_c_evt_handler_t)(ble_dfus_c_t * p_ble_dfus_c, const ble_dfus_c_evt_t * p_evt);

/**@brief dfus Client structure. */
struct ble_dfus_c_s
{
    uint8_t                  uuid_type;      /**< UUID type. */
    uint16_t                 conn_handle;    /**< Handle of the current connection. Set with @ref ble_dfus_c_handles_assign when connected. */
    ble_dfus_c_handles_t     handles;        /**< Handles on the connected peer device needed to interact with it. */
    ble_dfus_c_evt_handler_t evt_handler;    /**< Application event handler to be called when there is an event related to the dfus. */
};

/**@brief dfus Client initialization structure. */
typedef struct
{
    ble_dfus_c_evt_handler_t evt_handler;
} ble_dfus_c_init_t;

extern ble_dfus_c_t m_ble_dfus_c;

void dfus_c_init(void);

void ble_dfus_c_on_ble_evt(ble_dfus_c_t * p_ble_dfus_c, const ble_evt_t * p_ble_evt);
void ble_dfus_c_on_db_disc_evt(ble_dfus_c_t * p_ble_dfus_c, ble_db_discovery_evt_t * p_evt);

uint32_t ble_dfus_c_write_cmd(ble_dfus_c_t * p_ble_dfus_c, uint8_t * cmd, uint16_t length);
uint32_t ble_dfus_c_write_data(ble_dfus_c_t * p_ble_dfus_c, uint8_t * data, uint16_t length);

#endif

