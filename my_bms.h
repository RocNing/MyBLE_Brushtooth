#ifndef __MY_BMS_H__
#define __MY_BMS_H__

//SAADC引用的头文件
#include "nrf_drv_saadc.h"
#include "common.h"


//初始化SAADC，配置使用的SAADC通道的参数
void My_saadc_Init(void);
void saadc_battery_read(void);

#endif 
