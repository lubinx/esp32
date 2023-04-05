#ifndef __ESP32S3_SOC_H
#define __ESP32S3_SOC_H                 1

#include "esp_arch.h"
#include "soc/soc_caps.h"

// struct
#include "soc/apb_ctrl_struct.h"
#include "soc/apb_saradc_struct.h"
#include "soc/assist_debug_struct.h"
#include "soc/efuse_struct.h"
#include "soc/extmem_struct.h"
#include "soc/gdma_struct.h"
#include "soc/gpio_sd_struct.h"
#include "soc/gpio_struct.h"
#include "soc/hinf_struct.h"
#include "soc/host_struct.h"
#include "soc/i2c_struct.h"
#include "soc/i2s_struct.h"
#include "soc/interrupt_core0_struct.h"
#include "soc/interrupt_core1_struct.h"
#include "soc/lcd_cam_struct.h"
#include "soc/ledc_struct.h"
#include "soc/mcpwm_struct.h"
#include "soc/peri_backup_struct.h"
#include "soc/rmt_struct.h"
#include "soc/rtc_cntl_struct.h"
#include "soc/rtc_i2c_struct.h"
#include "soc/rtc_io_struct.h"
#include "soc/sdmmc_struct.h"
#include "soc/sens_struct.h"
#include "soc/sensitive_struct.h"
#include "soc/spi_mem_struct.h"
#include "soc/spi_struct.h"
#include "soc/syscon_struct.h"
#include "soc/system_struct.h"
#include "soc/systimer_struct.h"
#include "soc/timer_group_struct.h"
#include "soc/twai_struct.h"
#include "soc/uart_struct.h"
#include "soc/uhci_struct.h"
#include "soc/usb_dwc_struct.h"
#include "soc/usb_serial_jtag_struct.h"
#include "soc/usb_struct.h"
#include "soc/usb_wrap_struct.h"
#include "soc/world_controller_struct.h"

// reg
#include "soc/apb_ctrl_reg.h"
#include "soc/apb_saradc_reg.h"
#include "soc/assist_debug_reg.h"
#include "soc/efuse_reg.h"
#include "soc/extmem_reg.h"
#include "soc/gdma_reg.h"
#include "soc/gpio_sd_reg.h"
#include "soc/gpio_reg.h"
#include "soc/hinf_reg.h"
#include "soc/host_reg.h"
#include "soc/i2c_reg.h"
#include "soc/i2s_reg.h"
#include "soc/interrupt_core0_reg.h"
#include "soc/interrupt_core1_reg.h"
#include "soc/lcd_cam_reg.h"
#include "soc/ledc_reg.h"
#include "soc/mcpwm_reg.h"
#include "soc/peri_backup_reg.h"
#include "soc/rmt_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_i2c_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sdmmc_reg.h"
#include "soc/sens_reg.h"
#include "soc/sensitive_reg.h"
#include "soc/spi_mem_reg.h"
#include "soc/spi_reg.h"
#include "soc/syscon_reg.h"
#include "soc/system_reg.h"
#include "soc/systimer_reg.h"
#include "soc/timer_group_reg.h"
#include "soc/uart_reg.h"
#include "soc/uhci_reg.h"
#include "soc/usb_serial_jtag_reg.h"
#include "soc/usb_reg.h"
#include "soc/usb_wrap_reg.h"
#include "soc/world_controller_reg.h"

// pins
#include "soc/gpio_pins.h"
#include "soc/sdmmc_pins.h"
#include "soc/spi_pins.h"
#include "soc/touch_sensor_pins.h"
#include "soc/uart_pins.h"
#include "soc/usb_pins.h"

// wdt
    // wdt lock/unlock values
    #define WDT_LOCK_VALUE              (0)
    #define WDT_UNLOCK_VALUE            (0x50D83AA1)

#include "soc/ext_mem_defs.h"
    // mmu page size is fixed 64k, not depends sdkconfig.h
    #define SOC_MMU_PAGE_SIZE           (0x10000)


__BEGIN_DECLS

extern __attribute__((nothrow))
    void SOC_initialize(void);

extern __attribute__((nothrow, noreturn))
    void SOC_reset(void);

extern __attribute__((nothrow))
    void SOC_reset_core(unsigned core_id);

extern __attribute__((nothrow))
    void SOC_acquire_core(unsigned core_id);

extern __attribute__((nothrow))
    void SOC_release_core(unsigned core_id);

__END_DECLS
#endif
