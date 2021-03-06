/*----------------------------------------------------------------------------
 *      RL-ARM - TCPnet
 *----------------------------------------------------------------------------
 *      Name:    FTP_DEMO.C 
 *      Purpose: FTP Server demo example
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <RTL.h>
#include <XMC4500.h>                    /* XMC4500 definitions                */
#include "Serial.h"
#include "LED.h"


BOOL tick;

/*--------------------------- init ------------------------------------------*/

static void init () {
  /* Add System initialisation code here */

  /* update SystemCoreClock value */
  SystemCoreClockUpdate();

  /* Configure LEDs */
  LED_Init();

  /* Configure serial output */
  SER_Init();

  /* Setup and enable the SysTick timer for 10ms. */
  SystemCoreClockUpdate();
  SysTick->LOAD = (SystemCoreClock / 100) - 1;
  SysTick->CTRL = 0x05;
}


/*--------------------------- LED_out ---------------------------------------*/

void LED_out (U32 val) {
  /* Turn LEDs ON or OFF */
  int i;

  for (i = 0; i < LED_Num(); i++) {
    if (val & (1<<i)) {
      LED_On (i);
    } else {
      LED_Off(i);
    }
  }
}


/*--------------------------- timer_poll ------------------------------------*/

static void timer_poll () {
  /* System tick timer running in poll mode */
  static U32 time;

  if (SysTick->CTRL & 0x10000) {
    time++;
    if (time == 10) {
      /* Timer tick every 100 ms */
      time = 0;
      timer_tick ();
      tick = __TRUE;
    }
  }
}


/*--------------------------- sendchar --------------------------------------*/

int sendchar (int ch)  {
  /* Debug output to serial port. */
  if (ch == '\n')  {
    SER_PutChar (0x0D);
  }
  SER_PutChar (ch & 0xFF);
  return (ch);
}


/*--------------------------- getkey ----------------------------------------*/

int getkey (void) {
  /* A dummy function for 'retarget.c' */
  return (0);
}


/*--------------------------- blink_led -------------------------------------*/

static void blink_led () {
  /* Blink the LEDs on an eval board */
  const U8 led_val[] = { 0x01, 0x00};
  static U32 cnt;

  if (tick == __TRUE) {
    /* Every 100 ms */
    tick = __FALSE;
    LED_out (led_val[cnt]);
    if (++cnt >= sizeof(led_val)) {
      cnt = 0;
    }
  }
}


/*---------------------------------------------------------------------------*/

int main (void) {
  /* Main Thread of the TcpNet */

  init ();
  init_TcpNet ();
  finit (NULL);

  while (1) {
    timer_poll ();
    main_TcpNet ();
    blink_led ();
  }
}


/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
