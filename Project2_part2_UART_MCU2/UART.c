/*
Name:			Chanartip Soonthornwan
					Samuel Poff
Email:	  Chanartip.Soonthornwan@gmail.com
					Spoff42@gmail.com
Project:  Project2_part2_UART
Filename: UART.c
Revision 1.1: Date: 3/21/2018
Updated:  Edit UART_Init to initialize UART_1 instead of UART_0
				  
*/
// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1

#include "UART.h"

//#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
//#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
//#define GPIO_PORTA_AMSEL_R      (*((volatile unsigned long *)0x40004528))
//#define GPIO_PORTA_PCTL_R       (*((volatile unsigned long *)0x4000452C))
#define UART1_DR_R              (*((volatile unsigned long *)0x4000D000))
#define UART1_FR_R              (*((volatile unsigned long *)0x4000D018))
#define UART1_IBRD_R            (*((volatile unsigned long *)0x4000D024))
#define UART1_FBRD_R            (*((volatile unsigned long *)0x4000D028))
#define UART1_LCRH_R            (*((volatile unsigned long *)0x4000D02C))
#define UART1_CTL_R             (*((volatile unsigned long *)0x4000D030))
#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
#define UART_CTL_UARTEN         0x00000001  // UART Enable
#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
//#define SYSCTL_RCGC1_UART0      0x00000001  // UART0 Clock Gating Control
//#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

////------------UART0_Init------------
//// Initialize the UART for 115,200 baud rate (assuming 50 MHz UART clock),
//// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
///*
//	IBRD = BusFreq(hz)/ (ClkDiv * baud_rate)
//	     = 16,000,000 / (16 * 9600)
//		 = (104).166667
//		 = 104
//		 
//	FBRD = BRDF*64 + 0.05
//	     = 0.1667 *64 +0.05
//		 = (10).71688
//		 = 10
//*/
//// Input: none
//// Output: none
//void UART_Init(void){
//  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART0; // activate UART0
//  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // activate port A
//  UART0_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
//  UART0_IBRD_R = 104;                   // IBRD = int(50,000,000 / (16 * 115,200)) = int(27.1267)
//  UART0_FBRD_R = 10;                    // FBRD = int(0.1267 * 64 + 0.5) = 8
//                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
//  UART0_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
//  UART0_CTL_R |= UART_CTL_UARTEN;       // enable UART
//  GPIO_PORTA_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
//  GPIO_PORTA_DEN_R   |= 0x03;           // enable digital I/O on PA1-0
//                                        // configure PA1-0 as UART
//  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x00000011;
//  GPIO_PORTA_AMSEL_R &= ~0x03;          // disable analog functionality on PA
//}

////------------UART_InChar------------
//// Wait for new serial port input
//// Input: none
//// Output: ASCII code for key typed
//unsigned char UART_InChar(void){
//  while((UART0_FR_R&UART_FR_RXFE) != 0);
//  return((unsigned char)(UART0_DR_R&0xFF));
//}

//------------UART1_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART1_OutChar(unsigned char data){
  while((UART1_FR_R&UART_FR_TXFF) != 0);
  UART1_DR_R = data;
}


//------------UART1_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART1_OutString(char *pt){
  while(*pt){
    UART1_OutChar(*pt);
    pt++;
  }
}


//-----------------------UART_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART1_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART1_OutUDec(n/10);
    n = n%10;
  }
  UART1_OutChar(n+'0'); /* n is between 0 and 9 */
}


//------------UART1_NonBlockingInChar------------
// Get serial port input and return immediately
// Input: none
// Output: ASCII code for key typed or 0 if no character
unsigned char UART1_NonBlockingInChar(void)
{
  if((UART1_FR_R&UART_FR_RXFE) == 0)
  {
    return((unsigned char)(UART1_DR_R&0xFF));
  } 
  else
  {
    return 0;
  }
}
