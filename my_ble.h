#ifndef __MY_BLE_H__
#define __MY_BLE_H__

#include <string.h>

//广播需要引用的头文件
#include "ble_advdata.h"
#include "ble_advertising.h"
//电源管理需要引用的头文件
#include "nrf_pwr_mgmt.h"
//SoftDevice handler configuration需要引用的头文件
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
//排序写入模块需要引用的头文件
#include "nrf_ble_qwr.h"
//GATT需要引用的头文件
#include "nrf_ble_gatt.h"
//连接参数协商需要引用的头文件
#include "ble_conn_params.h"
#include "ble_conn_state.h"
//串口透传需要引用的头文件
#include "my_ble_uarts.h"
//DFU需要引用的头文件
#include "nrf_power.h"
#include "nrf_bootloader_info.h"
#include "ble_dfu.h"
#include "nrf_dfu_ble_svci_bond_sharing.h"
#include "nrf_svci_async_function.h"
#include "nrf_svci_async_handler.h"

//Log需要引用的头文件
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
//APP定时器需要引用的头文件
#include "app_timer.h"

#include "common.h"

#define BLE_UUID_USR1_SERVICE           0xFFB0    //自定义UUID，用于BLE串口服务

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)   // 最小连接间隔 (0.2 秒) 
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)   // 最大连接间隔 (0.65 秒) 
#define SLAVE_LATENCY                   0                                  // 从机延迟 
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)    // 监督超时(4 秒) 

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)              // 定义首次调用sd_ble_gap_conn_param_update()函数更新连接参数延迟时间（5秒）
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)             // 定义每次调用sd_ble_gap_conn_param_update()函数更新连接参数的间隔时间（30秒）
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                  // 定义放弃连接参数协商前尝试连接参数协商的最大次数（3次）

#define APP_ADV_INTERVAL                320                                // 广播间隔 (500 ms)，单位0.625 ms 
#define APP_ADV_DURATION                0                                  // 广播持续时间，单位：10ms。设置为0表示不超时 

#define APP_BLE_OBSERVER_PRIO           3               //应用程序BLE事件监视者优先级，应用程序不能修改该数值
#define APP_BLE_CONN_CFG_TAG            1               //SoftDevice BLE配置标志

#define TX_POWER_LEVEL                      (4)    //(accepted values are -40, -30, -20, -16, -12, -8, -4, 0, and 4 dBm).
                                                   //The default TX power level of S110 is 0 dBm


//用于stack dump的错误代码，可以用于栈回退时确定堆栈位置
#define DEAD_BEEF                       0xDEADBEEF 


void My_ble_Init(void);//蓝牙初始化
void advertising_start(void);//启动广播，该函数所用的模式必须和广播初始化中设置的广播模式一样
uint32_t my_ble_uarts_data_send(uint8_t   * p_data, uint16_t  * p_length);

#endif
