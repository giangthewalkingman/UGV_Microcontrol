/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define SBUS_SIGNAL_OK          0x00
#define SBUS_SIGNAL_LOST        0x01
#define SBUS_SIGNAL_FAILSAFE    0x03
// Define channels to control the car
#define THROTTLE_CHANNEL_INDEX 0
#define STEERING_CHANNEL_INDEX 1
// Define the maximum and minimum values for the SBUS channels (throttle and steering)
//trim ch1: 1481
//trim ch2: 1503
#define SBUS_THROTTLE_MIN  988
#define SBUS_THROTTLE_MAX  1974
#define SBUS_STEERING_MIN  994
#define SBUS_STEERING_MAX  2012
// Define the maximum PWM duty cycle for motor control
#define MAX_PWM_DUTY_CYCLE 100
#define MIN_PWM_DUTY_CYCLE 0
#define THROTTLE_TRIM 9
#define STEERING_TRIM 5
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void forward_drive(uint32_t rate);
static void backward_drive(uint32_t rate);
static void left_drive(uint32_t rate);
static void right_drive(uint32_t rate);
static void hold_drive();
static void test_drive();
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
int16_t mapSBUSValue(int16_t sbusValue, int16_t sbusMin, int16_t sbusMax, int16_t minValue, int16_t maxValue);
void controlCarThrottle();
void controlCarSteering();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t failsafe_status;
uint8_t buf[25];
uint16_t CH[18];
int lenght=0;
uint16_t USB_Send_Data[]={0};
uint16_t count_1=0, count_2=0, count_3=0, count_4=0;

