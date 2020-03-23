#include <stdlib.h>
#include "nrf.h"
#include "nrf_peripherals.h"
#include "nrf_soc.h"
#include "nrf52.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
//#include "tuya_common.h"
#include "rtc.h"

static uint8_t m_time_count_1s =0;

volatile uint32_t m_timestamp = 0;

/*

const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(2); 


static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if(int_type == NRF_DRV_RTC_INT_COMPARE0)
    {
    }
    else if (int_type == NRF_DRV_RTC_INT_TICK)
    {
        if(m_time_count_1s >=8 )//
        {
            m_time_count_1s = 0;
            m_timestamp ++;
            
            //NRF_LOG_INFO(":%d/%d/%d,  weeday %d",m_time.tm_year, m_time.tm_mon,m_time.tm_mday,  m_time.tm_wday);
            //NRF_LOG_INFO("%d-%d-%d", m_time.tm_hour, m_time.tm_min, m_time.tm_sec);
        }
        else
        {
            m_time_count_1s++;
        }
    }
}



void app_rtc_config(void)
{
    uint32_t err_code;
    m_time_count_1s = 0;    
    //Initialize RTC instance
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    config.prescaler = 4095; 
    err_code = nrf_drv_rtc_init(&rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    //Enable tick event & interrupt
    nrf_drv_rtc_tick_enable(&rtc, true);
    
    //Power on RTC instance
    nrf_drv_rtc_enable(&rtc);
}
*/

#define RTC2_IRQ_PRI            6 

#define MAX_RTC_TASKS_DELAY     47  

static bool      m_rtc2_running;

/**@brief Function for initializing the RTC1 counter.
 *
 * @param[in] prescaler   Value of the RTC1 PRESCALER register. Set to 0 for no prescaling.
 */
static void rtc2_init(uint32_t prescaler)
{
    NRF_RTC2->PRESCALER = prescaler;
    NVIC_SetPriority(RTC2_IRQn, RTC2_IRQ_PRI);
}


/**@brief Function for starting the RTC1 timer.
 */
static void rtc2_start(void)
{
    NRF_RTC2->EVTENSET = RTC_EVTEN_TICK_Msk;
    NRF_RTC2->INTENSET = RTC_EVTEN_TICK_Msk;

    NVIC_ClearPendingIRQ(RTC2_IRQn);
    NVIC_EnableIRQ(RTC2_IRQn);

    NRF_RTC2->TASKS_START = 1;
    nrf_delay_us(MAX_RTC_TASKS_DELAY);

    m_rtc2_running = true;
}


/**@brief Function for stopping the RTC1 timer.
 */
static void rtc2_stop(void)
{
    NVIC_DisableIRQ(RTC2_IRQn);

    NRF_RTC2->EVTENCLR = RTC_EVTEN_TICK_Msk;
    NRF_RTC2->INTENCLR = RTC_EVTEN_TICK_Msk;

    NRF_RTC2->TASKS_STOP = 1;
    nrf_delay_us(MAX_RTC_TASKS_DELAY);

    NRF_RTC2->TASKS_CLEAR = 1;
   // m_ticks_latest        = 0;
    nrf_delay_us(MAX_RTC_TASKS_DELAY);

    m_rtc2_running = false;
}



/**@brief Function for handling the RTC1 interrupt.
 *
 * @details Checks for timeouts, and executes timeout handlers for expired timers.
 */
void RTC2_IRQHandler(void)
{
    // Clear all events (also unexpected ones)
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
    NRF_RTC2->EVENTS_COMPARE[1] = 0;
    NRF_RTC2->EVENTS_COMPARE[2] = 0;
    NRF_RTC2->EVENTS_COMPARE[3] = 0;
    NRF_RTC2->EVENTS_TICK       = 0;
    NRF_RTC2->EVENTS_OVRFLW     = 0;

    // Check for expired timers
        if(m_time_count_1s >=8 )//
        {
            m_time_count_1s = 0;
            m_timestamp ++;
            
    //        tuya_log_d("current m_timestamp = %d\n",m_timestamp);

        }
        else
        {
            m_time_count_1s++;
        }
}


void app_rtc_config(void)
{
    uint32_t err_code;
    m_time_count_1s = 0;    
    //Initialize RTC instance
    rtc2_init(4095);
	rtc2_start();
}



uint32_t get_timestamp(void)
{
    return m_timestamp;
}


void set_timestamp(uint32_t timestamp)
{
    CRITICAL_REGION_ENTER();
    m_time_count_1s = 0;
    m_timestamp = timestamp;
    CRITICAL_REGION_EXIT();
}

