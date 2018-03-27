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

#include "UART.h"
#include "../lib/PLL.h"
#include "../lib/tm4c123gh6pm.h"
	
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
//  current clk with PLL: 80 MHz
//  count: 0~99,999 provides 1.25 ms delay
//
// Input: none
// Output: none
//-------------------------------------------------------------------------------
void delay(void) {
	unsigned long i;
	//for(i = 0; i < 100000; i++);
	for(i=0; i<1336000; i++); // 16.7ms delay (60Hz)
}




int main(void){
	unsigned long UART1_in;
	
	PLL_Init();
	UART0_Init();											// Initialize UART0
	UART1_Init();									    // Initialize UART1
	
	while(1){
		delay();
		UART1_in = UART1_InUDec(); 			// get a character Input from UART1
		UART0_OutUDec(UART1_in);				// Serially output to Terminal by UART0
		OutCRLF();
	} // end superloop

}
