#ifndef __ESP32S3_EFUSE_H
#define __ESP32S3_EFUSE_H               1

#include <features.h>
#include <stdbool.h>

enum EFUSE_field
{
    EFUSE_FIELD_WR_DIS,                             // Disable programming of individual eFuses
    EFUSE_FIELD_RD_DIS,                             // Disable reading from BlOCK4-10
    EFUSE_FIELD_DIS_ICACHE,                         // Set this bit to disable Icache
    EFUSE_FIELD_DIS_DCACHE,                         // Set this bit to disable Dcache
    EFUSE_FIELD_DIS_DOWNLOAD_ICACHE,                // Set this bit to disable Icache in download mode (boot_mode[3:0] is 0; 1; 2; 3; 6; 7)
    EFUSE_FIELD_DIS_DOWNLOAD_DCACHE,                // Set this bit to disable Dcache in download mode ( boot_mode[3:0] is 0; 1; 2; 3; 6; 7)
    EFUSE_FIELD_DIS_FORCE_DOWNLOAD,                 // Set this bit to disable the function that forces chip into download mode
    EFUSE_FIELD_DIS_USB_OTG,                        // Set this bit to disable USB function
    EFUSE_FIELD_DIS_TWAI,                           // Set this bit to disable CAN function
    EFUSE_FIELD_DIS_APP_CPU,                        // Disable app cpu
    EFUSE_FIELD_SOFT_DIS_JTAG,                      // Set these bits to disable JTAG in the soft way (odd number 1 means disable ). JTAG can be enabled in HMAC module
    EFUSE_FIELD_DIS_PAD_JTAG,                       // Set this bit to disable JTAG in the hard way. JTAG is disabled permanently
    EFUSE_FIELD_DIS_DOWNLOAD_MANUAL_ENCRYPT,        // Set this bit to disable flash encryption when in download boot modes
    EFUSE_FIELD_USB_EXCHG_PINS,                     // Set this bit to exchange USB D+ and D- pins
    EFUSE_FIELD_USB_EXT_PHY_ENABLE,                 // Set this bit to enable external PHY
    EFUSE_FIELD_VDD_SPI_XPD,                        // SPI regulator power up signal
    EFUSE_FIELD_VDD_SPI_TIEH,                       // If VDD_SPI_FORCE is 1; determines VDD_SPI voltage {0: "VDD_SPI connects to 1.8 V LDO"; 1: "VDD_SPI connects to VDD3P3_RTC_IO"}
    EFUSE_FIELD_VDD_SPI_FORCE,                      // Set this bit and force to use the configuration of eFuse to configure VDD_SPI
    EFUSE_FIELD_WDT_DELAY_SEL,                      // RTC watchdog timeout threshold; in unit of slow clock cycle {0: "40000"; 1: "80000"; 2: "160000"; 3: "320000"}
    EFUSE_FIELD_SPI_BOOT_CRYPT_CNT,                 // Enables flash encryption when 1 or 3 bits are set and disabled otherwise {0: "Disable"; 1: "Enable"; 3: "Disable"; 7: "Enable"}
    EFUSE_FIELD_SECURE_BOOT_KEY_REVOKE0,            // Revoke 1st secure boot key
    EFUSE_FIELD_SECURE_BOOT_KEY_REVOKE1,            // Revoke 2nd secure boot key
    EFUSE_FIELD_SECURE_BOOT_KEY_REVOKE2,            // Revoke 3rd secure boot key
    EFUSE_FIELD_KEY_PURPOSE_0,                      // Purpose of Key0
    EFUSE_FIELD_KEY_PURPOSE_1,                      // Purpose of Key1
    EFUSE_FIELD_KEY_PURPOSE_2,                      // Purpose of Key2
    EFUSE_FIELD_KEY_PURPOSE_3,                      // Purpose of Key3
    EFUSE_FIELD_KEY_PURPOSE_4,                      // Purpose of Key4
    EFUSE_FIELD_KEY_PURPOSE_5,                      // Purpose of Key5
    EFUSE_FIELD_SECURE_BOOT_EN,                     // Set this bit to enable secure boot
    EFUSE_FIELD_SECURE_BOOT_AGGRESSIVE_REVOKE,      // Set this bit to enable revoking aggressive secure boot
    EFUSE_FIELD_DIS_USB_JTAG,                       // Set this bit to disable function of usb switch to jtag in module of usb device
    EFUSE_FIELD_DIS_USB_SERIAL_JTAG,                // Set this bit to disable usb device
    EFUSE_FIELD_STRAP_JTAG_SEL,                     // Set this bit to enable selection between usb_to_jtag and pad_to_jtag through strapping gpio10 when both reg_dis_usb_jtag and reg_dis_pad_jtag are equal to 0
    EFUSE_FIELD_USB_PHY_SEL,                        // This bit is used to switch internal PHY and external PHY for USB OTG and USB Device {0: "internal PHY is assigned to USB Device while external PHY is assigned to USB OTG"; 1: "internal PHY is assigned to USB OTG while external PHY is assigned to USB Device"}
    EFUSE_FIELD_FLASH_TPUW,                         // Configures flash waiting time after power-up; in unit of ms. If the value is less than 15; the waiting time is the configurable value.  Otherwise; the waiting time is twice the configurable value
    EFUSE_FIELD_DIS_DOWNLOAD_MODE,                  // Set this bit to disable download mode (boot_mode[3:0] = 0; 1; 2; 3; 6; 7)
    EFUSE_FIELD_DIS_DIRECT_BOOT,                    // Disable direct boot mode
    EFUSE_FIELD_DIS_USB_SERIAL_JTAG_ROM_PRINT,      // USB printing {0: "Enable"; 1: "Disable"}
    EFUSE_FIELD_FLASH_ECC_MODE,                     // Flash ECC mode in ROM {0: "16to18 byte"; 1: "16to17 byte"}
    EFUSE_FIELD_DIS_USB_SERIAL_JTAG_DOWNLOAD_MODE,  // Set this bit to disable UART download mode through USB
    EFUSE_FIELD_ENABLE_SECURITY_DOWNLOAD,           // Set this bit to enable secure UART download mode
    EFUSE_FIELD_UART_PRINT_CONTROL,                 // Set the default UART boot message output mode {0: "Enable"; 1: "Enable when GPIO46 is low at reset"; 2: "Enable when GPIO46 is high at reset"; 3: "Disable"}
    EFUSE_FIELD_PIN_POWER_SELECTION,                // Set default power supply for GPIO33-GPIO37; set when SPI flash is initialized {0: "VDD3P3_CPU"; 1: "VDD_SPI"}
    EFUSE_FIELD_FLASH_TYPE,                         // SPI flash type {0: "4 data lines"; 1: "8 data lines"}
    EFUSE_FIELD_FLASH_PAGE_SIZE,                    // Set Flash page size
    EFUSE_FIELD_FLASH_ECC_EN,                       // Set 1 to enable ECC for flash boot
    EFUSE_FIELD_FORCE_SEND_RESUME,                  // Set this bit to force ROM code to send a resume command during SPI boot
    EFUSE_FIELD_SECURE_VERSION,                     // Secure version (used by ESP-IDF anti-rollback feature)
    EFUSE_FIELD_DIS_USB_OTG_DOWNLOAD_MODE,          // Set this bit to disable download through USB-OTG
    EFUSE_FIELD_DISABLE_WAFER_VERSION_MAJOR,        // Disables check of wafer version major
    EFUSE_FIELD_DISABLE_BLK_VERSION_MAJOR,          // Disables check of blk version major
    EFUSE_FIELD_MAC,                                // MAC address
    EFUSE_FIELD_SPI_PAD_CONFIG_CLK,                 // SPI_PAD_configure CLK
    EFUSE_FIELD_SPI_PAD_CONFIG_Q,                   // SPI_PAD_configure Q(D1)
    EFUSE_FIELD_SPI_PAD_CONFIG_D,                   // SPI_PAD_configure D(D0)
    EFUSE_FIELD_SPI_PAD_CONFIG_CS,                  // SPI_PAD_configure CS
    EFUSE_FIELD_SPI_PAD_CONFIG_HD,                  // SPI_PAD_configure HD(D3)
    EFUSE_FIELD_SPI_PAD_CONFIG_WP,                  // SPI_PAD_configure WP(D2)
    EFUSE_FIELD_SPI_PAD_CONFIG_DQS,                 // SPI_PAD_configure DQS
    EFUSE_FIELD_SPI_PAD_CONFIG_D4,                  // SPI_PAD_configure D4
    EFUSE_FIELD_SPI_PAD_CONFIG_D5,                  // SPI_PAD_configure D5
    EFUSE_FIELD_SPI_PAD_CONFIG_D6,                  // SPI_PAD_configure D6
    EFUSE_FIELD_SPI_PAD_CONFIG_D7,                  // SPI_PAD_configure D7
    EFUSE_FIELD_WAFER_VERSION_MINOR_LO,             // WAFER_VERSION_MINOR least significant bits
    EFUSE_FIELD_PKG_VERSION,                        // Package version
    EFUSE_FIELD_BLK_VERSION_MINOR,                  // BLK_VERSION_MINOR
    EFUSE_FIELD_K_RTC_LDO,                          // BLOCK1 K_RTC_LDO
    EFUSE_FIELD_K_DIG_LDO,                          // BLOCK1 K_DIG_LDO
    EFUSE_FIELD_V_RTC_DBIAS20,                      // BLOCK1 voltage of rtc dbias20
    EFUSE_FIELD_V_DIG_DBIAS20,                      // BLOCK1 voltage of digital dbias20
    EFUSE_FIELD_DIG_DBIAS_HVT,                      // BLOCK1 digital dbias when hvt
    EFUSE_FIELD_WAFER_VERSION_MINOR_HI,             // WAFER_VERSION_MINOR most significant bit
    EFUSE_FIELD_WAFER_VERSION_MAJOR,                // WAFER_VERSION_MAJOR
    EFUSE_FIELD_ADC2_CAL_VOL_ATTEN3,                // ADC2 calibration voltage at atten3
    EFUSE_FIELD_OPTIONAL_UNIQUE_ID,                 // Optional unique 128-bit ID
    EFUSE_FIELD_BLK_VERSION_MAJOR,                  // BLK_VERSION_MAJOR of BLOCK2 {0: "No calib"; 1: "ADC calib V1"}
    EFUSE_FIELD_TEMP_CALIB,                         // Temperature calibration data
    EFUSE_FIELD_OCODE,                              // ADC OCode
    EFUSE_FIELD_ADC1_INIT_CODE_ATTEN0,              // ADC1 init code at atten0
    EFUSE_FIELD_ADC1_INIT_CODE_ATTEN1,              // ADC1 init code at atten1
    EFUSE_FIELD_ADC1_INIT_CODE_ATTEN2,              // ADC1 init code at atten2
    EFUSE_FIELD_ADC1_INIT_CODE_ATTEN3,              // ADC1 init code at atten3
    EFUSE_FIELD_ADC2_INIT_CODE_ATTEN0,              // ADC2 init code at atten0
    EFUSE_FIELD_ADC2_INIT_CODE_ATTEN1,              // ADC2 init code at atten1
    EFUSE_FIELD_ADC2_INIT_CODE_ATTEN2,              // ADC2 init code at atten2
    EFUSE_FIELD_ADC2_INIT_CODE_ATTEN3,              // ADC2 init code at atten3
    EFUSE_FIELD_ADC1_CAL_VOL_ATTEN0,                // ADC1 calibration voltage at atten0
    EFUSE_FIELD_ADC1_CAL_VOL_ATTEN1,                // ADC1 calibration voltage at atten1
    EFUSE_FIELD_ADC1_CAL_VOL_ATTEN2,                // ADC1 calibration voltage at atten2
    EFUSE_FIELD_ADC1_CAL_VOL_ATTEN3,                // ADC1 calibration voltage at atten3
    EFUSE_FIELD_ADC2_CAL_VOL_ATTEN0,                // ADC2 calibration voltage at atten0
    EFUSE_FIELD_ADC2_CAL_VOL_ATTEN1,                // ADC2 calibration voltage at atten1
    EFUSE_FIELD_ADC2_CAL_VOL_ATTEN2,                // ADC2 calibration voltage at atten2
    EFUSE_FIELD_USER_DATA,                          // User data
    EFUSE_FIELD_KEY0,                               // Key0 or user data
    EFUSE_FIELD_KEY1,                               // Key1 or user data
    EFUSE_FIELD_KEY2,                               // Key2 or user data
    EFUSE_FIELD_KEY3,                               // Key3 or user data
    EFUSE_FIELD_KEY4,                               // Key4 or user data
    EFUSE_FIELD_KEY5,                               // Key5 or user data
    EFUSE_FIELD_SYS_DATA_PART2,                     // System data part 2 (reserved)
};

__BEGIN_DECLS

extern __attribute__((nothrow))
    int EFUSE_read_blob(enum EFUSE_field field, void *dst, size_t bits_size);

extern __attribute__((nothrow))
    int EFUSE_read_value(enum EFUSE_field field, unsigned *value);

static inline
    bool EFUSE_read_bit(enum EFUSE_field field)
    {
        unsigned val = 0;
        return EFUSE_read_value(field, &val) && 0 != val;
    }

__END_DECLS
#endif
