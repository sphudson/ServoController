
//
//  Copyright (c) 2016, Stanford P. Hudson, All Rights Reserved
//

#ifndef _BOARD_H_
#define _BOARD_H_

#include "stm32f10x.h"
#include "stm32f10x_gpio.h" 

#define BOARD_S2_PUSHBUTTON_GPIO_PIN    (GPIO_Pin_0)
#define BOARD_S2_PUSHBUTTON_GPIO_PORT   (GPIOC)

#define BOARD_S3_PUSHBUTTON_GPIO_PIN    (GPIO_Pin_1)
#define BOARD_S3_PUSHBUTTON_GPIO_PORT   (GPIOC)

#define BOARD_LED1_GPIO_PIN             (GPIO_Pin_8)
#define BOARD_LED1_GPIO_PORT            (GPIOB)

#define BOARD_LED2_GPIO_PIN             (GPIO_Pin_9)
#define BOARD_LED2_GPIO_PORT            (GPIOB)

#define BOARD_USART1_RX_GPIO_PIN        (GPIO_Pin_10)
#define BOARD_USART1_RX_GPIO_PORT       (GPIOA)

#define BOARD_USART1_TX_GPIO_PIN        (GPIO_Pin_9)
#define BOARD_USART1_TX_GPIO_PORT       (GPIOA)

#define BOARD_S2_PUSHBUTTON_PRESSED()   (GPIO_ReadInputDataBit(BOARD_S2_PUSHBUTTON_GPIO_PORT, BOARD_S2_PUSHBUTTON_GPIO_PIN) == 0)
#define BOARD_S3_PUSHBUTTON_PRESSED()   (GPIO_ReadInputDataBit(BOARD_S3_PUSHBUTTON_GPIO_PORT, BOARD_S3_PUSHBUTTON_GPIO_PIN) == 0)

void BoardInit(void);
void BoardDelayMsec(uint32_t msec);
void BoardResetSysTick(uint32_t msec);
int BoardMsecElapsed(void);
uint32_t BoardGetSysTicks(void);
int BoardHasExpiredMsec(uint32_t *startTime, uint32_t numMsecs);
void BoardGPIOCfgPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIOMode_TypeDef mode);

#endif
