#include "wdt.h"

#define NRF_LOG_MODULE_NAME "WDT"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define RELOAD_COUNT (32768*20*1-1)    //3 minutes

void wdt_init(void)
{
    NRF_WDT->TASKS_START = 0;
    NRF_WDT->CRV = RELOAD_COUNT;
    NRF_WDT->CONFIG =
        WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos |
        WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos;
    NRF_WDT->RREN = WDT_RREN_RR0_Enabled << WDT_RREN_RR0_Pos;
}

void wdt_start(void)
{
    NRF_WDT->TASKS_START = 1;
}

void wdt_feed(void)
{
    if(NRF_WDT->RUNSTATUS & WDT_RUNSTATUS_RUNSTATUS_Msk)
        NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void wdt_stop(void)
{
    NRF_WDT->TASKS_START = 0;
}
