/*
*       Servo.c
*/

#include "Servo.h"

static robot_t * robot;

void servo_init(robot_t * robot_) {
    robot = robot_;

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

    // Arming Sequence
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 1100);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 1000);
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Throtle Calibration
    // mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 1100);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 2000);
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 1000);
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
}

void servo_update(control_t * control) {
    if (control->speed > robot->max_speed) {
        control->speed = robot->max_speed;
    }
    if (!robot->stop) {
        control->speed = 0;
    }
    uint16_t steering = 1500 + 10*(control->steering);
    uint16_t speed = 1000 + 10*(control->speed);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, steering);
    mcpwm_set_duty_in_us(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, speed);
    vTaskDelay(10);
}