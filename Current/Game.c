// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano

// This virtual Nokia project only runs on the real board, this project cannot be simulated
// Instead of having a real Nokia, this driver sends Nokia
//   commands out the UART to TExaSdisplay
// The Nokia5110 is 48x84 black and white
// pixel LCD to display text, images, or other information.

// April 19, 2014
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#include "Nokia5110.h"
#include "Random.h"
#include "TExaS.h"
#include "Images.h"
// #include "ADC.h"
#include "Switches.h"

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "..//tm4c123gh6pm.h"

#define Y_MIN 2 // Minimum screen y-coord (top)
#define Y_MAX 47 // Maximum screen y-coord (bottom)
#define X_MIN 0 // Minimum screen x-coord (left)
#define X_MAX 82 // Maximum screen x-coord (right)

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Timer2_Init(unsigned long period);
void Delay100ms(unsigned long count); // Delay for count*100ms (excluding interrupts)
void TitleScreen(void);
void GameOver(void);
void PortFunction_Init(void);
void Score_Init(void); // Initializes player's score

unsigned long TimerCount;
unsigned long Semaphore;
unsigned long score;
unsigned long best;

unsigned char pipeCleared;

signed int c = 0; 	// timer counter

void Timer1A_Handler(void){ // timer 2A in full
// Generate pipes from the right
	TIMER2_ICR_R = 0x00000001;	
	c++;
	
	if(c>7) { c = 0; } // restart timer
	
// 	Pipe.var = ((Random()>>24)%3)+1;  // returns 1, 2, or 3
//	if(Pipe.x == 0){
//	UFOSpr = UFO_Init(0,UFOH);
//	Sound_UFO();
//	}
}

void Timer1_Init(unsigned long seconds){ 
// Initalise Timer2 to trigger every few seconds(assuming 880Mhz PLL)
  unsigned long volatile delay;
  SYSCTL_RCGCTIMER_R |= 0x04;
  delay = SYSCTL_RCGCTIMER_R;
  TIMER2_CTL_R = 0x00000000;
  TIMER2_CFG_R = 0x00000000; // 32-bit mode
  TIMER2_TAMR_R = 0x00000002; // periodic mode, default down-count
  TIMER2_TAILR_R = seconds * 80000000 - 1; 
  TIMER2_TAPR_R = 0; // bus clock resolution
  TIMER2_ICR_R = 0x00000001;
  TIMER2_IMR_R = 0x00000001;
  NVIC_PRI5_R = (NVIC_PRI5_R&0x0FFFFFFF)|(0x03 << 29); // priority 3
	NVIC_EN0_R |= 1<<23;
	TIMER2_CTL_R = 0x00000001; 
}

struct State {
  unsigned long x;      // x coordinate
  unsigned long y;      // y coordinate
  const unsigned char *image[3];				// pointer to image
  long life;            // 0=dead, 1=
};         

typedef struct State STyp;
STyp Player;		
void Init(void){ 
    Player.x = 5;
    Player.y = 23;		
    Player.image[0] = Sprite;
    Player.image[1] = Sprite_fall;
    Player.image[2] = Sprite_fly;
    Player.life = 1;
}

typedef struct Obstacle {
  unsigned long x;      // x coordinate
  unsigned long y;      // y coordinate
  const unsigned char *image; 
	unsigned char	var;  // pipe variant
} OTyp;

OTyp Pipe;  

void Obstacle_Init(void){ 
		unsigned char	var;  // pipe variant
    Pipe.x = 66; 
//    Pipe.y = 9;  // PipeY --> Pipe.y = Y - 1
			
}
	