/*
FORWARD: TIM3
right front: TIM3_CH2 -> PA7. EN: PC6
left front: TIM3_CH1  -> PA6. EN: PC7
right back: TIM3_CH4 -> PB1. EN: PC8
left back: TIM3_CH3 -> PB0. EN: PC9
BACKWARD: TIM4
right front: TIM4_CH2 -> PD13. EN: PD1
left front: TIM4_CH1 -> PD12. EN: PD2
right back: TIM4_CH4 -> PD15. EN: PD3
left back: TIM4_CH3 -> PD14. EN: PD4
*/

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_CRC_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_DMA(&huart2, buf, 25);

  //Press blue button to start the test drive
  GPIOC->ODR &= ~(1<<6 | 1<<7 | 1<<8 | 1<<9);
  GPIOD->ODR &= ~(1<<2 | 1<<3 | 1<<4 | 1<<5);
  while (!(GPIOA->IDR & (1 << 0)));
  GPIOC->ODR |= (1 << 1);
  while (GPIOA->IDR & (1 << 0));
  while(1) {
	  test_drive();
  }

  while(1);

  while(1) {
//	  count++;
	  if (HAL_UART_Receive(&huart2, buf, 25, 100) == HAL_OK) {
		  HAL_UART_RxCpltCallback(&huart2);
	  }
	  if(count_1==1000) {
		  count_1 = 0;
	  }
	  if(count_2==1000) {
		  count_2 = 0;
	  }
	  if(count_3==1000) {
		  count_3 = 0;
	  }
	  if(count_4==1000) {
		  count_4 = 0;
	  }
//	  HAL_UART_RxCpltCallback(&huart2);
	  if(failsafe_status == SBUS_SIGNAL_OK) {
		  count_1++;
		  controlCarThrottle();
//		  controlCarSteering();
	  } else if(failsafe_status == SBUS_SIGNAL_LOST) {
		  hold_drive();
		  count_2++;
	  } else if(failsafe_status == SBUS_SIGNAL_FAILSAFE) {
		  hold_drive();
		  count_3++;
	  }
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void forward_drive(uint32_t rate) {
	GPIOC->ODR |= 1<<6 | 1<<7 | 1<<8 | 1<<9;
	GPIOD->ODR &= ~(1<<5 | 1<<2 | 1<<3 | 1<<4);
	TIM3->CCR1 = rate;
	TIM3->CCR2 = rate;
	TIM3->CCR3 = rate;
	TIM3->CCR4 = rate;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
}

static void backward_drive(uint32_t rate) {
	GPIOC->ODR &= ~(1<<6 | 1<<7 | 1<<8 | 1<<9);
	GPIOD->ODR |= 1<<5 | 1<<2 | 1<<3 | 1<<4;
	TIM4->CCR1 = rate;
	TIM4->CCR2 = rate;
	TIM4->CCR3 = rate;
	TIM4->CCR4 = rate;
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
}

static void left_drive(uint32_t rate) {
	GPIOC->ODR &= ~(1<<7 | 1<<9);
	GPIOC->ODR |= 1<<6 | 1<<8;
	GPIOD->ODR &= ~(1<<5 | 1<<3);
	GPIOD->ODR |= 1<<2 | 1<<4;
	TIM3->CCR2 = rate;
	TIM3->CCR4 = rate;
	TIM4->CCR1 = rate;
	TIM4->CCR3 = rate;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
}

static void right_drive(uint32_t rate) {
	GPIOC->ODR &= ~(1<<6 | 1<<8);
	GPIOC->ODR |= 1<<7 | 1<<9;
	GPIOD->ODR &= ~(1<<2 | 1<<4);
	GPIOD->ODR |= 1<<5 | 1<<3;
	TIM3->CCR1 = rate;
	TIM3->CCR3 = rate;
	TIM4->CCR2 = rate;
	TIM4->CCR4 = rate;
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
}

static void hold_drive() {
	GPIOC->ODR &= ~(1<<6 | 1<<7 | 1<<8 | 1<<9);
	GPIOD->ODR &= ~(1<<5 | 1<<2 | 1<<3 | 1<<4);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	if (buf[0] == 0x0F) {
//	if (1) {
		count_4++;
		CH[0] = (buf[1] >> 0 | (buf[2] << 8)) & 0x07FF;
		CH[1] = (buf[2] >> 3 | (buf[3] << 5)) & 0x07FF;
		CH[2] = (buf[3] >> 6 | (buf[4] << 2) | buf[5] << 10) & 0x07FF;
		CH[3] = (buf[5] >> 1 | (buf[6] << 7)) & 0x07FF;
		CH[4] = (buf[6] >> 4 | (buf[7] << 4)) & 0x07FF;
		CH[5] = (buf[7] >> 7 | (buf[8] << 1) | buf[9] << 9) & 0x07FF;
		CH[6] = (buf[9] >> 2 | (buf[10] << 6)) & 0x07FF;
		CH[7] = (buf[10] >> 5 | (buf[11] << 3)) & 0x07FF;
		CH[8] = (buf[12] << 0 | (buf[13] << 8)) & 0x07FF;
		CH[9] = (buf[13] >> 3 | (buf[14] << 5)) & 0x07FF;
		CH[10] = (buf[14] >> 6 | (buf[15] << 2) | buf[16] << 10) & 0x07FF;
		CH[11] = (buf[16] >> 1 | (buf[17] << 7)) & 0x07FF;
		CH[12] = (buf[17] >> 4 | (buf[18] << 4)) & 0x07FF;
		CH[13] = (buf[18] >> 7 | (buf[19] << 1) | buf[20] << 9) & 0x07FF;
		CH[14] = (buf[20] >> 2 | (buf[21] << 6)) & 0x07FF;
		CH[15] = (buf[21] >> 5 | (buf[22] << 3)) & 0x07FF;

		if (buf[23] & (1 << 0)) {
			CH[16] = 1;
		} else {
			CH[16] = 0;
		}

		if (buf[23] & (1 << 1)) {
			CH[17] = 1;
		} else {
			CH[17] = 0;
		}

		// Failsafe
		failsafe_status = SBUS_SIGNAL_OK;
		if (buf[23] & (1 << 2)) {
			failsafe_status = SBUS_SIGNAL_LOST;
		}

		if (buf[23] & (1 << 3)) {
			failsafe_status = SBUS_SIGNAL_FAILSAFE;
		}

		//	SBUS_footer=buf[24];

	}
}
// Function to map the received SBUS values to the desired range
int16_t mapSBUSValue(int16_t sbusValue, int16_t sbusMin, int16_t sbusMax, int16_t minValue, int16_t maxValue) {
    return (int32_t)(((float)(sbusValue - sbusMin) / (sbusMax - sbusMin)) * (maxValue - minValue) + minValue);
}
// Function to control the car's speed proportional to the throttle value received from channel
void controlCarThrottle() {
    int32_t throttleValue = mapSBUSValue(CH[THROTTLE_CHANNEL_INDEX], SBUS_THROTTLE_MIN, SBUS_THROTTLE_MAX, -MAX_PWM_DUTY_CYCLE, MAX_PWM_DUTY_CYCLE);
    if(throttleValue >= -MAX_PWM_DUTY_CYCLE && throttleValue <= MAX_PWM_DUTY_CYCLE ) {
    	if(throttleValue > THROTTLE_TRIM) {
    		forward_drive(throttleValue);
    	} else if(throttleValue < -THROTTLE_TRIM) {
    		backward_drive(abs(throttleValue));
    	} else {
    		hold_drive();
    	}
    }
}

// Function to control the car's steering based on the value received from channel
void controlCarSteering() {
    int32_t steeringValue = mapSBUSValue(CH[STEERING_CHANNEL_INDEX], SBUS_STEERING_MIN, SBUS_STEERING_MAX, -MAX_PWM_DUTY_CYCLE, MAX_PWM_DUTY_CYCLE);
    if(steeringValue >= -MAX_PWM_DUTY_CYCLE && steeringValue <= MAX_PWM_DUTY_CYCLE ) {
    	if(steeringValue > STEERING_TRIM) {
    		right_drive(steeringValue);
    	} else if(steeringValue < -STEERING_TRIM) {
    		left_drive(abs(steeringValue));
    	} else {
    		// nothing
    	}
    }
}

static void test_drive() {
	for(int i = 0; i < 100; i ++) {
		forward_drive(50);
		HAL_Delay(100);
	}
//	for(int i = 100; i > 0; i --) {
//		forward_drive(i);
//		HAL_Delay(100);
//	}
//	for(int i = 0; i < 100; i ++) {
//		backward_drive(i);
//		HAL_Delay(100);
//	}
//	for(int i = 100; i > 0; i --) {
//		backward_drive(i);
//		HAL_Delay(100);
//	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
