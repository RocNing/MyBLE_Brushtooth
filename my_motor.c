#include "my_motor.h"

APP_TIMER_DEF(motor_timer_id); 
#define MOTOR_INTERVAL APP_TIMER_TICKS(100)//秒计时应用定时器间隔 1秒

APP_TIMER_DEF(mode4_timer_id); 
#define MODE4_INTERVAL APP_TIMER_TICKS(100)//秒计时应用定时器间隔 1秒

nrf_pwm_values_common_t m_pwm_seq_phase_1_values[PWM_SEQ_1];//实际存放PWM值的内存空间
nrf_pwm_sequence_t  m_pwm_seq;

static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);
static nrf_drv_pwm_t m_pwm1 = NRF_DRV_PWM_INSTANCE(1);
//static nrf_drv_pwm_t m_pwm2 = NRF_DRV_PWM_INSTANCE(2);

my_motor_t my_motor = {
	.is_renmender = false,
	.is_test_mode = false,
	.mode = 1,
	.freq = 295,
	.duty = 65,
	.strength = 2,
	.run_time = 120,
	.start_delay = 0,
	.is_end_rendender = false,
};

uint8_t history_data[240][9] = {0};
 
                       //频率 占空比 强度 刷牙时间
uint16_t motor_data[7][4] = {{0,   0,  0, 0},
							{292, 65, 2, 120},
							{285, 76, 2, 120},
							{295, 50, 2, 120},
							{0,   0,  2, 150},
							{283, 55, 2, 120},
							{292, 65, 2, 120}};
bool motor_run_flag = false;
							
volatile uint32_t motor_tick = 0;
volatile uint16_t mode4_tick = 0;
uint8_t is_mode4_final_change = 0;
static uint8_t motor_stop_dur = 0;//马达换区停止标志位

static void m_motor_suspend(void)
{
	nrf_drv_pwm_stop(&m_pwm0,1);
	nrf_drv_pwm_stop(&m_pwm1,1);
	nrf_drv_pwm_uninit(&m_pwm0);
	nrf_drv_pwm_uninit(&m_pwm1);
	NRF_LOG_INFO("motor_suspend\r\n");
}
bool motor_mode4_change_f = false;
bool is_mode4_final = false;

static void mode4_timer_hanndler(void * p_context)
{
	//mode4_tick++;
	
	if(is_mode4_final_change == 0)
	{
		//if(mode4_tick > 3)
		{
			//mode4_tick = 0;
			m_motor_suspend();
			m_motor_config(41);
			is_mode4_final_change = 1;
		}
	}
	if(is_mode4_final_change == 1)
	{
		//if(mode4_tick > 2)
		{
			//mode4_tick = 0;
			m_motor_suspend();
			m_motor_config(42);
			is_mode4_final_change = 0;
		}
	}
}

void mode4_timer_start()
{
	uint32_t err_code;
	
	err_code = app_timer_start(mode4_timer_id, MODE4_INTERVAL, NULL);
	APP_ERROR_CHECK(err_code);
}

void mode4_timer_stop()
{
	uint32_t err_code;
	
	err_code = app_timer_stop(mode4_timer_id);
	APP_ERROR_CHECK(err_code);
}



static void motor_timer_hanndler(void * p_context)
{
	motor_tick++;
	
	if((my_motor.mode == 4) && (motor_tick == ((my_motor.run_time-30) * 10)))
	{
		is_mode4_final = true;
		is_mode4_final_change = 0;
		mode4_timer_start();
	}
	
	if(motor_tick == (my_motor.run_time * 10))
	{		
		working_sec = 0;  //到达定时时间，准备关机
		if(my_motor.mode == 4)
		{
			mode4_timer_stop(); 
			is_mode4_final = false;
		}
		motor_timer_stop();
		if(my_charge.batt_low_f)
			my_motor.is_end_rendender = true;
		my_key.key_mode--;
		work_flag = WORK_FLAG_POWER_OFF;
		return;
	}
	
	if(is_mode4_final == false)
	{
		if((motor_tick % 300) == 0) //每30s停一次
		{
			motor_stop_dur = 1;
			m_motor_suspend();
		}	
		if((motor_stop_dur == 1) || (motor_stop_dur == 2))
		{
			motor_stop_dur++;		
		}
		else if(motor_stop_dur >= 3)
		{
			motor_stop_dur = 0;
			if(my_motor.mode == 4)
			{
				if(motor_mode4_change_f)
					m_motor_config(1);//模式1运行
				else
					m_motor_config(5);//模式5运行
			}
			else
				m_motor_config(my_motor.mode);
			
		}
		//模式4切换
		if((is_mode4_final == false) && (my_motor.mode == 4) && ((motor_tick % 20)==0) && (motor_stop_dur == 0))
		{
			if(motor_mode4_change_f)
			{
				motor_mode4_change_f = false;
				m_motor_suspend();
				m_motor_config(1);//模式1运行
			}
			else
			{
				motor_mode4_change_f = true;
				m_motor_suspend();
				m_motor_config(5);//模式5运行
			}
		}
	}
}

