/*------------------------------------------------------------------------------
 * Name:    UART_LPC18xx.c
 * Purpose: UART I/O functions for the Keil MCB1800 evaluation board 
 *          with the NXP LPC18xx microcontroller
 * Note(s):
 *------------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2004-2013 KEIL - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include "UART.h"
#include <LPC18xx.h>

#define  LPC_UARTx      LPC_USART3
#define  UARTx_IRQn     USART3_IRQn

#define  UART_CLK        (12000000)     /* Serial Peripheral Clock is 12 MHz   */


#define  BUFFER_SIZE         (1024)     /* Size of Receive and Transmit buffers
                                           MUST BE 2^n                        */

/*------------------------------------------------------------------------------
 * GLOBAL VARIABLES
 *----------------------------------------------------------------------------*/

static struct {
           uint8_t  data[BUFFER_SIZE];
  volatile uint16_t idx_in;
  volatile uint16_t idx_out;
  volatile  int16_t cnt_in;
  volatile  int16_t cnt_out;
} WrBuffer, RdBuffer;

static uint32_t Baudrate;
static float    Frac;
static uint32_t Dll;
static uint8_t  Lsr;
static uint32_t Tx_In_Progress;

UART_FlowControl FlowControl = UART_FLOW_CONTROL_NONE;


/*------------------------------------------------------------------------------
 * UART_Initialize:  Initialize the Serial interface
 *----------------------------------------------------------------------------*/

int32_t UART_Initialize (void) {

  /* Enable GPIO register interface clock                                     */
  LPC_CCU1->CLK_M3_GPIO_CFG     |= 1;
  while (!(LPC_CCU1->CLK_M3_GPIO_STAT   & 1));

  /* Enable USART3 peripheral clock                                           */
  LPC_CCU2->CLK_APB2_USART3_CFG |= 1;
  while (!(LPC_CCU2->CLK_APB2_USART3_STAT & 1));

  /* Enable USART3 register interface clock                                   */
  LPC_CCU1->CLK_M3_USART3_CFG   |= 1;
  while (!(LPC_CCU1->CLK_M3_USART3_STAT & 1));

  /* Init GPIO pins                                                           */
  LPC_SCU->SFSP2_3 =  (1 << 6) |        /* Input buffer enabled               */
                      (1 << 4) |        /* Pull-up disabled                   */
                      (2 << 0) ;        /* Pin P2_3 used as U3_TXD            */

  LPC_SCU->SFSP2_4 =  (1 << 6) |        /* Input buffer enabled               */
                      (1 << 4) |        /* Pull-up disabled                   */
                      (2 << 0) ;        /* Pin P2_3 used as U3_RXD            */

  LPC_UARTx->IER   = 1;                 /* Enable Rx interrupt                */

  UART_Reset ();

  NVIC_EnableIRQ (UARTx_IRQn);          /* Enable UART interrupt              */

  return (1);
}


/*------------------------------------------------------------------------------
 * UART_Uninitialize: Uninitialize the Serial interface
 *----------------------------------------------------------------------------*/

int32_t UART_Uninitialize (void) {

  /* Unconfigure RX and TX pins                                               */
  LPC_SCU->SFSP2_3 = 0;
  LPC_SCU->SFSP2_4 = 0;

  LPC_UARTx->IER   = 0;                 /* Disable all UART interrupts        */

  /* Disable USART3 register interface clock                                  */
  LPC_CCU1->CLK_M3_USART3_CFG   &=~1;
  while (LPC_CCU1->CLK_M3_USART3_STAT & 1);

  /* Disable USART3 peripheral clock                                          */
  LPC_CCU2->CLK_APB2_USART3_CFG &=~1;
  while (LPC_CCU2->CLK_APB2_USART3_STAT & 1);

  NVIC_DisableIRQ (UARTx_IRQn);         /* Disable UART interrupt             */

  return (1);
}


/*------------------------------------------------------------------------------
 * UART_Reset:  Reset the Serial module variables
 *----------------------------------------------------------------------------*/