void Move(void){
    if((Player.y < Y_MAX) && (Player.y > Y_MIN)){    
			
			if(Pipe.x < X_MAX) {    
			Pipe.x  -= 1; // move left
			}
			else
			{
				Pipe.x = 66;
			
//				
//						Pipe.var = ((Random()>>24)%3)+1;  // returns 1, 2, or 3
//	
//				switch(Pipe.var)
//									{
//													case 1: 
//															Pipe.image = Pipe10;
//															Pipe.y = 9;
//															break;
//													
//													case 2: 
//															Pipe.image = Pipe23;
//															Pipe.y = 22;
//															break;
//													
//													case 3: 
//															Pipe.image = Pipe27;
//															Pipe.y = 26;
//															break;
//													
//													case 6: 
//															Pipe.image = Pipe32;
//															Pipe.y = 31;
//															break;
//													
//													case 4: 
//															Pipe.image = Pipe38;
//															Pipe.y = 37;
//															break;
//													
//													case 5: 
//															Pipe.image = Pipe42;
//															Pipe.y = 41;
//															break;
//									}
			}
			
			if ((SW2 == 0) && (SW1 != 0))  //SW2 is pressed
			{
					GPIO_PORTF_DATA_R &= ~0x02; // Turn off red LED.
					GPIO_PORTF_DATA_R &= ~0x08;	// Turn off green LED.
				
			    GPIO_PORTF_DATA_R |= 0x04; // Turn on blue LED.
				
					Player.y -= 5;   // move up
					
			}
			else if ((SW1 == 0) && (SW2 != 0))
			{
					GPIO_PORTF_DATA_R &= ~0x04; // Turn off blue LED.
				
				// Turn on yellow LED:
			    GPIO_PORTF_DATA_R |= 0x02; // Turn on red LED.
					GPIO_PORTF_DATA_R |= 0x08; // Turn on green LED.

				
			// make it pause game and turn on orange or yellow LED
				
			}
			else 
			{
			   GPIO_PORTF_DATA_R &= ~0x04; // Turn off blue LED.
				 GPIO_PORTF_DATA_R &= ~0x02; // Turn off red LED.
				 GPIO_PORTF_DATA_R &= ~0x08;	// Turn off green LED.
			
				 Player.y += 1; // move down
			}
			
    }
		else{
      Player.life = 0;
      GameOver();
			
    }
}

unsigned long FrameCount=0;
void Draw(void){
  Nokia5110_ClearBuffer();
    if(Player.life > 0){			
			Nokia5110_PrintBMP(X_MIN, Y_MAX, Ground, 0);
			
				if(Pipe.x == 66)
					{
						
						Pipe.var = ((Random()>>24)%3)+1;  // returns 1, 2, or 3
	
						switch(Pipe.var)
									{
													case 1: 
															Pipe.image = Pipe10;
															Pipe.y = 9;
															break;
													
													case 2: 
															Pipe.image = Pipe23;
															Pipe.y = 22;
															break;
													
													case 3: 
															Pipe.image = Pipe27;
															Pipe.y = 26;
															break;
													
													case 6: 
															Pipe.image = Pipe32;
															Pipe.y = 31;
															break;
													
													case 4: 
															Pipe.image = Pipe38;
															Pipe.y = 37;
															break;
													
													case 5: 
															Pipe.image = Pipe42;
															Pipe.y = 41;
															break;
														}
													}

			Nokia5110_PrintBMP(Pipe.x, Pipe.y, Pipe.image, 0);
			
						if(SW2 == 0) // SW2 is pressed
			{
					Nokia5110_PrintBMP(Player.x, Player.y, Player.image[2], 0);
			}
			// else if (SW1 pressed) then pause game
			else 
			{
//				 Nokia5110_PrintBMP(Player.x, Player.y, Player.image[1], 0);
				Nokia5110_PrintBMP(Player.x, Player.y, Player.image[2], 0);
			}
    }
  Nokia5110_DisplayBuffer();      // draw buffer
  FrameCount = (FrameCount+1)&0x01; // 0,1,0,1,...
}

// PF4 (0x01) is input SW1 and PF2 (0x04) is output Blue LED
void PortF_Init(void){ 
	
	volatile unsigned long delay;

  SYSCTL_RCGC2_R |= 0x00000020;     // activate clock for Port F
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  // only PF0 needs to be unlocked, other bits can't be locked
  GPIO_PORTF_AMSEL_R = 0x00;        // disable analog on PF
  GPIO_PORTF_PCTL_R = 0x00000000;   // PCTL GPIO on PF4-0
  GPIO_PORTF_DIR_R = 0x0E;          // PF4,PF0 in, PF3-1 out
  GPIO_PORTF_AFSEL_R = 0x00;        // disable alt funct on PF7-0
  GPIO_PORTF_PUR_R = 0x11;          // enable pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R = 0x1F;          // enable digital I/O on PF4-0

}

//void Interrupt_Init(void)
//{
//  IntEnable(INT_GPIOF);  							// enable interrupt 30 in NVIC (GPIOF)
//	IntPrioritySet(INT_GPIOF, 0x00); 		// configure GPIOF interrupt priority as 0
//	GPIO_PORTF_IM_R |= 0x11;   		// arm interrupt on PF0 and PF4
//	GPIO_PORTF_IS_R &= ~0x11;     // PF0 and PF4 are edge-sensitive
//  GPIO_PORTF_IBE_R &= ~0x11;   	// PF0 and PF4 not both edges trigger 
//  GPIO_PORTF_IEV_R &= ~0x11;  	// PF0 and PF4 falling edge event
//}

