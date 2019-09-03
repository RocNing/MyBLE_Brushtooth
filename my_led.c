#include "my_led.h"

APP_TIMER_DEF(led_timer_id); 

#define FLASH1_ON_TIME_MS       200
#define FLASH1_OFF_TIME_MS      400
bool led_charge_flash_f = false;

#define MYPIN_MASK(_pin)  /*lint -save -e504 */                     \
                        (1u << (uint32_t)((_pin) & (~P0_PIN_NUM))) \
                        /*lint -restore    */

static void led_timer_hanndler(void * p_context)
{
    uint32_t err_code;

    err_code = app_timer_stop(led_timer_id);
    APP_ERROR_CHECK(err_code);

    if(led_charge_flash_f)
    {
        led_charge_flash_f = false;
		LED_POWER_OFF;
        err_code = app_timer_start(led_timer_id, APP_TIMER_TICKS(FLASH1_OFF_TIME_MS), NULL);
        APP_ERROR_CHECK(err_code);
    }
    else
    {
		led_charge_flash_f = true;
		LED_POWER_ON;
		err_code = app_timer_start(led_timer_id, APP_TIMER_TICKS(FLASH1_ON_TIME_MS), NULL);
		APP_ERROR_CHECK(err_code);
    }
}

bool isLED_FLASH_ON = false;
bool isLED_FLASH_OFF = false;
void Led_flash_on()
{
	if(isLED_FLASH_ON)//led闪光已开
		return;
	else
		isLED_FLASH_ON = true;
	
	uint32_t err_code;
	
	isLED_FLASH_OFF = false; //清零标志位
	led_charge_flash_f = true;
	LED_POWER_ON;
	err_code = app_timer_start(led_timer_id, APP_TIMER_TICKS(FLASH1_ON_TIME_MS), NULL);
	APP_ERROR_CHECK(err_code);
}

void Led_flash_off()
{
	if(isLED_FLASH_OFF)//led闪光已关
		return;
	else
		isLED_FLASH_OFF = true;
	
	uint32_t err_code;

	isLED_FLASH_ON = false; //标志位清零
	led_charge_flash_f = false;
	LED_POWER_OFF;
    err_code = app_timer_stop(led_timer_id);
    APP_ERROR_CHECK(err_code);
}

void My_led_Init(void)
{
	uint32_t err_code;
	err_code = app_timer_create(&led_timer_id, APP_TIMER_MODE_REPEATED, led_timer_hanndler);
    APP_ERROR_CHECK(err_code);
	
	nrf_gpio_cfg_output(LED1);
	nrf_gpio_cfg_output(LED2);
	nrf_gpio_cfg_output(LED3);
	nrf_gpio_cfg_output(LED4);
	nrf_gpio_cfg_output(LED5);
	nrf_gpio_cfg_output(LED_BLE);
	nrf_gpio_cfg_output(LED_WORK);
	nrf_gpio_cfg_output(LED_POWER);
	LED1_OFF;
	LED2_OFF;
	LED3_OFF;
	LED4_OFF;
	LED5_OFF;
	LED_BLE_OFF;
	LED_WORK_OFF;
	LED_POWER_OFF;
}

bool is_ledblink_on = false;//ledblink是否处于打开状态
void Led_softblink_on(void)
{
	if(is_ledblink_on)
		return;
	
	uint32_t err_code;
    uint32_t leds_mask = 0;
    led_sb_init_params_t led_sb_init_param;
	
	is_ledblink_on = true;
	
	leds_mask |= MYPIN_MASK(LED_POWER); 
	
	led_sb_init_param.active_high = 0;          /**< Activate negative polarity. */       
	led_sb_init_param.duty_cycle_max = 220;     /**< Maximum impulse width.最大220 */
	led_sb_init_param.duty_cycle_min = 20;      /**< Minimum impulse width. */
	led_sb_init_param.duty_cycle_step = 1;      /**< Cycle step. */
	led_sb_init_param.off_time_ticks = 200;     /**< 1S, Ticks to stay in low impulse state. */
	led_sb_init_param.on_time_ticks = 400;        /**< Ticks to stay in high impulse state. */
	led_sb_init_param.leds_pin_bm = leds_mask;                      /**< Mask of used LEDs. */
	led_sb_init_param.p_leds_port = LED_SB_INIT_PARAMS_LEDS_PORT;   /**< Port of used LEDs mask. */
	err_code = led_softblink_init(&led_sb_init_param);
	APP_ERROR_CHECK(err_code);
	err_code = led_softblink_start(leds_mask);
	APP_ERROR_CHECK(err_code);
}

void Led_softblink_off(void)
{
	if(is_ledblink_on == false) 
		return;
	is_ledblink_on = false;
	uint32_t err_code;
	
	err_code = led_softblink_uninit();
	APP_ERROR_CHECK(err_code);
}

void Led_quick_flash30(void)
{
	uint8_t i;
	
	for(i=0;i<30;i++)
	{
		LED_POWER_ON;
		nrf_delay_ms(80);
		LED_POWER_OFF;
		nrf_delay_ms(40);
	}
}

void Led_all_Off(void)
{
	Led_flash_off();//关闪光
	LED_POWER_OFF;
	LED_MODE_OFF;
	LED_WORK_OFF;
}


















