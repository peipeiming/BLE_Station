#ifndef DATADEAL_H
#define DATADEAL_H

#include "ble_bas_c.h"
#include "ble_rscs_c.h"

void Motion_parameter_deal(ble_rscs_c_evt_t * p_rscs_c_evt);
void bas_c_evt_handler(ble_bas_c_t * p_bas_c, ble_bas_c_evt_t * p_bas_c_evt);
void rscs_c_evt_handler(ble_rscs_c_t * p_rscs_c, ble_rscs_c_evt_t * p_rscs_c_evt);

#endif

