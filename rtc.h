/*
 * tuya_event.h
 *
 *  Created on: 
 *      Author: gyh
 */

#ifndef APP_RTC_H_
#define APP_RTC_H_

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif




void app_rtc_config(void);

uint32_t get_timestamp(void);

void set_timestamp(uint32_t timestamp);


#ifdef __cplusplus
}
#endif

#endif // 

