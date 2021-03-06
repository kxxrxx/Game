/*-----------------------------------------------------------------------------
 *      RL-ARM - USB
 *-----------------------------------------------------------------------------
 *      Name:    Memory.c
 *      Purpose: USB Memory Storage Demo
 *      Rev.:    V4.70
 *-----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2013 KEIL - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <stm32f4xx.h>                  /* STM32F4xx Definitions              */

#include <RTL.h>
#include <File_Config.h>
#include <rl_usb.h>

#include "Memory.h"
#include "LED.h"
#include "GLCD.h"

FAT_VI *nand0;                                      /* Media Control Block    */
Media_INFO info;

/*-----------------------------------------------------------------------------
  Setup GLCD and print out example title
 *----------------------------------------------------------------------------*/
static void GLCD_Setup (void) {
  GLCD_Init ();
  GLCD_Clear (Blue);
  GLCD_SetBackColor (Blue);
  GLCD_SetTextColor (White);
  GLCD_DisplayString (1, 0, 1, "    MCBSTM32F400    ");
  GLCD_DisplayString (2, 0, 1, "       RL-ARM       ");
  GLCD_DisplayString (3, 0, 1, "    MSD example     ");
}

/*-----------------------------------------------------------------------------
  Main Program
 *----------------------------------------------------------------------------*/
int main (void) {

  LED_Init   ();                            /* Init LEDs                      */
  GLCD_Setup ();                            /* Init and setup GLCD            */

  nand0 = ioc_getcb (NULL);
  if (ioc_init (nand0) == 0) {
    ioc_read_info (&info, nand0);
    usbd_init();                            /* USB Device Initialization      */
    usbd_connect(__TRUE);                   /* USB Device Connect             */
  } else {
    GLCD_DisplayString (5, 0, 1, "  Memory Failure!   ");
  }

  while (1);                                /* Loop forever                   */
}
