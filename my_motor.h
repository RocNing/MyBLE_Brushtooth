#ifndef __MY_MOTOR_H
#define __MY_MOTOR_H

#include "common.h"
#include "nrf_drv_pwm.h"
#include "nrf_delay.h"

//马达输出引脚
#define MOTOR_PWM1_PIN          17
#define MOTOR_PWM2_PIN          18

//#define MOTOR_RUN_TIME      120  //马达运行时间，s

#define PWM_SEQ_1		6

#define MODE_5_SEQ_LEN   12
void m_motor_start(uint8_t mode, uint16_t motor_frq, uint8_t motor_duty, uint8_t motor_strength, uint16_t motor_run_time);
void m_motor_config(uint8_t mode);
void m_motor_stop(void);

void m_motor_test_on(void);
void m_motor_test_off(void);

void m_motor_remender(void);

void mode4_timer_start(void);
void mode4_timer_stop(void);
extern bool is_mode4_final;

void motor_timer_start(void);
void motor_timer_stop(void);
void motor_timer_creat(void);

#endif