//**********************GameEngine_Init***********************
// Calls the initialization of the SysTick and give
// initial values to some of the game engine variables
// It needs to be called by the main function of the program
// inputs: none
// outputs: none

int main(void)
	{ 
		
	TExaS_Init(NoLCD_NoScope);  // set system clock to 80 MHz
  // you cannot use both the Scope and the virtual Nokia (both need UART0)
  Random_Init(1);
  Nokia5110_Init();
  EnableInterrupts(); // virtual Nokia uses UART0 interrupts
	
  PortF_Init();	
		
	// Interrupt_Init();
		
	TitleScreen();		

  Init();
	Obstacle_Init();
		
  Timer2_Init(80000000/10);  //  80000000/30 for 30 Hz  // increase denom to speed up		
			
  while(1){
		
		 // Timer1_Init(1);  // increment counter every T seconds, slows down game for some reason
		
		Draw();		
		Move();
		
//		Nokia5110_Clear();
//	
//		Nokia5110_PrintBMP(0, 82, Ground, 0);
//	
//		Nokia5110_DisplayBuffer(); 

		Delay100ms(1);    // delay 5 sec at 80 MHz //c 50 to 1 to speed up sim
		
	}
		
		
	
//	//SW2 is pressed
//  if(GPIO_PORTF_RIS_R&0x01)
//	{
//		// acknowledge flag for PF0
//		GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0);
//		Player.y += 4;
//		Nokia5110_PrintBMP(Player.x, Player.y, Player.image[1], 0);
//	}
		
	//~~~ During Game

//  Nokia5110_Clear();
//	
//	// Nokia5110_PrintBMP(5, 25, Sprite, 0);
//	
//	Nokia5110_DisplayBuffer(); 
//	
//	Nokia5110_SetCursor(0, 4);	// 6 rows (0-5), 12 characters wide
//	Nokia5110_OutString("____________"); 	
//	Nokia5110_SetCursor(0, 5);
//	Nokia5110_OutString("//Score:1 ||"); 		
//	Nokia5110_SetCursor(0, 0); // renders screen
		

		
}			
		




// You can use this timer only if you learn how it works
void Timer2_Init(unsigned long period){ 
  unsigned long volatile delay;
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  delay = SYSCTL_RCGCTIMER_R;
  TimerCount = 0;
  Semaphore = 0;
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period-1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}

void Timer2A_Handler(void){ 
  TIMER2_ICR_R = 0x00000001;   // acknowledge timer2A timeout
  TimerCount++;
  Move(); 
  Semaphore = 1; // trigger
}

void Delay100ms(unsigned long count){
	unsigned long volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

void TitleScreen(void){

  Nokia5110_ClearBuffer();
		
	Nokia5110_PrintBMP(16, 22, Birdy, 0);
	Nokia5110_PrintBMP(48, 22, Flap, 0);
  Nokia5110_PrintBMP(27, 40, Start, 0);
		
  Nokia5110_DisplayBuffer();   // draw buffer
		
  Delay100ms(5);              // delay 5 sec at 80 MHz //c 50 to 1 to speed up sim
	while(SW2 != 0){};					 // displays until SW2 is pressed		
	Delay100ms(5);
	
}

void GameOver(void){
	
	GPIO_PORTF_DATA_R &= 0x02; // Turn on red LED
	
	Nokia5110_ClearBuffer();
	Nokia5110_PrintBMP(15, 17, Game_Over, 0);		// y = 21
	Nokia5110_DisplayBuffer();   // draw buffer
	
	Nokia5110_SetCursor(1, 3);
  Nokia5110_OutString("Score:");
	//	Nokia5110_OutUDec(Score);
	 Nokia5110_SetCursor(1, 5);
   Nokia5110_OutString("Best:"); 
  //	Nokia5110_OutUDec(bestScore);	
	
  Nokia5110_SetCursor(0, 0); // renders screen
	
//	Delay100ms(5);	
	// while(!FIRE){};
  Delay100ms(10);
}

void Score_Init(void){
		score = 0;

		while(Player.life > 0){
		if(pipeCleared){
				score = 0;
				score += 1;
				GPIO_PORTF_DATA_R &= 0x08; // Turn on green LED
			}
	
		if (score > best){
		best = score;
			// display rainbow led?
			}
	
	}

}


 

	

