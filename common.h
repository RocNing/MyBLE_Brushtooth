#ifndef __MY_COMMON_H__
#define __MY_COMMON_H__

//引用的C库头文件
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
//Log需要引用的头文件
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
//SAADC引用的头文件
#include "nrf_drv_saadc.h"
//APP定时器需要引用的头文件
#include "app_timer.h"
//APP按键需要引用的头文件
#include "app_button.h"
#include "nrf_delay.h"
//看门狗
#include "nrfx_wdt.h"
#include "nrf_drv_clock.h"


#define DEVICE_NAME              "TCSY"                             // 设备名称字符串 
#define MANUFACTURER_NAME        "GDE"    /**< Manufacturer. Will be passed to Device Information Service. */
#define MODEL_NAME               "H1601"
//固件版本
#define FW_VER_1            1 
#define FW_VER_2            8   //'4':SDK6.1; '5':SDK10.0; '6':SDK12.3,'8':SDK15.2
#define FW_VER_3            02  //<=99 //vtest_changed_20181129
//硬件版本
#define HW_VER_1            3 
#define HW_VER_2            0
#define HW_VER_3            00  



extern volatile uint32_t working_sec;
extern volatile uint32_t rtc_sec;

/******************工作状态*******************/
enum    //0-关机，1-工作，2-关机Logo, 3-开机Logo, 4-关机处理， 5-开机处理，6-唤醒处理，7-睡眠状态
{
    WORK_FLAG_POWER_OFF = 0,
    WORK_FLAG_WORKING,                  //正常上电工作模式
    WORK_FLAG_WORKING_LOW_BATT,         //低电量工作模式
	WORK_FLAG_AUTO_OFF,                 //低电量自动关机
	WORK_FLAG_CHARING,                  //充电模式
	WORK_FLAG_TSET,                     //测试模式
}; 
extern uint8_t work_flag;

/******************电池充电*******************/
enum
{
	NO_CHARGE = 0,
	CHARGING,
	CHARGE_FULL,
};
extern uint8_t charge_flag;

typedef struct
{
	bool batt_low_f;   //电量低标志位
	uint8_t batt_off_s;//没电标志位
	bool batt_3v3_on_f;//电池充电到3v3解禁标志位
	
	uint16_t batt_mv;  //电池电量
	uint16_t batt_mv_last;//上一次电池电压
	uint8_t  batt_percent;//电量百分比
} my_charge_t;
extern my_charge_t my_charge;

/******************按键判断*******************/
typedef struct
{
	/**@brief 按键模式切换：
	*0：关机
	*1：工作模式1；2：工作模式2；3：工作模式3；4：工作模式4；5：工作模式5*/
	uint8_t key_mode;
	bool iskeypre_f;//按键是否处于按下标志位
	uint16_t key_Pre_cnt;//按键按下时间计数
	bool key_mode_change_f;//模式切换标志位
} my_key_t;
extern my_key_t my_key;

typedef struct
{
	bool                   is_test_mode;//是否进入了测试模式
	bool                   is_renmender;//是否处于振动提醒
	bool                   is_end_rendender;
	
	uint8_t                mode;    //马达工作模式
	uint16_t               freq;    //马达频率
	uint8_t				   duty;    //马达占空比
	uint8_t                strength;//马达强度
	uint16_t               run_time;//设置刷牙时间
	uint16_t               dur_time;//实际刷牙时间
	uint8_t                start_delay;//启动延时
} my_motor_t;
extern my_motor_t my_motor;
extern uint16_t motor_data[7][4];
/******************BLE控制*******************/
typedef struct
{
	bool is_connected; //是否蓝牙连接上app
	bool is_ble_con;   //蓝牙控制
	bool is_send_device_info;//上报设备信息
	bool is_send_his_data;   //上报历史数据
	
	uint8_t device_info_s;
	uint8_t his_data_s;
	uint8_t mac_addr[6];
} my_ble_t;
extern my_ble_t my_ble;

//FDS异步操作标志结构体
typedef struct
{
	bool hisdata_record_update;    //历史数据记录更新标志
	bool devicetime_record_update; //desp_record记录更新标志
	bool read;       //读记录标志
	bool gc;         //碎片收集标志
	bool busy;       //FDS忙标志
}my_fds_info_t;
extern my_fds_info_t my_fds_info;//定义FDS异步操作标志结构体

//记录历史数据
typedef struct
{
	uint32_t end_sec;    //刷牙完成时间
	uint8_t  mode;       //刷牙模式
	uint16_t dur_time;   //刷牙实际用时
	uint16_t run_time;   //刷牙设置用时
}__attribute__((aligned(4)))his_data_t;
extern his_data_t his_data;

