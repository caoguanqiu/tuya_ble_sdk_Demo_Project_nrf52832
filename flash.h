
#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>
#include <stdbool.h>

#include "ota.h"

#define TUYA_FLASH_SETTING_BASE_ADDR   APP_NEW_FW_END_ADR

#define TUYA_FLASH_APP_DATA_END_ADDR     0x78000

void  nrf_fstorage_port_init(void);

uint32_t nrf_fstorage_port_erase_sector(uint32_t addr,bool block);

uint32_t nrf_fstorage_port_write_bytes(uint32_t addr, uint8_t *buf,uint32_t len,bool block);

uint32_t nrf_fstorage_port_read_bytes(uint32_t addr, uint8_t *buf,uint32_t len);



#endif