int32_t UART_Reset (void) {
  uint8_t *ptr;
  int32_t  i;

  NVIC_DisableIRQ (UARTx_IRQn);         /* Disable UART interrupt             */

  LPC_UARTx->FCR = 0x06;                /* Reset FIFOs                        */

  Baudrate  = 0;
  Frac      = 1.;
  Dll       = 0;
  Lsr       = 0x60;
  Tx_In_Progress = 0;
  ptr = (uint8_t *)&WrBuffer;
  for (i = 0; i < sizeof(WrBuffer); i++) *ptr++ = 0;
  ptr = (uint8_t *)&RdBuffer;
  for (i = 0; i < sizeof(RdBuffer); i++) *ptr++ = 0;

  NVIC_EnableIRQ (UARTx_IRQn);          /* Enable UART interrupt              */

  return (1);
}


/*------------------------------------------------------------------------------
 * UART_SetConfiguration:   Set configuration parameters of the Serial interface
 *----------------------------------------------------------------------------*/

int32_t UART_SetConfiguration (UART_Configuration *config) {
  uint32_t lcr = 0;

  switch (config->DataBits) {           /* Prepare data bits value for MR reg */
    case UART_DATA_BITS_5:
      break;
    case UART_DATA_BITS_6:
      lcr |= 1;
      break;
    case UART_DATA_BITS_7:
      lcr |= 2;
      break;
    case UART_DATA_BITS_8:
      lcr |= 3;
      break;
    default:
      return (0);
  }

  switch (config->Parity) {             /* Prepare parity value for MR reg    */
    case UART_PARITY_NONE:
      break;
    case UART_PARITY_ODD:
      lcr |= (1 << 3) | (0 << 4);
      break;
    case UART_PARITY_EVEN:
      lcr |= (1 << 3) | (1 << 4);
      break;
    case UART_PARITY_MARK:
      lcr |= (1 << 3) | (2 << 4);
      break;
    case UART_PARITY_SPACE:
      lcr |= (1 << 3) | (3 << 4);
      break;
    default:
      return (0);
  }

  switch (config->StopBits) {           /* Prepare stop bits value for MR reg */
    case UART_STOP_BITS_1:
    case UART_STOP_BITS_1_5:
      break;
    case UART_STOP_BITS_2:
      lcr |= 1 << 2;
      break;
    default:
      return (0);
  }

  switch (config->FlowControl) {        /* Prepare flow control value for MR  */
    case UART_FLOW_CONTROL_NONE:
      FlowControl = UART_FLOW_CONTROL_NONE;
      break;
    case UART_FLOW_CONTROL_RTS_CTS:
    default:
      return (0);
  }

  Baudrate = config->Baudrate;

  Frac  = 1 + (7. / 6);
  Dll   = ((UART_CLK / Frac / Baudrate) / 16);

  LPC_UARTx->LCR =  0x80;                    /* DLAB = 1                      */
  LPC_UARTx->FDR =  0x67;                    /* Fractional div (1+(7/6))      */
  LPC_UARTx->DLL =  Dll;                     /* Low divisor latch             */
  LPC_UARTx->DLM = (Dll >> 8);               /* High divisor latch            */
  LPC_UARTx->LCR =  lcr;                     /* Line Control parameters       */

  return (1);
}


/*------------------------------------------------------------------------------
 * UART_GetConfiguration:   Get configuration parameters of the serial interface
 *----------------------------------------------------------------------------*/

