/*------------------------------------------------------------------------------
 * Name:    SER.c
 * Purpose: Serial Input Output for Atmel SAM4S
 * Note(s): Possible defines select the used communication interface:
 *            __DBG_ITM   - ITM SWO interface
 *            __UART      - UART  (UART0)  interface
 *                        - USART (USART1) interface  (default)
 *------------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2004-2012 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include "SAM4S.h"                        /* SAM4S definitions                */
#include "system_SAM4S.h"                 /* System Header                    */
#include "SER.h"

#ifdef __DBG_ITM
volatile int32_t ITM_RxBuffer;
#endif

/* Clock Definitions */
#define BAUD(b) ((SystemCoreClock + 8*b)/(16*b))

/*------------------------------------------------------------------------------
 * SER_Init:        Initialize Serial Interface
 *----------------------------------------------------------------------------*/

void SER_Init (void) {
#ifndef __DBG_ITM
  SystemCoreClockUpdate ();
  PMC->PMC_WPMR = 0x504D4300;             /* Disable write protect            */

#ifdef __UART
  PMC->PMC_PCER0 = ((1UL << ID_PIOA) |    /* enable PIOA clock                */
                    (1UL << ID_UART0)  ); /* enable UART clock                */  

  /* Configure USART1 Pins (PA10 = TX, PA9 = RX). */
  PIOA->PIO_IDR        =  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
  PIOA->PIO_PUDR       =  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
  PIOA->PIO_ABCDSR[0] &= ~(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
  PIOA->PIO_ABCDSR[1] &= ~(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);
  PIOA->PIO_PDR        =  (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);

  /* Configure UART for 115200 baud. */
  UART0->UART_CR   = (UART_CR_RSTRX | UART_CR_RSTTX) |
                     (UART_CR_RXDIS | UART_CR_TXDIS);
  UART0->UART_IDR  = 0xFFFFFFFF;
  UART0->UART_BRGR = BAUD(115200);
  UART0->UART_MR   =  (0x4 <<  9);        /* (UART) No Parity                 */
  UART0->UART_BRGR = (96000000UL / 115200) / 16;
  UART0->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;
  UART0->UART_CR   = UART_CR_RXEN | UART_CR_TXEN;
#else
  PMC->PMC_PCER0 = ((1UL << ID_PIOA  ) |  /* enable PIOA   clock              */
                    (1UL << ID_USART1)  );/* enable USART1 clock              */  

  /* Configure USART1 Pins (PA22 = TX, PA21 = RX). */
  PIOA->PIO_IDR        =  (PIO_PA21A_RXD1 | PIO_PA22A_TXD1);
  PIOA->PIO_PUDR       =  (PIO_PA21A_RXD1 | PIO_PA22A_TXD1);
  PIOA->PIO_ABCDSR[0] &= ~(PIO_PA21A_RXD1 | PIO_PA22A_TXD1);
  PIOA->PIO_ABCDSR[1] &= ~(PIO_PA21A_RXD1 | PIO_PA22A_TXD1);
  PIOA->PIO_PDR        =  (PIO_PA21A_RXD1 | PIO_PA22A_TXD1);

  /* configure USART1 enable Pin (PA23) */
  PIOA->PIO_PUDR   =  (PIO_PA23);
  PIOA->PIO_CODR   =  (PIO_PA23);
  PIOA->PIO_OER    =  (PIO_PA23);
  PIOA->PIO_PER    =  (PIO_PA23);

  /* Configure USART1 for 115200 baud. */
  USART1->US_CR   = (UART_CR_RSTRX | UART_CR_RSTTX) | 
                    (UART_CR_RXDIS | UART_CR_TXDIS);
  USART1->US_IDR  = 0xFFFFFFFF;
  USART1->US_BRGR = BAUD(115200);
  USART1->US_MR   = (0x0 <<  0) |         /* (USART) Normal                   */
                    (0x0 <<  4) |         /* (USART) Clock                    */
                    (0x3 <<  6) |         /* (USART) Character Length: 8 bits */
                    (0x4 <<  9) |         /* (USART) No Parity                */
                    (0x0 << 12) |         /* (USART) 1 stop bit               */
                    (0x0 << 14) ;         /* (USART) Normal Mode              */
  USART1->US_PTCR  = US_PTCR_RXTDIS | US_PTCR_TXTDIS;
  USART1->US_CR    = US_CR_RXEN     | US_CR_TXEN;
#endif

  PMC->PMC_WPMR = 0x504D4301;             /* Enable write protect             */
#endif
}


/*------------------------------------------------------------------------------
 * SER_PutChar:     Write a character to the Serial Port
 *----------------------------------------------------------------------------*/

int32_t SER_PutChar (int32_t ch) {

#ifdef __DBG_ITM
  int i;
  ITM_SendChar (ch & 0xFF);
  for (i = 5000; i; i--);
#else
  #ifdef __UART
    while ((UART0->UART_SR & UART_SR_TXEMPTY) == 0);
    UART0->UART_THR = ch;
  #else
    while ((USART1->US_CSR & US_CSR_TXEMPTY) == 0);
    USART1->US_THR = ch;
  #endif
#endif
  return (ch);
}


/*------------------------------------------------------------------------------
 * SER_GetChar:     Read a character from the Serial Port (non-blocking)
 *----------------------------------------------------------------------------*/

int32_t SER_GetChar (void) {

#ifdef __DBG_ITM
  if (ITM_CheckChar())
    return (ITM_ReceiveChar());
#else
  #ifdef __UART
    if (UART0->UART_SR & UART_SR_RXRDY)
      return UART0->UART_RHR;
  #else
    if (USART1->US_CSR & US_CSR_RXRDY)
      return USART1->US_RHR;
  #endif
#endif
  return (-1);
}


/*------------------------------------------------------------------------------
 * End of file
 *----------------------------------------------------------------------------*/
