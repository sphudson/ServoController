
//
//  Copyright (c) 2016, Stanford P. Hudson, All Rights Reserved
//

#include "stm32f10x.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "Board.h"
#include "stm32f10x_gpio.h" 
#include "stm32f10x_tim.h" 
#include "USART.h"

void InitializeTimer(int period)
{
  TIM_TimeBaseInitTypeDef timerInitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

  // 72MHz / 72 = 1MHz timer frequency
  // Timer period = <period> / 1MHz
  timerInitStructure.TIM_Prescaler = (72 - 1);
  timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  timerInitStructure.TIM_Period = period;
  timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  timerInitStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM4, &timerInitStructure);
}

void InitializePWMChannel()
{
  TIM_OCInitTypeDef outputChannelInit = {0};
  
  outputChannelInit.TIM_OCMode = TIM_OCMode_PWM1;
  outputChannelInit.TIM_Pulse = 1500;
  outputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
  outputChannelInit.TIM_OCPolarity = TIM_OCPolarity_High;

  TIM_OC1Init(TIM4, &outputChannelInit);
  TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
  
  TIM_OC2Init(TIM4, &outputChannelInit);
  TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);  
  
  TIM_OC3Init(TIM4, &outputChannelInit);
  TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);  

  TIM_OC4Init(TIM4, &outputChannelInit);
  TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);  

  TIM_ARRPreloadConfig(TIM4, ENABLE);
  TIM_Cmd(TIM4, ENABLE);
  
  BoardGPIOCfgPin(GPIOB, GPIO_Pin_6, GPIO_Mode_AF_PP);
  BoardGPIOCfgPin(GPIOB, GPIO_Pin_7, GPIO_Mode_AF_PP);
  BoardGPIOCfgPin(GPIOB, GPIO_Pin_8, GPIO_Mode_AF_PP);
  BoardGPIOCfgPin(GPIOB, GPIO_Pin_9, GPIO_Mode_AF_PP);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void AppInit(void)
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void AppMain(void)
{ 
  uint8_t ch;
  
  InitializeTimer(20000);
  InitializePWMChannel();
  
  USARTInit(USART_DEVNUM_1, 115200, 0);
  
  TIM_SetCompare1(TIM4, 1500);
  TIM_SetCompare2(TIM4, 1500);
  TIM_SetCompare3(TIM4, 1500);
  TIM_SetCompare4(TIM4, 1500);

  while (1)
  {        
    if (USARTRxAvailable(USART_DEVNUM_1))
    {      
      if (!USARTReadWait(USART_DEVNUM_1, &ch))
      {
        if (ch == 's')
        {
          if (!USARTReadWait(USART_DEVNUM_1, &ch))
          {
            switch (ch)
            {
              case '0':
              case '1':
              case '2':
              case '3':
              {
                int servo = ch - '0';
                if (!USARTReadWait(USART_DEVNUM_1, &ch))
                {
                  int value, d3, d2, d1, d0;
                  
                  d3 = ch - '0';
                  if ((d3 >= 0) && (d3 <= 9) && !USARTReadWait(USART_DEVNUM_1, &ch))
                  {
                    d2 = ch - '0';
                    if ((d2 >= 0) && (d2 <= 9) && !USARTReadWait(USART_DEVNUM_1, &ch))
                    {
                      d1 = ch - '0';
                      if ((d1 >= 0) && (d1 <= 9) && !USARTReadWait(USART_DEVNUM_1, &ch))
                      {
                        d0 = ch - '0';
                        if ((d0 >= 0) && (d0 <= 9))
                        {
                          value = (d3 * 1000) + (d2 * 100) + (d1 * 10) + d0;
                          switch (servo)
                          {
                            case 0: TIM_SetCompare1(TIM4, value); break;
                            case 1: TIM_SetCompare2(TIM4, value); break;
                            case 2: TIM_SetCompare3(TIM4, value); break;
                            case 3: TIM_SetCompare4(TIM4, value); break;
                          }
                        }
                      }
                    }
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int main(void)
{
  BoardInit();
  AppInit();
  AppMain();
  return 0;
}

