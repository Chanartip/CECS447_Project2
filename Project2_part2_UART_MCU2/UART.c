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
#include "tm4c123gh6pm.h"
//#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
//#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
//#define GPIO_PORTA_AMSEL_R      (*((volatile unsigned long *)0x40004528))
//#define GPIO_PORTA_PCTL_R       (*((volatile unsigned long *)0x4000452C))
//#define UART1_DR_R              (*((volatile unsigned long *)0x4000D000))
//#define UART1_FR_R              (*((volatile unsigned long *)0x4000D018))
//#define UART1_IBRD_R            (*((volatile unsigned long *)0x4000D024))
//#define UART1_FBRD_R            (*((volatile unsigned long *)0x4000D028))
//#define UART1_LCRH_R            (*((volatile unsigned long *)0x4000D02C))
//#define UART1_CTL_R             (*((volatile unsigned long *)0x4000D030))
//#define UART_FR_TXFF            0x00000020  // UART Transmit FIFO Full
//#define UART_FR_RXFE            0x00000010  // UART Receive FIFO Empty
//#define UART_LCRH_WLEN_8        0x00000060  // 8 bit word length
//#define UART_LCRH_FEN           0x00000010  // UART Enable FIFOs
//#define UART_CTL_UARTEN         0x00000001  // UART Enable
//#define SYSCTL_RCGC1_R          (*((volatile unsigned long *)0x400FE104))
//#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
//#define SYSCTL_RCGC1_UART0      0x00000001  // UART0 Clock Gating Control
//#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

//---------------------UAR0Init------------------------------------------
// Initialize the UART for 9600 baud rate (assuming 80 MHz UART clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
/*
    IBRD = BusFreq(hz)/ (ClkDiv * baud_rate)
         = 50,000,000 / (16 * 9600)
           = (325).5208
           = 325
         
    FBRD = BRDF*64 + 0.05
         = 0.5208 *64 +0.05
           = (33).3812
           = 33
*/
// Input: none
// Output: none
//--------------------------------------------------------------------------------
void UART0_Init(void){
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_UART0; // activate UART0
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA; // activate port A
  UART0_CTL_R &= ~UART_CTL_UARTEN;      // disable UART
  UART0_IBRD_R = 325;                   // IBRD = int(50,000,000 / (16 * 115,200)) = int(27.1267)
  UART0_FBRD_R = 33;                    // FBRD = int(0.1267 * 64 + 0.5) = 8
                                        // 8 bit word length (no parity bits, one stop bit, FIFOs)
  UART0_LCRH_R = 0x70;
  UART0_CTL_R |= UART_CTL_UARTEN;       // enable UART
  GPIO_PORTA_AFSEL_R |= 0x03;           // enable alt funct on PA1-0
  GPIO_PORTA_DEN_R   |= 0x03;           // enable digital I/O on PA1-0
                                        // configure PA1-0 as UART
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFFFFFF00)+0x00000011;
  GPIO_PORTA_AMSEL_R &= ~0x03;          // disable analog functionality on PA
}

//------------UART_InChar------------
// Wait for new serial port input
// Input: none
// Output: ASCII code for key typed
unsigned char UART0_InChar(void){
  while((UART0_FR_R&UART_FR_RXFE) != 0);
  return((unsigned char)(UART0_DR_R&0xFF));
}
unsigned char UART1_InChar(void){
  while((UART1_FR_R&UART_FR_RXFE) != 0);
  return((unsigned char)(UART1_DR_R&0xFF));
}

//------------UART_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART0_OutChar(unsigned char data){
  while((UART0_FR_R&UART_FR_TXFF) != 0);
  UART0_DR_R = data;
}
void UART1_OutChar(unsigned char data){
  while((UART1_FR_R&UART_FR_TXFF) != 0);
  UART1_DR_R = data;
}


//------------UART_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART0_OutString(char *pt){
  while(*pt){
    UART0_OutChar(*pt);
    pt++;
  }
}
void UART1_OutString(char *pt){
  while(*pt){
    UART1_OutChar(*pt);
    pt++;
  }
}

//------------UART_InUDec------------
// InUDec accepts ASCII input in unsigned decimal format
//     and converts to a 32-bit unsigned number
//     valid range is 0 to 4294967295 (2^32-1)
// Input: none
// Output: 32-bit unsigned number
// If you enter a number above 4294967295, it will return an incorrect value
// Backspace will remove last digit typed
unsigned long UART0_InUDec(void){
unsigned long number=0, length=0;
char character;
  character = UART0_InChar();
  while(character != CR){ // accepts until <enter> is typed
// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART0_OutChar(character);
    }
// If the input is a backspace, then the return number is
// changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART0_OutChar(character);
    }
    character = UART0_InChar();
  }
  return number;
}

unsigned long UART1_InUDec(void){
unsigned long number=0, length=0;
char character;
  character = UART1_InChar();
  while(character != CR){ // accepts until <enter> is typed
// The next line checks that the input is a digit, 0-9.
// If the character is not 0-9, it is ignored and not echoed
    if((character>='0') && (character<='9')) {
      number = 10*number+(character-'0');   // this line overflows if above 4294967295
      length++;
      UART1_OutChar(character);
    }
// If the input is a backspace, then the return number is
// changed and a backspace is outputted to the screen
    else if((character==BS) && length){
      number /= 10;
      length--;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  return number;
}

//-----------------------UART_OutUDec-----------------------
// Output a 32-bit number in unsigned decimal format
// Input: 32-bit number to be transferred
// Output: none
// Variable format 1-10 digits with no space before or after
void UART0_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART0_OutUDec(n/10);
    n = n%10;
  }
  UART0_OutChar(n+'0'); /* n is between 0 and 9 */
}
void UART1_OutUDec(unsigned long n){
// This function uses recursion to convert decimal number
//   of unspecified length as an ASCII string
  if(n >= 10){
    UART1_OutUDec(n/10);
    n = n%10;
  }
  UART1_OutChar(n+'0'); /* n is between 0 and 9 */
}

//------------UART_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART0_InString(char *bufPt, unsigned short max) {
int length=0;
char character;
  character = UART0_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        UART0_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART0_OutChar(character);
    }
    character = UART0_InChar();
  }
  *bufPt = 0;
}

void UART1_InString(char *bufPt, unsigned short max) {
int length=0;
char character;
  character = UART1_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        UART1_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART1_OutChar(character);
    }
    character = UART1_InChar();
  }
  *bufPt = 0;
}

//------------UART0_NonBlockingInChar------------
// Get serial port input and return immediately
// Input: none
// Output: ASCII code for key typed or 0 if no character
unsigned char UART0_NonBlockingInChar(void)
{
  if((UART0_FR_R&UART_FR_RXFE) == 0)
  {
    return((unsigned char)(UART0_DR_R&0xFF));
  } 
  else
  {
    return 0;
  }
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
