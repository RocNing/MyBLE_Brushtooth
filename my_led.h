#ifndef __MY_LED_H__
#define __MY_LED_H__

#include "common.h"
#include "led_softblink.h"


void My_led_Init(void);//Led≥ı ºªØ
void Led_all_Off(void);
void Led_flash_on(void);
void Led_flash_off(void);
void Led_softblink_on(void);
void Led_softblink_off(void);
void Led_quick_flash30(void);

#endif 


