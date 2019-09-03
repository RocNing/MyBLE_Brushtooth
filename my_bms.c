#include "my_bms.h"
#include <stdlib.h>
#include <math.h>

//SAADC事件回调函数，因为是堵塞模式，所以不需要事件，这里定义了一个空的事件回调函数
void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
}

//初始化SAADC，配置使用的SAADC通道的参数
void My_saadc_Init(void)
{
	ret_code_t err_code;	
	//初始化SAADC，注册事件回调函数。
	err_code = nrf_drv_saadc_init(NULL, saadc_callback);
	APP_ERROR_CHECK(err_code);
	
	//初始化SAADC通道0
	//定义ADC通道配置结构体，并使用单端采样配置宏初始化，
	//NRF_SAADC_INPUT_AIN0是使用的模拟输入通道
	nrf_saadc_channel_config_t channel_config =
	NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(BATTERY_ADC_AIN);
	channel_config.gain = NRF_SAADC_GAIN1;
	err_code = nrfx_saadc_channel_init(BATTERY_ADC_CHN, &channel_config);
	APP_ERROR_CHECK(err_code);
}

static uint8_t saadc_battery_level_in_percent_li_ion(const uint16_t mvolts)
{
    uint8_t battery_level;
    
    if (mvolts >= BATT_FULL_POWER)
    {
        battery_level = 100;
    }
    else if (mvolts > BATT_AUTO_OFF)
    {
        battery_level = ((mvolts - BATT_AUTO_OFF)*100) / (BATT_FULL_POWER - BATT_AUTO_OFF); //0~100%
    }
    else
    {
        battery_level = 0;
    }

    return battery_level;
}

my_charge_t my_charge = {
	.batt_low_f = false,   //电量低标志位
	.batt_off_s = 0,       //没电标志位
	.batt_3v3_on_f = false,//电池充电到3v3解禁标志位
	.batt_mv = 0,
	.batt_mv_last = 0,
	.batt_percent = 0,
};
uint8_t charge_flag = NO_CHARGE;
void saadc_battery_read()
{
	uint8_t i;
	nrf_saadc_value_t batt_result, adc_value[10];//采样10次求平均值
	int32_t adc_value_sum = 0;
	uint16_t bat_max, bat_min;
	bat_max = 0;
	bat_min = 4500;

	for(i = 0; i < 10; i++)
	{
	//启动一次AD采样
		nrfx_saadc_sample_convert(BATTERY_ADC_CHN, &adc_value[i]);
		if(bat_max < adc_value[i])
			bat_max = adc_value[i];
		if(bat_min > adc_value[i])
			bat_min = adc_value[i];
		adc_value_sum += adc_value[i];
    }
	adc_value_sum = adc_value_sum - bat_max - bat_min;//去掉最大值和最小值
	batt_result = adc_value_sum / 8;
    NRF_LOG_INFO("Sample value = %d\r\n", batt_result);
	
	my_charge.batt_mv = BATTERY_ADC_RESULT_IN_MV(batt_result);
	
	if((my_charge.batt_mv_last == 0) || (abs(my_charge.batt_mv - my_charge.batt_mv_last) < 50))
		my_charge.batt_mv_last = my_charge.batt_mv;
	else
		my_charge.batt_mv = my_charge.batt_mv_last;
	NRF_LOG_INFO("Voltage = %d\r\n", my_charge.batt_mv);
    my_charge.batt_percent = saadc_battery_level_in_percent_li_ion(my_charge.batt_mv);//计算电量百分比
	
	if(work_flag == WORK_FLAG_CHARING)//是否充满
	{
		if(my_charge.batt_mv > BATT_FULL_POWER)
		{
			charge_flag = CHARGE_FULL;
			return;			
		}
	}
	
	if(my_charge.batt_mv < BATT_AUTO_OFF)//低电量自动关机
	{
		my_charge.batt_off_s = 1;
		//work_flag = WORK_FLAG_AUTO_OFF;
	}
	else if(my_charge.batt_mv >= 3300)//工作解禁
	{
		if(my_charge.batt_off_s)
		{
			my_charge.batt_off_s = 0;
			work_flag = WORK_FLAG_POWER_OFF;
		}
	}
	if((my_charge.batt_mv < (BATT_LOW_POWER - 50)) && (my_charge.batt_mv > BATT_AUTO_OFF))//电量低
	{
		my_charge.batt_low_f = true;
//		if(work_flag == WORK_FLAG_WORKING)//如果在工作状态
//		{
//			Led_flash_on();
//		}
//		if(work_flag == WORK_FLAG_TSET)//退出测试模式
//		{
//			work_flag = WORK_FLAG_POWER_OFF;
//		}
	}
	else if(my_charge.batt_mv > (BATT_LOW_POWER + 50))
		my_charge.batt_low_f = false;
	

}