void motor_timer_start()
{
	uint32_t err_code;
	
	err_code = app_timer_start(motor_timer_id, MOTOR_INTERVAL, NULL);
	APP_ERROR_CHECK(err_code);
}

void motor_timer_stop()
{
	uint32_t err_code;
	
	err_code = app_timer_stop(motor_timer_id);
	APP_ERROR_CHECK(err_code);
}

void motor_timer_creat()
{
	uint32_t err_code;
	err_code = app_timer_create(&motor_timer_id, APP_TIMER_MODE_REPEATED, motor_timer_hanndler);
    APP_ERROR_CHECK(err_code);
	
	err_code = app_timer_create(&mode4_timer_id, APP_TIMER_MODE_REPEATED, mode4_timer_hanndler);
    APP_ERROR_CHECK(err_code);
}
/*static void demo1_handler(nrf_drv_pwm_evt_type_t event_type)
{
    if (event_type == NRF_DRV_PWM_EVT_FINISHED)
    {}
}*/
void m_motor_start(uint8_t mode, uint16_t motor_frq, uint8_t motor_duty, uint8_t motor_strength, uint16_t motor_run_time)
{	
	my_motor.run_time = motor_data[mode][3];
	
	m_motor_stop();
	if(mode == 4)
	{
		motor_mode4_change_f = false;
		m_motor_config(1);//模式1运行		
	}
	else
		m_motor_config(mode);	
	
	if(work_flag == WORK_FLAG_WORKING)
	{
		motor_tick = 0;      //计数清零
		motor_timer_start(); //启动马达定时器
	}
	motor_run_flag = true;
	NRF_LOG_INFO("motor_start\r\n");
}	

