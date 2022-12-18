/* 
 * File:   drv_nvm_avrDX.h
 * Author: pablo
 *
 * Created on 25 de marzo de 2022, 10:18 AM
 */

#ifndef DRV_NVM_AVRDX_H
#define	DRV_NVM_AVRDX_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <compiler.h>

#define BOOTLOADER_SECTION __attribute__((section(".bootloader")))

#ifdef __cplusplus
extern "C" {
#endif

/** Datatype for flash address */
typedef uint32_t flash_adr_t;

/** Datatype for EEPROM address */
typedef uint16_t eeprom_adr_t;

/** Datatype for return status of NVMCTRL operations */
typedef enum {
	NVM_OK    = 0, ///< NVMCTRL free, no write ongoing.
	NVM_ERROR = 1, ///< NVMCTRL operation retsulted in error
	NVM_BUSY  = 2, ///< NVMCTRL busy, write ongoing.
} nvmctrl_status_t;

int8_t FLASH_0_init();

uint8_t FLASH_0_read_eeprom_byte(eeprom_adr_t eeprom_adr);

nvmctrl_status_t FLASH_0_write_eeprom_byte(eeprom_adr_t eeprom_adr, uint8_t data);

void FLASH_0_read_eeprom_block(eeprom_adr_t eeprom_adr, uint8_t *data, size_t size);

nvmctrl_status_t FLASH_0_write_eeprom_block(eeprom_adr_t eeprom_adr, uint8_t *data, size_t size);

bool FLASH_0_is_eeprom_ready();

uint8_t FLASH_0_read_flash_byte(flash_adr_t flash_adr);

nvmctrl_status_t FLASH_0_write_flash_byte(flash_adr_t flash_adr, uint8_t *ram_buffer, uint8_t data);

nvmctrl_status_t FLASH_0_erase_flash_page(flash_adr_t flash_adr);

nvmctrl_status_t FLASH_0_write_flash_page(flash_adr_t flash_adr, uint8_t *data);

nvmctrl_status_t FLASH_0_write_flash_block(flash_adr_t flash_adr, uint8_t *data, size_t size, uint8_t *ram_buffer);

nvmctrl_status_t FLASH_0_write_flash_stream(flash_adr_t flash_adr, uint8_t data, bool finalize);

#ifdef __cplusplus
}
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* DRV_NVM_AVRDX_H */

