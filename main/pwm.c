#include "pwm.h"

#define GPIO_PWM0A_OUT 15   //Set GPIO 15 as PWM0A

bool was_stopped = true;

/**
 * @brief motor moves in forward direction, with duty cycle = duty %
 */
void brushed_motor_forward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    if (was_stopped) {
        mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
    }
    was_stopped = false;
}

/**
 * @brief motor moves in backward direction, with duty cycle = duty %
 */
void brushed_motor_backward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    was_stopped = true;
}

/**
 * @brief motor stop
 */
void brushed_motor_stop(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    was_stopped = true;
}


void init_pwm() {
  //1. mcpwm gpio initialization
  printf("initializing mcpwm gpio...\n");
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);

  //2. initial mcpwm configuration
  printf("Configuring Initial Parameters of mcpwm...\n");
  mcpwm_config_t pwm_config;
  pwm_config.frequency = 1000;    //frequency = 500Hz,
  pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
  pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
}