int32_t UART_GetConfiguration (UART_Configuration *config) {
  float    br;
  uint32_t lcr;

  lcr = LPC_UARTx->LCR;                      /* Line Control parameters       */

  br  = UART_CLK / (Dll * 16 * Frac);        /* Baudrate                      */
  /* If inside +/- 2% tolerance */
  if (((br * 100) <= (Baudrate * 102)) && ((br * 100) >= (Baudrate * 98)))
    config->Baudrate = Baudrate;
  else
    config->Baudrate = br;

  switch ((lcr >> 0) & 3) {                       /* Get data bits            */
    case 0:
      config->DataBits = UART_DATA_BITS_5;
      break;
    case 1:
      config->DataBits = UART_DATA_BITS_6;
      break;
    case 2:
      config->DataBits = UART_DATA_BITS_7;
      break;
    case 3:
      config->DataBits = UART_DATA_BITS_8;
      break;
    default:
      return (0);
  }

  switch ((lcr >> 3) & 7) {                       /* Get parity               */
    case 0:
    case 2:
    case 4:
    case 6:
      config->Parity = UART_PARITY_NONE;
      break;
    case 1:
      config->Parity = UART_PARITY_ODD;
      break;
    case 3:
      config->Parity = UART_PARITY_MARK;
      break;
    case 5:
      config->Parity = UART_PARITY_EVEN;
      break;
    case 7:
      config->Parity = UART_PARITY_SPACE;
      break;
    default:
      return (0);
  }

  switch ((lcr >> 2) & 1) {                       /* Get stop bits            */
    case 0:
      config->StopBits = UART_STOP_BITS_1;
      break;
    case 1:
      config->StopBits = UART_STOP_BITS_2;
      break;
    default:
      return (0);
  }

  config->FlowControl = UART_FLOW_CONTROL_NONE;   /* Get flow control         */

  return (1);
}


/*------------------------------------------------------------------------------
 * UART_WriteData:     Write data from buffer
 *----------------------------------------------------------------------------*/

int32_t UART_WriteData (uint8_t *data, uint16_t size) {
  uint32_t cnt;
  int16_t  len_in_buf;

  if (size == 0) return (0);

  cnt = 0;
  while (size--) {
    len_in_buf = WrBuffer.cnt_in - WrBuffer.cnt_out;
    if (len_in_buf < BUFFER_SIZE) {
      WrBuffer.data[WrBuffer.idx_in++] = *data++;
      WrBuffer.idx_in &= (BUFFER_SIZE - 1);
      WrBuffer.cnt_in++;
      cnt++;
    }
  }

  LPC_UARTx->IER |= (1 << 1);           /* Enable THRE interrupt              */

  if (!Tx_In_Progress) 
    NVIC_SetPendingIRQ(UARTx_IRQn);     /* Force interrupt to start tx        */

  return (cnt);
}


/*------------------------------------------------------------------------------
 * UART_ReadData:     Read data to buffer
 *----------------------------------------------------------------------------*/

int32_t UART_ReadData (uint8_t *data, uint16_t size) {
  uint32_t cnt;
  
  if (size == 0) return (0);
  
  cnt = 0;
  while (size--) {
    if (RdBuffer.cnt_in != RdBuffer.cnt_out) {
      *data++ = RdBuffer.data[RdBuffer.idx_out++];
      RdBuffer.idx_out &= (BUFFER_SIZE - 1);
      RdBuffer.cnt_out++;
      cnt++;
    }
  }

  LPC_UARTx->IER |= (1 << 0);           /* Enable RBR interrupt               */

  return (cnt);
}


/*------------------------------------------------------------------------------
 * UART_GetChar:     Read a received character
 *----------------------------------------------------------------------------*/

int32_t UART_GetChar (void) {
  uint8_t ch;
  
  if ((UART_ReadData (&ch, 1)) == 1) return ((int32_t)ch);
  else return (-1);
}


/*------------------------------------------------------------------------------
 * UART_PutChar:     Write a character
 *----------------------------------------------------------------------------*/

int32_t UART_PutChar (uint8_t ch) {

  if ((UART_WriteData (&ch, 1)) == 1) return ((uint32_t) ch);
  else return (-1);
}


/*------------------------------------------------------------------------------
 * UART_DataAvailable: returns number of bytes available in RdBuffer
 *----------------------------------------------------------------------------*/

int32_t UART_DataAvailable (void) {

  return (RdBuffer.cnt_in - RdBuffer.cnt_out);
}


