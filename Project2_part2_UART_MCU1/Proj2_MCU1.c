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

#define LED (*((volatile unsigned long *)0x40025038))        // PF3-1


#define _r 0x72
#define _$ 0x24
#define _sh 0x23  //'#'

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
  PWM1_3_CMPA_R = duty - 1;             // 6) count value when output rises
  
  PWM1_3_CTL_R |= 0x00000001;           // 7) start PWM1
  PWM1_ENABLE_R |= 0x00000040;          // enable PF2/M1PWM6
    
}
// change duty cycle of PB6
void PWM_PF2_Duty(unsigned int duty){
  PWM1_3_CMPA_R = duty - 1;             // count value when output rises
}

// SysTick Init, period will be loaded such that the interrupts happen
// at 1ms intervals.
void SysTick_Init(unsigned long period) { 
    NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
    NVIC_ST_RELOAD_R = period-1;// reload value
    NVIC_ST_CURRENT_R = 0;      // any write to current clears it
    NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x80000000; // priority 4
    // enable SysTick with core clock and interrupts
    NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC+NVIC_ST_CTRL_INTEN;
}

unsigned char delay60Hz=0;
void SysTick_Handler(void){
    
    delay60Hz = (delay60Hz+1)%25; // increment up to 17ms for ~60Hz delay ******************  25ms... not sure how many Hz.

}    

int main(void){
    unsigned char UART0_in_char;
    unsigned char UART1_in_char;
    unsigned int UART1_in_num;
    unsigned long LED_Percentage;
    char bufPt; 
    unsigned short max=15;
//    int i=0;
        
    PLL_Init();                         // Initialize 50MHz PLL
    UART0_Init();                       // Initialize UART0
    UART1_Init();                       // Initialize UART1
    M1_PWM6_PF2_Init(50000,35000);      // Initialize M1PMW6 with 1000 Hz and 70% duty
    SysTick_Init(50000);                // 1ms systick
    
    while(1){
        if(delay60Hz == 0){
            
            UART0_in_char = UART0_NonBlockingInChar();
            
            if(UART0_in_char == _r){
                UART1_OutChar(UART0_in_char);         // Received _r from Terminal, and send to MCU2
            }
            else {
                
                UART1_in_char = UART1_NonBlockingInChar();
                
                if(UART1_in_char == _r){
//                    UART0_OutString("Got the _r");
//                    UART0_OutChar(CR);
//                    UART0_OutChar(LF);
//                    
                    UART1_InString(&bufPt, max);
//                    UART0_OutString("just printed");
                    
//                    // Loop to clear the buffer
//                    for(i=bufPt; i < bufPt+max; i++){
//                        bufPt = 0;      
//                    }
//                    
                    UART0_OutChar(CR);
                    UART0_OutChar(LF);
                    
                }
                else if(UART1_in_char == _$){
//                    UART0_OutString("Did not get the _r");
//                    UART0_OutChar(CR);
//                    UART0_OutChar(LF);
//                    
                    UART1_InString(&bufPt, max);
//                    UART0_OutString("just printed");
                    
//                    // Loop to clear the buffer
//                    for(i=bufPt; i < bufPt+max; i++){
//                        bufPt = 0;      
//                    }
//                    
                    UART0_OutChar(CR);
                    UART0_OutChar(LF);
                }
                else if(UART1_in_char == _sh){
                    UART1_in_num = UART1_InUDec();               // Get decimal number from MCU2_UART1
                    UART1_in_num &= 0x00000FFF;
                    LED_Percentage = UART1_in_num*100/4095;      // Calculate Percentage 0~100%
                    PWM_PF2_Duty((LED_Percentage*50000/100)-1);  // Change the duty of LED.
                    
                    
                    // no necessary part
//                    UART0_OutChar('*');
//                    UART0_OutUDec(LED_Percentage);               // Display the percentage (don't need by the prompt but for showing that this is worked so far.)  
//                    UART0_OutChar(CR);
//                    UART0_OutChar(LF);                           // Get a new line each time of display
                }
                else{
                    //nothing
                }
            }
            
            
        }
        
    } // end superloop

}