void m_motor_config(uint8_t mode)
{
	uint8_t i;
	
	uint16_t m_motor_top; //存放计数上限值
	uint16_t m_motor_delay;                //延迟时间
	uint8_t  m_motor_duty;
	uint8_t  m_motor_strength;	
	
	if(mode == 41)
	{
		m_motor_top = m_motor_delay = 500000/283;
		m_motor_duty = 63;
	}
	else if(mode == 42)
	{
		m_motor_top = m_motor_delay = 500000/292;
		m_motor_duty = 60;
	}
	else
	{
		m_motor_top = 500000 / motor_data[mode][0]; //存放计数上限值
		m_motor_delay = m_motor_top;                //延迟时间
		m_motor_duty = motor_data[mode][1];
		m_motor_strength = motor_data[mode][2];
	}
	// PWM0 initialization.
	//pwm配置结构体
	nrf_drv_pwm_config_t m_config =
    {
        .output_pins =
        { 
            MOTOR_PWM1_PIN | NRF_DRV_PWM_PIN_INVERTED,   // channel 0
			NRF_DRV_PWM_PIN_NOT_USED,        	    	 // channel 1
            NRF_DRV_PWM_PIN_NOT_USED,           	     // channel 2
            NRF_DRV_PWM_PIN_NOT_USED,          		     // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_500kHz,         //PWM时钟500kHz
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = m_motor_top,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_AUTO
    };
	APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm0, &m_config, NULL));
	
	// PWM1 initialization.
	m_config.output_pins[0] = MOTOR_PWM2_PIN | NRF_DRV_PWM_PIN_INVERTED; //配置引脚
	APP_ERROR_CHECK(nrf_drv_pwm_init(&m_pwm1, &m_config, NULL));

	switch(mode)
	{
		case 1:
		case 2:
		case 41:
		case 42:
			for (i = 0; i < PWM_SEQ_1; ++i)
			{
				m_pwm_seq_phase_1_values[i] = (m_motor_top * m_motor_duty) / 100;
			}
			m_pwm_seq.values.p_common  = m_pwm_seq_phase_1_values;
			m_pwm_seq.length           = NRF_PWM_VALUES_LENGTH(m_pwm_seq_phase_1_values);
			m_pwm_seq.repeats          = 0;//(MOTOR_RUN_TIME / PWM_SEQ_PHASE_1) * m_motor_freq -1;
			m_pwm_seq.end_delay        = 0;
			nrf_drv_pwm_simple_playback(&m_pwm0, &m_pwm_seq, 1, NRF_DRV_PWM_FLAG_LOOP);
			nrf_delay_us(m_motor_delay); //延迟半个周期输出另一路pwm
			nrf_drv_pwm_simple_playback(&m_pwm1, &m_pwm_seq, 1, NRF_DRV_PWM_FLAG_LOOP);
			break;
		case 3:{
			// This array cannot be allocated on stack (hence "static") and it must
			// be in RAM.
			uint16_t	LOW	       = (m_motor_top * m_motor_duty) / 100;
			uint16_t	TOP        = (m_motor_top * (m_motor_duty+40)) / 100;
			uint16_t	STEP_COUNT = 140;
			static nrf_pwm_values_common_t fade_in_out_values[2 * 140];
			uint16_t value = 0;
			uint16_t step  = (TOP - LOW) / STEP_COUNT;
			uint16_t  i;
			for (i = 0; i < STEP_COUNT; ++i)
			{
				
				fade_in_out_values[i]              = TOP - value;
				fade_in_out_values[STEP_COUNT + i] = LOW + value;
				value                             += step;
			}
			
			m_pwm_seq.values.p_common  = fade_in_out_values;
			m_pwm_seq.length           = NRF_PWM_VALUES_LENGTH(fade_in_out_values);
			m_pwm_seq.repeats          = 0;
			m_pwm_seq.end_delay        = 0;
			nrf_drv_pwm_simple_playback(&m_pwm0, &m_pwm_seq, 1, NRF_DRV_PWM_FLAG_LOOP);//运行两分钟后停下
			nrf_delay_us(m_motor_delay); //延迟半个周期输出另一路pwm
			nrf_drv_pwm_simple_playback(&m_pwm1, &m_pwm_seq, 1, NRF_DRV_PWM_FLAG_LOOP);//运行两分钟后停下
			break;
		}
		case 4:
		{
			break;
		}
		case 5 :
		{
			uint16_t t4;
			uint16_t l4;
			if(m_motor_strength == 2)
			{
				t4 = 1215;
				l4 = 868;
			}else if(m_motor_strength == 1)
			{
				t4 = 1015;
				l4 = 668;
			}else if(m_motor_strength == 0)
			{
				t4 = 815;
				l4 = 468;
			}
			uint16_t value = 0;
			uint16_t step  = (t4 - l4) / 6;
			static nrf_pwm_values_common_t low_top_values[MODE_5_SEQ_LEN];

			for (i = 0; i < MODE_5_SEQ_LEN / 2; ++i)
			{			
				low_top_values[i]     = l4 + value;
				low_top_values[6 + i] = t4 - value;
				value                += step;		
			}

			nrf_pwm_sequence_t const pwm5_seq =
			{
				.values.p_common = low_top_values,
				.length          = NRF_PWM_VALUES_LENGTH(low_top_values),
				.repeats         = 0,
				.end_delay       = 0
			};
			nrf_drv_pwm_simple_playback(&m_pwm0, &pwm5_seq, 1, NRF_DRV_PWM_FLAG_LOOP);
			nrf_delay_us(m_motor_delay); //延迟半个周期输出另一路pwm
			nrf_drv_pwm_simple_playback(&m_pwm1, &pwm5_seq, 1, NRF_DRV_PWM_FLAG_LOOP);
			break;
		}
	}
}	

void m_motor_stop(void)
{
	if(motor_run_flag == false)
		return;

	motor_timer_stop();
	nrf_drv_pwm_stop(&m_pwm0,1);
	nrf_drv_pwm_stop(&m_pwm1,1);
	nrf_drv_pwm_uninit(&m_pwm0);
	nrf_drv_pwm_uninit(&m_pwm1);
	
	if(motor_tick >= 300)//运转超过30s做记录
	{
		my_motor.dur_time = motor_tick / 10;
		my_fds_info.hisdata_record_update = true;
	}
	motor_run_flag = false;
	NRF_LOG_INFO("motor_stop\r\n");
}

//低电量马达振动提醒
void m_motor_remender(void)
{
	uint8_t i;
	my_motor.is_renmender = true;
	m_motor_suspend();
	for(i=0;i<3;i++)
	{
		m_motor_config(1);
		nrf_delay_ms(290);
		m_motor_suspend();
		nrf_delay_ms(290);
	}
	my_motor.is_renmender = false;
}

void m_motor_test_on(void)
{
	if(my_motor.is_test_mode)
		return;
	my_motor.is_test_mode = true;
	LED_MODE_ALL;
	m_motor_config(1);
}

void m_motor_test_off(void)//退出测试模式
{
	if(my_motor.is_test_mode == false)
		return;
	my_motor.is_test_mode = false;
	LED_MODE_OFF;
	m_motor_suspend();
}








