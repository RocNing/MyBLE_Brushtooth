#include "my_key.h"
#include "my_led.h"
#include "my_motor.h"

my_key_t my_key = 
{
	.key_mode = 0,
	.iskeypre_f = false,
	.key_Pre_cnt = 0,
	.key_mode_change_f = false,
};

extern volatile uint32_t systick;//上电工作时间
uint8_t charger_flag = NO_CHARGE;     // 0-no charger;  1-charging;  2-charge full
//APP按键事件回调函数
void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
	switch (pin_no)//判断键值
    {
        case MY_BUTTON_1://是BUTTON_1按键
			if(systick < 6)//刚上电，不允许开机
				return;
			if(work_flag == WORK_FLAG_CHARING)//充电不允许开机
			{
				return;
			}
			if(my_motor.is_renmender)//振动提醒期间屏蔽按键
				return;
            if(button_action == APP_BUTTON_PUSH)//按键按下事件
			{
				my_key.iskeypre_f = true;
				if(work_flag == WORK_FLAG_TSET)//工作在测试模式，再按退出
				{
					work_flag = WORK_FLAG_POWER_OFF;
					return;
				}
				
				if(my_ble.is_ble_con)
				{
					my_ble.is_ble_con = false;
					my_key.key_mode--;
				}
				
				if(working_sec <= 5)//5s以内按键切换模式
				{
					my_key.key_mode_change_f = true;
					working_sec = 0; //从零开始计算工作时间
					my_key.key_mode++;
					if(my_key.key_mode > LED_NUM)
						my_key.key_mode = 1;
					my_motor.mode = my_key.key_mode;
					//m_motor_mode = key_mode;
				}
				else
				{
					working_sec = 0; //从零开始计算工作时间
					my_key.key_mode--;
					work_flag = WORK_FLAG_POWER_OFF;						
					return;
				}			
				work_flag = WORK_FLAG_WORKING;
			}
			
			if(button_action == APP_BUTTON_RELEASE)//按键释放事件
			{
				my_key.iskeypre_f = false;
				my_key.key_Pre_cnt = 0;
			}
			break;
			
		case CHG_IN_PIN_NO://插入充电器，电池开始充电
           if(button_action == APP_BUTTON_PUSH)//按键按下事件
			{
				if(work_flag == WORK_FLAG_WORKING)
				{//检测到充电暂停工作
					my_key.key_mode--;					
					Led_all_Off();
					m_motor_stop();
				}
				work_flag = WORK_FLAG_CHARING;
				charge_flag = CHARGING;
				Led_softblink_on();
				NRF_LOG_INFO("Charging.\r\n"); 
			}
			
			if(button_action == APP_BUTTON_RELEASE)//充电器拔下或充满
			{
				//if(STB_IN_RD)
				{
					charge_flag = NO_CHARGE;
					work_flag = WORK_FLAG_POWER_OFF;
					Led_softblink_off();
					NRF_LOG_INFO("Charge is not\r\n"); 
				}
			}
           break;
			
		/*case STB_IN_PIN_NO://电池电量充满
           if(button_action == APP_BUTTON_PUSH)//按键按下事件
			{
				work_flag = WORK_FLAG_CHARING;
				//LED_POWER_ON;
				LED_MODE_OFF;
				
				NRF_LOG_INFO("Charge is over.\r\n"); 
			}
			
			if(button_action == APP_BUTTON_RELEASE)//按键释放事件
			{
				work_flag = WORK_FLAG_POWER_OFF;
				LED_POWER_OFF;
				NRF_LOG_INFO("Charge is not\r\n");
			}*/
         //  break;
        default:
            return; 
    }
}
void buttons_init(void)
{
    ret_code_t err_code;
	
    //app按键数组必须定义为static类型，因为按键处理模块需要保存指向该数组的指针
    static app_button_cfg_t buttons[] =
    {
        {MY_BUTTON_1, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_NOPULL, button_event_handler},
		{CHG_IN_PIN_NO, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_NOPULL, button_event_handler},//充电指示脚
		{STB_IN_PIN_NO, APP_BUTTON_ACTIVE_LOW, NRF_GPIO_PIN_NOPULL, button_event_handler},//充满指示脚
    };
    //初始化APP按键
    err_code = app_button_init(buttons, ARRAY_SIZE(buttons),
                               BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);		
	//使能APP按键
	err_code = app_button_enable();
	APP_ERROR_CHECK(err_code);
}

