// UARTTestMain.c
// Runs on LM4F120/TM4C123
// Used to test the UART.c driver
// Daniel Valvano
// September 12, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include "PLL.h"
#include "UART.h"
#include "tm4c123gh6pm.h"

#define LED (*((volatile unsigned long *)0x40025038))		// PF3-1
	
//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void OutCRLF(void){
  UART0_OutChar(CR);
  UART0_OutChar(LF);
}
//---------------------delay-----------------------------------------------------
// Crude delay for debounce.
//  current clk with PLL: 50 MHz
//  count: 0~833,333 provides 16.67 ms delay (60Hz)
//
// Input: none
// Output: none
//-------------------------------------------------------------------------------
void delay(void) {
	unsigned long i;
	for(i = 0; i < 833333; i++);
	
}


//
//
void M1_PWM6_PF2_Init(unsigned int period, unsigned int duty){
	
  volatile unsigned long delay;
  SYSCTL_RCGCPWM_R |= 0x02;             // 1) activate PWM1
  SYSCTL_RCGCGPIO_R |= 0x20;            // 2) activate port F
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
  delay = SYSCTL_RCGCGPIO_R;            // allow time to finish activating delay here
  GPIO_PORTF_AFSEL_R |= 0x04;           // enable alt funct on PF2
  GPIO_PORTF_PCTL_R &= ~0x00000F00;     // configure PF2 as M1PWM6
  GPIO_PORTF_PCTL_R |= 0x00000500;
  GPIO_PORTF_AMSEL_R &= ~0x04;          // disable analog functionality on PF2
  GPIO_PORTF_DEN_R |= 0x04;             // enable digital I/O on PF2
  SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; // 3) use PWM divider
  SYSCTL_RCC_R &= ~SYSCTL_RCC_PWMDIV_M; //    clear PWM divider field
  SYSCTL_RCC_R += SYSCTL_RCC_PWMDIV_2;  //    configure for /2 divider
  PWM1_3_CTL_R = 0;                     // 4) re-loading down-counting mode
  
  PWM1_3_GENA_R = 0xC8;
  PWM1_3_LOAD_R = period - 1;           // 5) cycles needed to count down to 0 
  PWM1_3_CMPA_R = duty - 1;							// 6) count value when output rises
  
  PWM1_3_CTL_R |= 0x00000001;           // 7) start PWM1
  PWM1_ENABLE_R |= 0x00000040;          // enable PF2/M1PWM6
	
}
// change duty cycle of PB6
void PWM_PF2_Duty(unsigned int duty){
  PWM1_3_CMPA_R = duty - 1;             // count value when output rises
}

int main(void){
	unsigned int UART1_in_num;
	unsigned long LED_Percentage;
	unsigned char UART0_in_char;
	unsigned char UART1_in_char;
	unsigned short buffer_max =32;
	char buffer;
	
	PLL_Init();												// Initialize 50MHz PLL
	UART0_Init();											// Initialize UART0
	UART1_Init();									    // Initialize UART1
	M1_PWM6_PF2_Init(50000,35000);		// Initialize M1PMW6 with 1000 Hz and 70% duty
	
	while(1){
		
		UART1_in_num = UART1_InUDec(); 							// Get decimal number from MCU2_UART1
		LED_Percentage = UART1_in_num*100/4095;			// Calculate Percentage 0~100%
		UART0_OutUDec(LED_Percentage);							// Display the percentage (don't need by the prompt but for showing that this is worked so far.)
		OutCRLF();																	// Get a new line each time of display
		PWM_PF2_Duty((LED_Percentage*50000/100)-1);	// Change the duty of LED.
		
		// 2.a) receives 'r' from serial terminal, then sends 'r' to MCU2,
		//			and then waits for MCU2 confirmation message.
		//			Once got InString will receive a char at the time and display to the terminal
		//			(check UART.C for more code.)
		UART0_in_char = UART0_NonBlockingInChar();
		if(UART0_in_char == 0x72){					
			UART1_OutChar(UART0_in_char);
			UART1_InString(&buffer, buffer_max);
		}
		
		// 2.c) checks a token (0x73 's') from pressing SW0 on MCU2,
		//			then passing message from MCU2 to the Terminal
		UART1_in_char = UART1_NonBlockingInChar();
		if(UART1_in_char == 0x73){
			UART1_InString(&buffer, buffer_max);
		}
		
		delay();																		// Create 16.67ms (60Hz) delay
	} // end superloop

}
