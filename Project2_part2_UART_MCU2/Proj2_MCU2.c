/*
Name:			Chanartip Soonthornwan
					Samuel Poff
					
Email:	  Chanartip.Soonthornwan@gmail.com
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

#include "UART.h"
#include "../lib/PLL.h"
#include "../lib/tm4c123gh6pm.h"

//---------------------OutCRLF---------------------------------------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
//-------------------------------------------------------------------------------
void OutCRLF(void){
  UART1_OutChar(CR);
  UART1_OutChar(LF);
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
	int i;
	for(i = 0; i < 100000; i++);
}

//---------------------PortB_UART1_Init------------------------------------------
// Initialize the UART for 9600 baud rate (assuming 80 MHz UART clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
/*
	IBRD = BusFreq(hz)/ (ClkDiv * baud_rate)
	     = 80,000,000 / (16 * 9600)
		   = (520).83333333
		   = 520
		 
	FBRD = BRDF*64 + 0.05
	     = 0.83333333 *64 +0.05
		   = (53).383312
		   = 53
*/
// Input: none
// Output: none
//--------------------------------------------------------------------------------
void PortB_UART1_Init(void)
{
  SYSCTL_RCGC1_R     |=  0x02;       // activate UART1
  SYSCTL_RCGCGPIO_R  |=  0x02;       // activate port B
  while((SYSCTL_PRGPIO_R&0x02) == 0){};	
  UART1_CTL_R        &= ~0x01;       // disable UART
  UART1_IBRD_R        =  520;        // IBRD, 80Mhz clk, 9600 baud
  UART1_FBRD_R        =  53;         // FBRD
  UART1_LCRH_R        =  0x70;       // 8 bit(no parity, one stop, FIFOs)
  UART1_CTL_R        |=  0x01;       // enable UART
  GPIO_PORTB_AFSEL_R |=  0x03;       // enable alt funct on PB0, PB1
  GPIO_PORTB_PCTL_R  &= ~0x000000FF; // configure PB0 as U1Rx and PB1 as U1Tx
  GPIO_PORTB_PCTL_R  |=  0x00000011;
  GPIO_PORTB_AMSEL_R &= ~0x03;       // disable analog funct on PB0, PB1
  GPIO_PORTB_DEN_R   |=  0x03;       // enable digital I/O on PB0, PB1
}

//---------------------PortE_Init------------------------------------------
//	Initialize Analog Input PE3
//	input: none
//	output: none
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
  ADC0_ACTSS_R 	     &= ~0x0008;     // 9) disable sample sequencer 3
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

//---------------------main---------------------
int main(void){
	unsigned long PE3_ADC0_IN_DATA;
	PLL_Init();
	PortB_UART1_Init();									// Initialize UART1
	PortE_AIN0_Init();
	
	/* Receiving input from PE3(AIN0) with Sequencer3
	 * 	then transfers the converted digital data 
	 *  through UART1_TX from this MCU_2 to MCU_1
	 * Delaying about 1.25ms before receiving new Analog
	 * 	data input, so the input incoming won't be too often.
	 */
	while(1){
		PE3_ADC0_IN_DATA = ADC0_InSeq3(); // Getting Input from Sequencer3
		UART1_OutUDec(PE3_ADC0_IN_DATA);
		UART1_OutChar(CR);
		delay();
	} // end superloop

}

