#include "common.h"
#include "my_led.h"
#include "my_key.h"
#include "my_bms.h"
#include "my_fds.h"
#include "my_ble.h"
#include "my_ble_uarts.h"
#include "my_motor.h"

uint8_t work_flag = WORK_FLAG_POWER_OFF;//工作状态标志位，默认关机
//RTC
APP_TIMER_DEF(m_second_timer_id);
#define SECOND_INTERVAL         APP_TIMER_TICKS(1000)//秒计时应用定时器间隔 1秒
//看门狗通道
nrfx_wdt_channel_id m_wdt_channel_id;
//WDT中断中可花费的最长的时间是2个32.768KHz时钟周期，之后系统复位
void wdt_event_handler(void)
{}
//看门狗初始化，初始化完成后会启动看门狗，看门狗一旦启动后就无法停止
void My_wdt_Init(void)
{
    uint32_t err_code = NRF_SUCCESS;
    
    //定义WDT配置结构体并使用
    nrfx_wdt_config_t config = NRFX_WDT_DEAFULT_CONFIG;
	  //初始化WDT
    err_code = nrfx_wdt_init(&config, wdt_event_handler);
    APP_ERROR_CHECK(err_code);
	  //申请喂狗通道，也就是使用哪个
    err_code = nrfx_wdt_channel_alloc(&m_wdt_channel_id);
    APP_ERROR_CHECK(err_code);
	  //启动WDT
    nrfx_wdt_enable();       
}
//时钟相关
volatile uint32_t working_sec = 0;//上电工作时间
volatile uint32_t systick = 0;//上电工作时间
volatile uint32_t rtc_sec = 0;//时钟

//秒计时 应用定时器事件回调函数
static void second_updata_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
	 //更新tm格式时间变量和时间更新标志
    //CAL_updata();
	rtc_sec++;
	systick++;
	
	if((systick % 5 == 0) && (my_key.iskeypre_f == false))//每10s采样一次电池电压
		saadc_battery_read();  //采样电池电压
	
	if(my_key.iskeypre_f)
	{
		my_key.key_Pre_cnt++;
		if(my_key.key_Pre_cnt == 2)//长按2s关机
		{			
			my_key.key_mode--;//保持下次开机工作模式不变
			//if(work_flag == WORK_FLAG_WORKING)
				//My_device_sleep();
			work_flag = WORK_FLAG_POWER_OFF;
		}
		if(my_key.key_Pre_cnt >= 8)//长按8s进入测试模式
		{
			if(my_charge.batt_low_f == false)//低电量不进入测试模式
				work_flag = WORK_FLAG_TSET;
		}
	}
	
	if(work_flag == WORK_FLAG_WORKING)
	{
		working_sec++;
	}
}

