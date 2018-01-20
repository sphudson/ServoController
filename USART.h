
//
//  Copyright (c) 2016, Stanford P. Hudson, All Rights Reserved
//

#ifndef _USART_H_
#define _USART_H_

#include "stm32f10x.h"

typedef enum
{
  USART_DEVNUM_1,  // Debug USART
  USART_DEVNUM_MAX
} USARTDevNum_t;

typedef struct
{
  uint32_t rxNumBytes;
  uint32_t txNumBytes;
  uint32_t maxRxFifoCount;
  uint32_t maxTxFifoCount;
} USARTStats_t;

void USARTInit(USARTDevNum_t devNum, uint32_t baudRate, uint8_t flowControl);
void USARTPrintString(USARTDevNum_t devNum, char *str);
void USARTPrintf(USARTDevNum_t devNum, const char *pFormat, ...);
void USARTWriteByte(USARTDevNum_t devNum, uint8_t ch, int lastByte);
uint16_t USARTRxAvailable(USARTDevNum_t devNum);
uint16_t USARTTxEmpty(USARTDevNum_t devNum);
uint8_t USARTReadByte(USARTDevNum_t devNum);
void USARTWriteBuf(USARTDevNum_t devNum, uint8_t *buf, uint16_t size);
USARTStats_t *USARTGetStats(USARTDevNum_t devNum);
void USARTFlush(USARTDevNum_t devNum);
int USARTReadWait(USARTDevNum_t devNum, uint8_t *retChar);
uint16_t USARTRxNumAvailable(USARTDevNum_t devNum);

#endif
