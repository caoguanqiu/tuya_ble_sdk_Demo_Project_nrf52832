
#include "nrf_delay.h"
#include "nrf_soc.h"
//#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_fstorage.h"
#include "nrf_atomic.h"
#include "flash.h"
#include "nrf_log.h"
#include "tuya_ble_log.h"

#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"
#else
#include "nrf_drv_clock.h"
#include "nrf_fstorage_nvmc.h"
#endif

#define current_flash_op_index_max    10


typedef struct
{
    uint32_t value;
    uint32_t flag;
}current_flash_op_array_t;

static nrf_atomic_u32_t current_flash_op_cnt = 0;
//static nrf_atomic_u32_t current_flash_op_cnt_check = 1;
static volatile current_flash_op_array_t current_flash_op_array[current_flash_op_index_max];
static nrf_atomic_u32_t current_flash_op_index = 0;

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);


static uint32_t get_current_flash_op_index(void)
{
    uint32_t value;
    value = nrf_atomic_u32_add(&current_flash_op_index,1);
    if(value>=current_flash_op_index_max)
    {
        value = nrf_atomic_u32_store(&current_flash_op_index,0);
    }

    return value;
}

static uint32_t get_current_flash_op_cnt(void)
{
    uint32_t value;
    value = nrf_atomic_u32_add(&current_flash_op_cnt,1);
    if((value==0xFFFFFFFF)||(value==0))
    {
        value = nrf_atomic_u32_store(&current_flash_op_cnt,1);
    }

    return value;
}

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = TUYA_FLASH_SETTING_BASE_ADDR,
    .end_addr   = TUYA_FLASH_APP_DATA_END_ADDR,
};


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    current_flash_op_array_t *p_op;
    
    p_op = (current_flash_op_array_t *)p_evt->p_param;
    
    if (p_evt->result != NRF_SUCCESS)
    {
        p_op->flag = 0xFFFFFFFF;
        TUYA_BLE_LOG_ERROR("--> Event received: ERROR while executing an fstorage operation. %d bytes at address 0x%x.",p_evt->len, p_evt->addr);
        return;
    }

    switch (p_evt->id)
    {
    case NRF_FSTORAGE_EVT_WRITE_RESULT:
    {
        p_op->flag = 1;
        TUYA_BLE_LOG_DEBUG("--> Event received: wrote %d bytes at address 0x%x, current_flash_op_cnt = %d",
                   p_evt->len, p_evt->addr,p_op->value);
    }
    break;

    case NRF_FSTORAGE_EVT_ERASE_RESULT:
    {
        p_op->flag = 1;
        TUYA_BLE_LOG_DEBUG("--> Event received: erased %d page from address 0x%x, current_flash_op_cnt = %d",
                   p_evt->len, p_evt->addr,p_op->value);
    }
    break;

    default:
        break;
    }
}


void  nrf_fstorage_port_init(void)
{
    ret_code_t rc;

    nrf_fstorage_api_t * p_fs_api;

//    current_flash_op_cnt = 0;
//    current_flash_op_cnt_check = 0;
    nrf_atomic_u32_store(&current_flash_op_cnt,0);
    nrf_atomic_u32_store(&current_flash_op_index,0);
    memset((void *)&current_flash_op_array[0],0,sizeof(current_flash_op_array));
#ifdef SOFTDEVICE_PRESENT
    p_fs_api = &nrf_fstorage_sd;
#else
    NRF_LOG_INFO("SoftDevice not present.");
    NRF_LOG_INFO("Initializing nrf_fstorage_nvmc implementation...");
    /* Initialize an fstorage instance using the nrf_fstorage_nvmc backend.
     * nrf_fstorage_nvmc uses the NVMC peripheral. This implementation can be used when the
     * SoftDevice is disabled or not present.
     *
     * Using this implementation when the SoftDevice is enabled results in a hardfault. */
    p_fs_api = &nrf_fstorage_nvmc;
#endif

    rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);

}


