#ifndef BLE_BRACELET_DFU_H
#define BLE_BRACELET_DFU_H

#include "stdint.h"
#include "ble_db_discovery.h"

#define DFU_STEPS_START              0x01
#define DFU_STEPS_DATA               0x02
#define DFU_STEPS_END                0x03

#define BLE_UUID_DFU_BRACELET_SERVICE 	      0xfff0
#define BLE_UUID_DFU_BRACELET_CHARACTERISTIC  0xfff1
#define DFU_BRACELET_BASE_UUID 		    	      {{0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}


struct ble_dfu_bracelet_c_s
{
  uint16_t                 conn_handle;
	uint16_t                 enter_dfu_handle;
};

typedef struct ble_dfu_bracelet_c_s ble_dfu_bracelet_c_t;

extern ble_dfu_bracelet_c_t m_ble_bracelet_c;


void ble_dfu_bracelet_c_init(void);
void ble_dfu_bracelet_ackResponse(uint8_t *bracelet, uint8_t steps, uint8_t err_code);
void ble_dfu_bracelet_on_db_disc_evt(ble_dfu_bracelet_c_t * p_ble_bracelet_c, const ble_db_discovery_evt_t * p_evt);

uint32_t ble_dfu_bracelet_start(void);
	
#endif