//初始化APP定时器模块
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();//初始化APP定时器模块
    APP_ERROR_CHECK(err_code);             //检查返回值

    //加入创建用户定时任务的代码，创建用户定时任务。 
	err_code = app_timer_create(&m_second_timer_id, APP_TIMER_MODE_REPEATED, second_updata_handler);
    APP_ERROR_CHECK(err_code);
	err_code = app_timer_start(m_second_timer_id, SECOND_INTERVAL, NULL);//启动RTC定时器
    APP_ERROR_CHECK(err_code);
	
	motor_timer_creat();
}
//初始化电源管理模块
static void power_management_init(void)
{
    ret_code_t err_code;
	  //初始化电源管理
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

static void log_init(void)
{
    //初始化log程序模块
	ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    //设置log输出终端（根据sdk_config.h中的配置设置输出终端为UART或者RTT）
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//空闲状态处理函数。如果没有挂起的日志操作，则睡眠直到下一个事件发生后唤醒系统
static void idle_state_handle(void)
{
    //处理挂起的log
	if (NRF_LOG_PROCESS() == false)
    {  
		nrf_pwr_mgmt_run();//运行电源管理，该函数需要放到主循环里面执行
    }
}


void Device_Work_Loop(void);
//主函数
int main(void)
{
	NRF_UICR->NFCPINS = 0;
	uint32_t                err_code;
	err_code = ble_dfu_buttonless_async_svci_init();
    APP_ERROR_CHECK(err_code);
	
	log_init();             //初始化log程序模块	
	timers_init();          //初始化APP定时器
	power_management_init();//初始化电源管理
	
	My_ble_Init();
	My_led_Init();
	buttons_init();
	My_saadc_Init();
	My_wdt_Init();
	My_fds_Init();
	
    NRF_LOG_INFO("Smart brushtooth started.");  
	read_devicetime_record();
	read_hiscnt_record();
	advertising_start();    //启动广播
	//saadc_battery_read();  //采样电池电压
	//m_motor_config(1);

	while(true)
	{
		Device_Work_Loop();
		//处理挂起的LOG和运行电源管理
		idle_state_handle();
	}
}

void Device_Work_Loop(void)
{
	//喂狗
	nrfx_wdt_channel_feed(m_wdt_channel_id);
	
	//蓝牙事件
	if(my_ble.is_connected)
	{
		if(work_flag == WORK_FLAG_WORKING)//正常工作，蓝牙指示灯不亮
			LED_BLE_OFF;
		else
			LED_BLE_ON;
		
		if(my_fds_info.devicetime_record_update)//更新本地事件
		{
			my_fds_info.devicetime_record_update = false;
			update_devicetime_record();
		}
		
		if(my_ble.is_send_device_info)//发送设备信息
		{
//			my_ble.device_info_s++;
//			if(my_ble.device_info_s > 7)
//			{
//				my_ble.device_info_s = 0;
				my_ble.is_send_device_info = false;
				//return;
			//}
			send_device_info(my_ble.device_info_s);
		}
		
		if(my_ble.is_send_his_data)//发送历史数据
		{
			my_ble.his_data_s++;
			if(my_ble.his_data_s > 240)
			{
				my_ble.is_send_his_data = false;
				return;
			}
			read_hisdata_record(my_ble.his_data_s);
			
		}
	}
	
	//工作模式判定
	switch(work_flag)
	{
		case WORK_FLAG_AUTO_OFF:
		{
			break;
		}
		
		case WORK_FLAG_CHARING:
		{
			if(charge_flag == CHARGE_FULL)
			{
				Led_softblink_off();
				LED_POWER_ON;
				NRF_LOG_INFO("Charge is not\r\n");
			}
			break;
		}
		
		case WORK_FLAG_POWER_OFF:
		{	
			m_motor_test_off();
			if(my_motor.is_end_rendender)
			{
				my_motor.is_end_rendender = false;
				m_motor_remender();	
			}
			if(my_fds_info.devicetime_record_update)
			{
				my_fds_info.devicetime_record_update = false;
				update_devicetime_record();
			}
			if(my_ble.is_ble_con == false)
				Led_all_Off();
			if((my_motor.mode == 4) && is_mode4_final)
			{
				mode4_timer_stop(); 
				is_mode4_final = false;
			}
			m_motor_stop();
			
			if(my_fds_info.hisdata_record_update)
				update_hisdata_record();
			break;
		}
		
		case WORK_FLAG_WORKING:
		{
			//检测到没电了
			if(my_charge.batt_off_s == 1)
			{
				my_charge.batt_off_s = 2;
				Led_all_Off();
				m_motor_stop();
				Led_quick_flash30();
				work_flag = WORK_FLAG_AUTO_OFF;
				return;
			}
			
			//检测到电量低
			if(my_charge.batt_low_f)
			{
				LED_WORK_OFF;
				Led_flash_on();
			}
			else
			{
				Led_flash_off();
				LED_WORK_ON;
			}
			
			if(my_key.key_mode_change_f)
			{
				my_key.key_mode_change_f = false;
				LED_MODE_ON(my_key.key_mode);
				//检测到电量低，马达振动提醒
				if(my_charge.batt_low_f)
					m_motor_remender();	
				//启动延时
				if(my_motor.start_delay)
					nrf_delay_ms(my_motor.start_delay);
				
				m_motor_start(my_motor.mode, 0, 0, 2, 0);
			}			
			break;
		}
		
		case WORK_FLAG_TSET:
		{
			//检测到电量低
			if(my_charge.batt_low_f)
			{
				work_flag = WORK_FLAG_POWER_OFF;
			}
			else
			{
				m_motor_test_on();
			}
			break;
		}
	}
}

