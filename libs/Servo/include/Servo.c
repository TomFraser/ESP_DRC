/*
*       Servo.c
*/

#include "Servo.h"

void servo_init() {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, STEERING_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, ESC_PIN);

    mcpwm_config_t pwm_config1;
        pwm_config1.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
        pwm_config1.cmpr_a = 0;    //duty cycle of PWMxA = 0
        pwm_config1.cmpr_b = 0;    //duty cycle of PWMxb = 0
        pwm_config1.counter_mode = MCPWM_UP_COUNTER;
        pwm_config1.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config1);

    mcpwm_config_t pwm_config2;
        pwm_config2.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
        pwm_config2.cmpr_a = 0;    //duty cycle of PWMxA = 0
        pwm_config2.cmpr_b = 0;    //duty cycle of PWMxb = 0
        pwm_config2.counter_mode = MCPWM_UP_COUNTER;
        pwm_config2.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &pwm_config2);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    control_t init_control;
        init_control.steering = 0;
        init_control.speed = 20;
    servo_update(&init_control);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
        init_control.speed = 0;
    servo_update(&init_control);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

void servo_update(control_t * control) {
    uint16_t steering = 1500 + 10*control->steering;
    uint16_t speed = 1000 + 10*control->speed;
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, steering);
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, speed);
    vTaskDelay(10);
}