/***********************LED定义************************/
#define LED_NUM    5
//定义LED使用的管脚
#define LED1 		3      //工作模式1指示LED
#define LED2        7
#define LED3 	    19
#define LED4 	    30
#define LED5 	    29    
#define LED_BLE     9     //蓝牙指示LED
#define LED_WORK    13
#define LED_POWER   28 	  //电源指示LED


#define LED1_ON  nrf_gpio_pin_clear(LED1)   //输出低电平，灯亮
#define LED1_OFF nrf_gpio_pin_set(LED1)     //输出高电平，灯灭
#define LED2_ON  nrf_gpio_pin_clear(LED2)   //输出低电平，灯亮
#define LED2_OFF nrf_gpio_pin_set(LED2)     //输出高电平，灯灭
#define LED3_ON  nrf_gpio_pin_clear(LED3)   //输出低电平，灯亮
#define LED3_OFF nrf_gpio_pin_set(LED3)     //输出高电平，灯灭
#define LED4_ON  nrf_gpio_pin_clear(LED4)   //输出低电平，灯亮
#define LED4_OFF nrf_gpio_pin_set(LED4)     //输出高电平，灯灭
#define LED5_ON  nrf_gpio_pin_clear(LED5)   //输出低电平，灯亮
#define LED5_OFF nrf_gpio_pin_set(LED5)     //输出高电平，灯灭
#define LED_WORK_ON  nrf_gpio_pin_clear(LED_WORK)   //输出低电平，灯亮
#define LED_WORK_OFF nrf_gpio_pin_set(LED_WORK)     //输出高电平，灯灭
#define LED_BLE_ON  nrf_gpio_pin_clear(LED_BLE)   //输出低电平，灯亮
#define LED_BLE_OFF nrf_gpio_pin_set(LED_BLE)     //输出高电平，灯灭
#define LED_POWER_ON  nrf_gpio_pin_clear(LED_POWER)   //输出低电平，灯亮
#define LED_POWER_OFF nrf_gpio_pin_set(LED_POWER)     //输出高电平，灯灭

#define LED_MODE_OFF LED1_OFF;LED2_OFF;LED3_OFF;LED4_OFF;LED5_OFF;
#define LED_MODE_1   LED1_ON;LED2_OFF;LED3_OFF;LED4_OFF;LED5_OFF;
#define LED_MODE_2   LED1_OFF;LED2_ON;LED3_OFF;LED4_OFF;LED5_OFF;
#define LED_MODE_3   LED1_OFF;LED2_OFF;LED3_ON;LED4_OFF;LED5_OFF;
#define LED_MODE_4   LED1_OFF;LED2_OFF;LED3_OFF;LED4_ON;LED5_OFF;
#define LED_MODE_5   LED1_OFF;LED2_OFF;LED3_OFF;LED4_OFF;LED5_ON;
#define LED_MODE_ALL LED1_ON;LED2_ON;LED3_ON;LED4_ON;LED5_ON;

#define LED_MODE_ON(mode)   switch(mode)		\
							{					\
								case 1:			\
									LED_MODE_1;	\
									break;		\
								case 2:			\
									LED_MODE_2;	\
									break;		\
								case 3:			\
									LED_MODE_3;	\
									break;		\
								case 4:			\
									LED_MODE_4;	\
									break;		\
								case 5:			\
									LED_MODE_5;	\
									break;		\
							}					\

/***********************BSP定义************************/
/* 
* CHARGER
*/
#define CHG_IN_PIN_NO           23
#define CHG_IN_SET              nrf_gpio_cfg_input(CHG_IN_PIN_NO, NRF_GPIO_PIN_NOPULL);
#define CHG_IN_RD               nrf_gpio_pin_read(CHG_IN_PIN_NO)
#define STB_IN_PIN_NO           24
#define STB_IN_SET              nrf_gpio_cfg_input(STB_IN_PIN_NO, NRF_GPIO_PIN_NOPULL);
#define STB_IN_RD               nrf_gpio_pin_read(STB_IN_PIN_NO)

#define BATT_FULL_POWER 4170   //电量充满标志位 
#define BATT_LOW_POWER  3500   //3.5V，低电量提醒
#define BATT_AUTO_OFF   3000   //低于3V自动关机

/* 
* SAADC
*/
#define BATTERY_ADC_CHN     0
#define BATTERY_ADC_AIN     NRF_SAADC_INPUT_AIN0
#define BATTERY_ADC_RESULT_IN_MV(adc_value) (((uint32_t)(adc_value) * 600 * 1155) / (1024 * 150)) + 10

/***********************按键定义************************/
#define MY_BUTTON_1      6 //定义按键1引脚
//#define BUTTON_2       16//定义按键2引脚，未用
#define BUTTON_DETECTION_DELAY    APP_TIMER_TICKS(50) //按键延时检测，50ms


#endif
