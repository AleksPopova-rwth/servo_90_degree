/*
 * Degree5.c
 *
 *  Created on: Dec 4, 2024
 *      Author: alexp
 */
#include "Degree5.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

jmp_buf env;

void Degree5_Init(Degree5* controller, ADC_HandleTypeDef* adc1, ADC_HandleTypeDef* adc2, TIM_HandleTypeDef* tim, UART_HandleTypeDef* uart) {
    controller->hadc1 = adc1;
    controller->hadc2 = adc2;
    controller->htim = tim;
    controller->huart = uart;
    controller->current_pwm = 750; // Initial position
}

uint32_t Degree5_ReadADC(ADC_HandleTypeDef* adc) {
    uint32_t raw_value = 0;
    HAL_ADC_Start(adc);
    if (HAL_ADC_PollForConversion(adc, HAL_MAX_DELAY) == HAL_OK) {
        raw_value = HAL_ADC_GetValue(adc);
    }
    return raw_value;
}

void Degree5_SetServoPosition(Degree5* controller, uint16_t target_pwm) {
    uint32_t step_count = 0;
    const uint16_t tolerance = 20; // Tolerance for ADC similarity
	//uint32_t raw1 = Degree5_ReadADC(controller->hadc1);
	//uint32_t raw2 = Degree5_ReadADC(controller->hadc2);

//
//    	char msg[50];
//    	sprintf(msg, "Stopped: ADC1=%lu, ADC2=%lu\r\n", raw1, raw2);
//    	HAL_UART_Transmit(controller->huart, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    	if (controller->current_pwm < target_pwm) {
    		while (controller->current_pwm < target_pwm)
    		{
    			controller->current_pwm++;
    			controller->htim->Instance->CCR1 = controller->current_pwm;
    			HAL_Delay(1); // Adjust speed of transition
    			step_count++;

            // Check ADC values every 20 steps
    			if (step_count >= 10) {
    				uint32_t raw1 = Degree5_ReadADC(controller->hadc1);
    				uint32_t raw2 = Degree5_ReadADC(controller->hadc2);
    				   if ((raw1 > 2048 && raw2 <= 2048) || (raw2 > 2048 && raw1 <= 2048)) {
    					   break; // Stop movement if new ADC values require action
    				   }
    				   else if (abs((int32_t)(raw1 - raw2)) <= tolerance)
    				   {
    				   break;
    				   }
                // Stop if ADC values are close
    				step_count = 0; // Reset step counter
    			}

            }
        }
    	else if (controller->current_pwm > target_pwm) {
    		while (controller->current_pwm > target_pwm) {
    			controller->current_pwm--;
    			controller->htim->Instance->CCR1 = controller->current_pwm;
    			HAL_Delay(1); // Adjust speed of transition
    			step_count++;

            // Check ADC values every 20 steps
    			if (step_count >= 10) {
    				uint32_t raw1 = Degree5_ReadADC(controller->hadc1);
    				uint32_t raw2 = Degree5_ReadADC(controller->hadc2);
    				if ((raw1 > 2048 && raw2 <= 2048) || (raw2 > 2048 && raw1 <= 2048)) {
    					break; // Stop movement if new ADC values require action
    				}
 				   else if (abs((int32_t)(raw1 - raw2)) <= tolerance)
 				   {
 					   break;
 				   }
                // Stop if ADC values are close
    				step_count = 0; // Reset step counter
    			}
    		}
    	}
    }

void Degree5_ReadADCAndControlServo(Degree5* controller) {
    uint32_t raw1 = 0, raw2 = 0;
    char msg[50];
    const uint16_t tolerance = 20; // Tolerance for ADC similarity

    // Read from ADC1
    raw1 = Degree5_ReadADC(controller->hadc1);
    sprintf(msg, "ADC1: %lu\r\n", raw1);
    //HAL_UART_Transmit(controller->huart, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, (raw1 > 2048) ? GPIO_PIN_RESET : GPIO_PIN_SET);

    HAL_Delay(100);

    // Read from ADC2
    raw2 = Degree5_ReadADC(controller->hadc2);
    sprintf(msg, "ADC2: %lu\r\n", raw2);
    //HAL_UART_Transmit(controller->huart, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, (raw2 > 2048) ? GPIO_PIN_RESET : GPIO_PIN_SET);

    HAL_Delay(100);

    // Determine target PWM based on ADC values
    uint16_t target_pwm = 750;
    if (raw1 > raw2) {
        target_pwm = 250;
    } else if (raw1 < raw2) {
        target_pwm = 1250;
    }

    if (abs((int32_t)(raw1 - raw2)) >= tolerance) {
    // Gradually move servo to the target position
    	Degree5_SetServoPosition(controller, target_pwm);
    }
    HAL_Delay(100);
}
