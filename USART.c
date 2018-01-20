
//
//  Copyright (c) 2016, Stanford P. Hudson, All Rights Reserved
//

#include <stdio.h>
#include <stdarg.h>
#include "stm32f10x.h"
#include "USART.h"
#include "Board.h"
#include "stm32f10x_gpio.h" 
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "string.h"

#define USART_BUFFER_SIZE     (1024)  // must be a multiple of 2^n

typedef struct
{
  USARTStats_t                stats;
  uint8_t                     rxBuffer[USART_BUFFER_SIZE];
  uint32_t                    rxDMAIdx;
  uint8_t                     txBuffer[USART_BUFFER_SIZE];
  uint32_t                    txBufferTail;
  uint32_t                    txBufferHead;
  uint32_t                    txBufferCount;
  DMA_Channel_TypeDef        *dmaTxChannel;
  DMA_Channel_TypeDef        *dmaRxChannel;
  IRQn_Type                   dmaTxIRQChannel;
  IRQn_Type                   dmaRxIRQChannel;
  USART_TypeDef              *usartDevice;
} USARTDevStruct_t;

static USARTDevStruct_t device[USART_DEVNUM_MAX];

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
static void USARTSetupTxDMA(USARTDevNum_t devNum)
{
  USARTDevStruct_t *devPtr = &device[devNum];
   
  devPtr->dmaTxChannel->CMAR = (uint32_t)&(devPtr->txBuffer[devPtr->txBufferTail]);
  
  if (devPtr->txBufferHead > devPtr->txBufferTail)
  {
    devPtr->dmaTxChannel->CNDTR = devPtr->txBufferHead - devPtr->txBufferTail;
    devPtr->txBufferCount = devPtr->txBufferHead - devPtr->txBufferTail;
    devPtr->txBufferTail = devPtr->txBufferHead;
  }
  else
  {
    devPtr->dmaTxChannel->CNDTR = USART_BUFFER_SIZE - devPtr->txBufferTail;
    devPtr->txBufferCount = USART_BUFFER_SIZE - devPtr->txBufferTail;
    devPtr->txBufferTail = 0;
  }

  if (devPtr->txBufferCount > devPtr->stats.maxTxFifoCount)
    { devPtr->stats.maxTxFifoCount = devPtr->txBufferCount; }

  DMA_Cmd(devPtr->dmaTxChannel, ENABLE);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
uint16_t USARTRxAvailable(USARTDevNum_t devNum)
{
  USARTDevStruct_t *devPtr = &device[devNum];
  
  return (DMA_GetCurrDataCounter(devPtr->dmaRxChannel) != devPtr->rxDMAIdx) ? 1 : 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
uint16_t USARTRxNumAvailable(USARTDevNum_t devNum)
{
  USARTDevStruct_t *devPtr = &device[devNum];
  
  return DMA_GetCurrDataCounter(devPtr->dmaRxChannel);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
uint16_t USARTTxEmpty(USARTDevNum_t devNum)
{
  USARTDevStruct_t *devPtr = &device[devNum];
  
  return (devPtr->dmaTxChannel->CNDTR == 0) ? 1 : 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void USARTFlush(USARTDevNum_t devNum)
{
  USARTDevStruct_t *devPtr = &device[devNum];

  NVIC_DisableIRQ(devPtr->dmaTxIRQChannel);
  devPtr->txBufferHead = 0;
  devPtr->txBufferTail = 0;
  devPtr->txBufferCount = 0;
  devPtr->rxDMAIdx = DMA_GetCurrDataCounter(devPtr->dmaRxChannel);
  NVIC_EnableIRQ(devPtr->dmaTxIRQChannel);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
uint8_t USARTReadByte(USARTDevNum_t devNum)
{
  uint8_t ch;
  USARTDevStruct_t *devPtr = &device[devNum];

  ch = devPtr->rxBuffer[USART_BUFFER_SIZE - devPtr->rxDMAIdx];
  
  if (--devPtr->rxDMAIdx == 0)
  {
    devPtr->rxDMAIdx = USART_BUFFER_SIZE;
  }
  
  devPtr->stats.rxNumBytes++;

  return ch;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int USARTReadWait(USARTDevNum_t devNum, uint8_t *retChar)
{
  int expired = 0;
  uint32_t startMsec = BoardGetSysTicks();
  
  while (!(expired = BoardHasExpiredMsec(&startMsec, 10)) &&
         !USARTRxAvailable(devNum)) { __WFI(); }; // wait for some bytes
         
  if (!expired)
  {
    *retChar = USARTReadByte(devNum);
  }
  
  return expired;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void USARTWriteByte(USARTDevNum_t devNum, uint8_t ch, int lastByte)
{
  USARTDevStruct_t *devPtr = &device[devNum];
  
  while (devPtr->txBufferCount == USART_BUFFER_SIZE) { __WFI(); };
  
  NVIC_DisableIRQ(devPtr->dmaTxIRQChannel);
  devPtr->txBuffer[devPtr->txBufferHead] = ch;
  devPtr->txBufferHead = (devPtr->txBufferHead + 1) & (USART_BUFFER_SIZE - 1);
  devPtr->txBufferCount++;
  devPtr->stats.txNumBytes++;
  NVIC_EnableIRQ(devPtr->dmaTxIRQChannel);
  
  if (lastByte && (devPtr->dmaTxChannel->CCR & DMA_CCR1_EN) == 0)
  {
    NVIC_DisableIRQ(devPtr->dmaTxIRQChannel);
    USARTSetupTxDMA(devNum);
    NVIC_EnableIRQ(devPtr->dmaTxIRQChannel);
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void USARTPrintString(USARTDevNum_t devNum, char *str)
{
  while (*str)
  {
    char ch = *(str++);
    USARTWriteByte(devNum, ch, (*str == 0));
  }    
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void USARTPrintf(USARTDevNum_t devNum, const char *pFormat, ...)
{
  static char buffer[128];
  char *bufPtr = buffer;
  va_list argPtr;
 
  va_start(argPtr, pFormat);
  vsprintf(buffer, pFormat, argPtr);
  va_end(argPtr);
  
  USARTPrintString(devNum, bufPtr);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void USARTWriteBuf(USARTDevNum_t devNum, uint8_t *buf, uint16_t size)
{
  while (size--)
  {
    USARTWriteByte(devNum, *(buf++), (size == 0));
  }    
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
USARTStats_t *USARTGetStats(USARTDevNum_t devNum)
{
  USARTDevStruct_t *devPtr = &device[devNum];
  
  return (&(devPtr->stats));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DMA1_Channel5_IRQHandler(void)
{
  DMA_ClearITPendingBit(DMA1_IT_GL5 | DMA1_IT_TC5);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DMA1_Channel4_IRQHandler(void)
{
  USARTDevStruct_t *devPtr = &device[USART_DEVNUM_1];
  
  DMA_ClearITPendingBit(DMA1_IT_TC4);
  DMA_Cmd(devPtr->dmaTxChannel, DISABLE);

  if (devPtr->txBufferHead != devPtr->txBufferTail)
  {
    USARTSetupTxDMA(USART_DEVNUM_1);
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void USARTInit(USARTDevNum_t devNum, uint32_t baudRate, uint8_t flowControl)
{
  USART_InitTypeDef USART_InitStructure;
  DMA_InitTypeDef DMA_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  USARTDevStruct_t *devPtr;
  
  memset(&device[devNum], 0, sizeof(device[devNum]));
  
  switch (devNum)
  {
    case USART_DEVNUM_1:
      
      // DMA channel 5 (USART1 RX)
      // DMA channel 4 (USART1 TX)
      devPtr = &device[USART_DEVNUM_1];
      devPtr->dmaTxChannel = DMA1_Channel4;
      devPtr->dmaRxChannel = DMA1_Channel5;
      devPtr->usartDevice = USART1;
      
      BoardGPIOCfgPin(BOARD_USART1_RX_GPIO_PORT, BOARD_USART1_RX_GPIO_PIN, GPIO_Mode_IPU);
      BoardGPIOCfgPin(BOARD_USART1_TX_GPIO_PORT, BOARD_USART1_TX_GPIO_PIN, GPIO_Mode_AF_PP);

      USART_InitStructure.USART_BaudRate = baudRate;
      USART_InitStructure.USART_WordLength = USART_WordLength_8b;
      USART_InitStructure.USART_StopBits = USART_StopBits_1;
      USART_InitStructure.USART_Parity = USART_Parity_No;
      USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
      USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
      USART_Init(devPtr->usartDevice, &USART_InitStructure);
      
      NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
      devPtr->dmaTxIRQChannel = DMA1_Channel4_IRQn;
      
      NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
      NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
      NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
      NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
      NVIC_Init(&NVIC_InitStructure);
      NVIC_EnableIRQ(DMA1_Channel5_IRQn);
      devPtr->dmaRxIRQChannel = DMA1_Channel5_IRQn;

      DMA_DeInit(devPtr->dmaRxChannel);
      DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
      DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
      DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(devPtr->usartDevice->DR);
      DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)(devPtr->rxBuffer);
      DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
      DMA_InitStructure.DMA_BufferSize = USART_BUFFER_SIZE;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
      DMA_Init(devPtr->dmaRxChannel, &DMA_InitStructure);
      DMA_Cmd(devPtr->dmaRxChannel, ENABLE);
      USART_DMACmd(devPtr->usartDevice, USART_DMAReq_Rx, ENABLE);
      devPtr->rxDMAIdx = DMA_GetCurrDataCounter(devPtr->dmaRxChannel);
      
      DMA_DeInit(devPtr->dmaTxChannel);
      DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(devPtr->usartDevice->DR);
      DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
      DMA_Init(devPtr->dmaTxChannel, &DMA_InitStructure);
      DMA_ITConfig(devPtr->dmaTxChannel, DMA_IT_TC, ENABLE);
      devPtr->dmaTxChannel->CNDTR = 0;
      USART_DMACmd(devPtr->usartDevice, USART_DMAReq_Tx, ENABLE);
      USART_Cmd(devPtr->usartDevice, ENABLE);
      break;
      
    default:
      break;
  }
}
