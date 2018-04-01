/*
Name:            Chanartip Soonthornwan
                    Samuel Poff
                    
Email:      Chanartip.Soonthornwan@gmail.com
                    Spoff42@gmail.com
                    
Project:  Project2_part2_UART

Filename: Proj2_MCU2.c

Revision 1.0: Date: 3/21/2018
Revision 1.1: Date: 3/21/2018
        - Added PortB_UART1_Init()
        - Added PortE_AIN0_Init()

*/

// U1Rx (VCP receive) connected to PB0
// U1Tx (VCP transmit) connected to PB1

#include "../lib/UART.h"
#include "../lib/PLL.h"
#include "../lib/tm4c123gh6pm.h"
#include "../lib/SysTick.h"

#define _r 0x72
#define _$ 0x24

unsigned char UPDATE_SYS=0;
    
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
    for(i = 0; i < 833333; i++);
}

//---------------------PortE_Init------------------------------------------
//    Initialize Analog Input PE3
//    input: none
//    output: none
//-------------------------------------------------------------------------
void PortE_AIN0_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R     |= 0x00000010;  // 1) Activate clock for Port E
  delay = SYSCTL_RCGC2_R;            //    Allow time for clock to start.
  GPIO_PORTE_CR_R    |=  0x10;       //    Allow changes to PE3
  GPIO_PORTE_AMSEL_R |=  0x10;       // 3) Disable analog on PF.
  GPIO_PORTE_PCTL_R  &= ~0x000F0000; // 4) PCTL GPIO on PE3.
  GPIO_PORTE_DIR_R   &= ~0x10;       // 5) set PE3 as input
  GPIO_PORTE_AFSEL_R |=  0x10;       // 6) Enable alt funct on PE3
  GPIO_PORTE_DEN_R   &= ~0x10;       // 7) Disable digital I/O on PE3
    
  SYSCTL_RCGC0_R     |=  0x00010000; // 6) activate ADC0 
  delay = SYSCTL_RCGC2_R;         
  SYSCTL_RCGC0_R     &= ~0x00000300; // 7) configure for 125K 
  ADC0_SSPRI_R        =  0x0123;     // 8) Sequencer 3 is highest priority
  ADC0_ACTSS_R       &= ~0x0008;     // 9) disable sample sequencer 3
  ADC0_EMUX_R        &= ~0xF000;     // 10) seq3 is software trigger
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+0; // 11) channel Ain0 (PE3)
  ADC0_SSCTL3_R       =  0x0006;     // 12) no TS0 D0, yes IE0 END0
  ADC0_ACTSS_R       |=  0x0008;     // 13) enable sample sequencer 3

}
//------------ADC0_InSeq3------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
unsigned long ADC0_InSeq3(void){  
    unsigned long result;
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
  ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
  return result;
}

// SysTick_Handler
//      Active at 60Hz as RELOAD value set in Systick_Init()
//      
void SysTick_Handler(){
    UPDATE_SYS = 1;
}

//---------------------main---------------------
int main(void){

    unsigned char UART1_in_char;
    unsigned long PE3_ADC0_IN_DATA;
    
    PLL_Init();
    SysTick_Init();
    UART0_Init();
    UART1_Init();                                    // Initialize UART1
    PortE_AIN0_Init();
    
    /* Receiving input from PE3(AIN0) with Sequencer3
     *     then transfers the converted digital data 
     *  through UART1_TX from this MCU_2 to MCU_1
     * Delaying about 1.25ms before receiving new Analog
      */
    while(1){

        if(UPDATE_SYS){
            UPDATE_SYS=0;
            PE3_ADC0_IN_DATA = ADC0_InSeq3(); // Getting Input from Sequencer3
            UART1_OutUDec(PE3_ADC0_IN_DATA);
            UART1_NonBlockingOutChar(CR);
            UART0_OutUDec(PE3_ADC0_IN_DATA);
            UART0_OutCRLF();
        }
        else{
            
        }
    } // end superloop

}

/*

        UART1_in_char = UART1_NonBlockingInChar();
        if(UART1_in_char == _r){
            UART1_OutString("Red LED is Blinking.");
            UART1_OutChar(CR);
            UART1_in_char = 0;
        }
        else{
            PE3_ADC0_IN_DATA = ADC0_InSeq3(); // Getting Input from Sequencer3
            UART1_OutUDec(PE3_ADC0_IN_DATA);
            UART1_OutChar(CR);
        }            
            
        
        
        delay();
*/