/*------------------------------------------------------------------------------
 * UART_SetControlLineState:  Set state of control lines
 *----------------------------------------------------------------------------*/

int32_t UART_SetControlLineState (uint16_t ls, uint16_t msk) {

  return (0);
}


/*------------------------------------------------------------------------------
 * UART_GetControlLineState:  Get state of control lines
 *----------------------------------------------------------------------------*/

uint16_t UART_GetControlLineState (uint16_t msk) {

  return (0);
}


/*------------------------------------------------------------------------------
 * UART_GetStatusLineState:   Get state of status lines
 *----------------------------------------------------------------------------*/

int32_t UART_GetStatusLineState (void) {

  return (0);
}


/*------------------------------------------------------------------------------
 * UART_GetCommunicationErrorStatus:  Get error status
 *----------------------------------------------------------------------------*/

int32_t UART_GetCommunicationErrorStatus (void) {
  int32_t err;
  
  err = 0;
  if ((Lsr >> 1) & 1) err |= UART_OVERRUN_ERROR_Msk;
  if ((Lsr >> 2) & 1) err |= UART_PARITY_ERROR_Msk;
  if ((Lsr >> 3) & 1) err |= UART_FRAMING_ERROR_Msk;
  
  return (err);
}


/*------------------------------------------------------------------------------
 * UART_SetBreak :  set break condition
 *----------------------------------------------------------------------------*/

int32_t UART_SetBreak (void) {

  LPC_UARTx->LCR |=  (1 << 6);          /* Start break                        */

  return (1);
}


/*------------------------------------------------------------------------------
 * UART_ClearBreak :  clear break condition
 *----------------------------------------------------------------------------*/

int32_t UART_ClearBreak (void) {

  LPC_UARTx->LCR &= ~(1 << 6);          /* Stop break                         */

  return (1);
}


/*------------------------------------------------------------------------------
 * UART_GetBreak:   if break character received, return 1
 *----------------------------------------------------------------------------*/

int32_t UART_GetBreak (void) {

  return ((Lsr >> 4) & 1);
}


/*------------------------------------------------------------------------------
 * UART_IRQ:        Serial interrupt handler routine
 *----------------------------------------------------------------------------*/

void UART3_IRQHandler (void) { 
  uint32_t iir;
  int16_t  len_in_buf;

  iir = LPC_UARTx->IIR;                 /* Read Interrupt Identification Reg  */

  /* Handle character to transmit                                             */
  if (WrBuffer.cnt_in != WrBuffer.cnt_out) {  /* If there is data to be sent  */
    if (LPC_UARTx->LSR & (1 << 5)) {    /* If THR empty                       */
      LPC_UARTx->THR = WrBuffer.data[WrBuffer.idx_out++];
      WrBuffer.idx_out &= (BUFFER_SIZE - 1);
      WrBuffer.cnt_out++;
      Tx_In_Progress = 1;
    }
  } else if (Tx_In_Progress) {
    Tx_In_Progress = 0;
    LPC_UARTx->IER &= ~(1 << 1);        /* Disable THRE interrupt             */
  }

  /* Handle received character                                                */
  if (((iir & 0x0E) == 0x04)  ||        /* Rx interrupt (RDA)                 */
      ((iir & 0x0E) == 0xC0))  {        /* Rx interrupt (CTI)                 */
    while (LPC_UARTx->LSR & 0x01) {
      len_in_buf = RdBuffer.cnt_in - RdBuffer.cnt_out;
      if (len_in_buf < BUFFER_SIZE) {
        RdBuffer.data[RdBuffer.idx_in++] = LPC_UARTx->RBR;
        RdBuffer.idx_in &= (BUFFER_SIZE - 1);
        RdBuffer.cnt_in++;
      } else {
        LPC_UARTx->IER &= ~(1 << 0);    /* Disable RBR interrupt              */
        break;
      }
    }
  }

  Lsr = LPC_UARTx->LSR;
}


/*------------------------------------------------------------------------------
 * End of file
 *----------------------------------------------------------------------------*/

