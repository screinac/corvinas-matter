#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED    8

// %%%%%% MOTOR %%%%%%
#define PWM_B    5
#define B_IN2    6
#define B_IN1    7
#define STBY    10

#define ENCODER_IN0            20
#define ENCODER_IN1            21


#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0


float position_new = 0;
float position_old = 0;
float vel = 0;
float u = 0;
float r = 0;




int32_t encoder_count = 0;
bool position_change = false;

uint32_t ii = 0;



static void encoder_isr_handler(void* arg){
    
    position_change = true;

    if(gpio_get_level(ENCODER_IN1)){
        encoder_count++;
    } 
    else {
        encoder_count--;
    }


    // gpio_isr_handler_add(ENCODER_IN0, gpio_isr_handler, NULL);
    // gpio_intr_enable(ENCODER_IN0);
}

void pwm_set(float u) {
    uint16_t duty = 0;

    if (u < -1) u = -1;
    if (u > 1) u = 1; 

    if(u >= 0){
        gpio_set_level(B_IN2, 1);
        gpio_set_level(B_IN1, 0);
        duty = (uint16_t)(u * 8191);
    } else if (u < 0){
        gpio_set_level(B_IN2, 0);
        gpio_set_level(B_IN1, 1);
        duty = (uint16_t)(-u * 8191);
    }

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}


void app_main(void)
{
    gpio_config_t gpios_conf_out = {
        .pin_bit_mask = ((1ULL<<B_IN2) | (1ULL<<B_IN1) | (1ULL<<STBY)),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpios_conf_out);

    gpio_config_t gpios_conf_ENCODER_IN0 = {
        .pin_bit_mask = (1ULL<<ENCODER_IN0),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&gpios_conf_ENCODER_IN0);

    gpio_config_t gpios_conf_ENCODER_IN1 = {
        .pin_bit_mask = (1ULL<<ENCODER_IN1),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpios_conf_ENCODER_IN1);


    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_IN0, encoder_isr_handler, NULL);
    gpio_intr_enable(ENCODER_IN0);


    gpio_dump_io_configuration(stdout, ((1ULL<<B_IN2) | (1ULL<<B_IN1) | (1ULL<<STBY) | (1ULL<<ENCODER_IN0) | (1ULL<<ENCODER_IN1)));


    ledc_timer_config_t pwm_timer_conf = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&pwm_timer_conf));

    ledc_channel_config_t pwm_channel_conf = {
        .gpio_num = PWM_B,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
    };

    ESP_ERROR_CHECK(ledc_channel_config(&pwm_channel_conf));
   


    // gpio_set_level(GPIO_OUTPUT_8, 0);  // Establecer el nivel alto (3.3V)



   


    printf("Proyecto Cortinas Matter by Plupweb");


     gpio_set_level(STBY, 1);




     while(1){

        if(position_change){
            // printf("%f\n", vel);
            printf("%f\n", position_new);

            position_new = (float)encoder_count/1820.0;
            vel = (float)(position_new - position_old)*100.0; //sample time 10ms
            position_old = position_new;
    
            position_change = false;
        }


        // if(ii < 600) {
        //     r = 0.5;
        // }
        // if(ii > 600) {
        //     r = 8;
        // }

        r = 1;

        u = r*7.852865189024962 - position_new*7.852865189024955 - vel*(-0.588985831993560);
        pwm_set(u);



        if(ii > 3000){
            ii = 0;
        }

        ii++;
       
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Sample time 10ms

     }
}