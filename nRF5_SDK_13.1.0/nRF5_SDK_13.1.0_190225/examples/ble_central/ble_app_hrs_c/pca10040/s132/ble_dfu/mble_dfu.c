#include "ble_dfu.h"
#include "nrf_log.h"
#include <string.h>
#include "ble_hci.h"
#include "sdk_macros.h"
#include "ble_srv_common.h"
#include "nrf_nvic.h"
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "nrf_log.h"

#define IRQ_ENABLED                     0x01                        /**< Field that identifies if an interrupt is enabled. */
#define MAX_NUMBER_INTERRUPTS           32                          /**< Maximum number of interrupts available. */
#define BOOTLOADER_DFU_START            0xB1

/**@brief Function for disabling all interrupts before jumping from bootloader to application.
 */
static void interrupts_disable(void)
{
    uint32_t interrupt_setting_mask;
    uint32_t irq;

    // Fetch the current interrupt settings.
    interrupt_setting_mask = NVIC->ISER[0];

    // Loop from interrupt 0 for disabling of all interrupts.
    for (irq = 0; irq < MAX_NUMBER_INTERRUPTS; irq++)
    {
        if (interrupt_setting_mask & (IRQ_ENABLED << irq))
        {
            // The interrupt was enabled, hence disable it.
            NVIC_DisableIRQ((IRQn_Type)irq);
        }
    }
}

/**@brief Function for preparing the reset, disabling SoftDevice, and jumping to the bootloader.
 *
 */
uint32_t bootloader_start(void)
{
    uint32_t err_code;

    err_code = sd_power_gpregret_clr(0, 0xffffffff);
    VERIFY_SUCCESS(err_code);

    err_code = sd_power_gpregret_set(0, BOOTLOADER_DFU_START);
    VERIFY_SUCCESS(err_code);

    err_code = sd_softdevice_disable();
    VERIFY_SUCCESS(err_code);

    err_code = sd_softdevice_vector_table_base_set(NRF_UICR->NRFFW[0]);
    VERIFY_SUCCESS(err_code);

    NVIC_ClearPendingIRQ(SWI2_IRQn);
    interrupts_disable();

    NVIC_SystemReset();
    return NRF_SUCCESS;
}