uint32_t nrf_fstorage_port_erase_sector(uint32_t addr,bool block)
{
    uint32_t cur_op_cnt_erase = get_current_flash_op_index();
    uint32_t i=0;
    
    memset(&current_flash_op_array[cur_op_cnt_erase],0,sizeof(current_flash_op_array_t));
    current_flash_op_array[cur_op_cnt_erase].value = get_current_flash_op_cnt(); 
  //  current_flash_op_array[cur_op_cnt_erase] = cur_op_cnt_erase_value;

   // nrf_atomic_u32_store(&current_flash_op_cnt_check,0);

    ret_code_t rc = nrf_fstorage_erase(
                        &fstorage,   /* The instance to use. */
                        addr,     /* The address of the flash pages to erase. */
                        1, /* The number of pages to erase. */
                        &current_flash_op_array[cur_op_cnt_erase]  /* Optional parameter, backend-dependent. */
                    );
    if (rc == NRF_SUCCESS)
    {
        /* The operation was accepted.
           Upon completion, the NRF_FSTORAGE_ERASE_RESULT event
           is sent to the callback function registered by the instance. */
		if(!block)
		{
			return rc;
		}
		
    }
    else
    {
        return rc; /* Handle error.*/
    }
    
//    tuya_log_d("SW1 priority = %d ",NVIC_GetPriority((IRQn_Type)SWI1_IRQn)); 
//    tuya_log_d("SW2 priority = %d ",NVIC_GetPriority((IRQn_Type)SWI2_IRQn)); 
//    tuya_log_d("SW5 priority = %d ",NVIC_GetPriority((IRQn_Type)SWI5_IRQn)); 
    
    i = 0;
    while (current_flash_op_array[cur_op_cnt_erase].flag != 1) //等待操作完成
    {
        nrf_delay_ms(2);
        if((i++)>=500)
        {
            TUYA_BLE_LOG_ERROR("tuya flash erase timeout");
            rc = NRF_ERROR_TIMEOUT;
            break;
        }
        else if(current_flash_op_array[cur_op_cnt_erase].flag==0xFFFFFFFF)
        {
            TUYA_BLE_LOG_ERROR("tuya flash erase error");
            rc = NRF_ERROR_TIMEOUT;
            break;
        }
        else
        {
            
        }
    }
    if(rc == 0)
    {
        TUYA_BLE_LOG_DEBUG("tuya flash erase success");
    }
    return rc;
}

uint32_t nrf_fstorage_port_read_bytes(uint32_t addr, uint8_t *buf,uint32_t len)
{
//    memcpy(buf,(u8 *)addr,len);
//    return 0;

    ret_code_t rc = nrf_fstorage_read(
                        &fstorage,   /* The instance to use. */
                        addr,     /* The address in flash where to read data from. */
                        buf,        /* A buffer to copy the data into. */
                        len  /* Lenght of the data, in bytes. */
                    );
    if (rc == NRF_SUCCESS)
    {
        /* The operation was accepted.
           Upon completion, the NRF_FSTORAGE_READ_RESULT event
           is sent to the callback function registered by the instance.
           Once the event is received, it is possible to read the contents of 'number'. */
    }
    else
    {
        /* Handle error.*/
    }

    return rc;

}


uint32_t nrf_fstorage_port_write_bytes(uint32_t addr, uint8_t *buf,uint32_t len,bool block)
{
    uint32_t cur_op_cnt_write = get_current_flash_op_index();
    uint32_t i=0;
    
    memset(&current_flash_op_array[cur_op_cnt_write],0,sizeof(current_flash_op_array_t));
    current_flash_op_array[cur_op_cnt_write].value = get_current_flash_op_cnt();
   // current_flash_op_array[cur_op_cnt_write] = cur_op_cnt_write_value;

   // nrf_atomic_u32_store(&current_flash_op_cnt_check,0);
    
    ret_code_t rc = nrf_fstorage_write(
                        &fstorage,   /* The instance to use. */
                        addr,     /* The address in flash where to read data from. */
                        buf,        /* A buffer to copy the data into. */
                        len,  /* Lenght of the data, in bytes. */
                        &current_flash_op_array[cur_op_cnt_write]
                    );
    if (rc == NRF_SUCCESS)
    {
        /* The operation was accepted.
           Upon completion, the NRF_FSTORAGE_READ_RESULT event
           is sent to the callback function registered by the instance.
           Once the event is received, it is possible to read the contents of 'number'. */
		
		if(!block)
		{
			return rc;
		}		
		
    }
    else
    {
        TUYA_BLE_LOG_ERROR("nrf_fstorage_write error,rc=%d",rc);
        return rc; /* Handle error.*/
    }

    i = 0;
    while (current_flash_op_array[cur_op_cnt_write].flag!=1) //等待操作完成
    {
        nrf_delay_ms(2);
        if((i++)>=500)
        {
            TUYA_BLE_LOG_ERROR("tuya flash write timeout");
            rc = NRF_ERROR_TIMEOUT;
            break;
        }
        else if(current_flash_op_array[cur_op_cnt_write].flag==0xFFFFFFFF)
        {
            TUYA_BLE_LOG_ERROR("tuya flash write error");
            rc = NRF_ERROR_TIMEOUT;
            break;
        }
        else
        {
            
        }
    }
    if(rc == 0)
    {
        TUYA_BLE_LOG_DEBUG("tuya flash write success");
    }
    return rc;
}


