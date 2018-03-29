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
#include "PLL.h"
#include "tm4c123gh6pm.h"

#define _r 0x72
#define _$ 0x24

#define SW0 (*((volatile unsigned long *)0x40025004))
#define LED (*((volatile unsigned long *)0x40025008))
#define LOW_ACTIVE 0

unsigned char BLINK_FLAG = 0;

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
	unsigned long i;
	for(i = 0; i < 833333; i++);
}

//---------------------PortB_UART1_Init------------------------------------------
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
void PortB_UART1_Init(void)
{
  SYSCTL_RCGC1_R     |=  0x02;       // activate UART1
  SYSCTL_RCGCGPIO_R  |=  0x02;       // activate port B
  while((SYSCTL_PRGPIO_R&0x02) == 0){};	
  UART1_CTL_R        &= ~0x01;       // disable UART
  UART1_IBRD_R        =  325;        // IBRD, 80Mhz clk, 9600 baud
  UART1_FBRD_R        =  33;         // FBRD
  UART1_LCRH_R        =  0x70;       // 8 bit(no parity, one stop, FIFOs)
  UART1_CTL_R        |=  0x01;       // enable UART
  GPIO_PORTB_AFSEL_R |=  0x03;       // enable alt funct on PB0, PB1
  GPIO_PORTB_PCTL_R  &= ~0x000000FF; // configure PB0 as U1Rx and PB1 as U1Tx
  GPIO_PORTB_PCTL_R  |=  0x00000011;
  GPIO_PORTB_AMSEL_R &= ~0x03;       // disable analog funct on PB0, PB1
  GPIO_PORTB_DEN_R   |=  0x03;       // enable digital I/O on PB0, PB1
}

//---------------------PortE_AIN0_Init-----------------------------------------
//	Initialize Analog Input PE3
//	input: none
//	output: none
//------------------------------------------------------------------------
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
//--------------------------ADC0_InSeq3-----------------------------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
//------------------------------------------------------------------------
unsigned long ADC0_InSeq3(void){  
	unsigned long result;
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
  ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
  return result;
}

//---------------------PortF_Init------------------------------------------
//	Initialize push button 0 and Red LED on TM4C123
//	input: none
//	output: none
//-------------------------------------------------------------------------
void PortF_Init(void){ volatile unsigned long FallingEdges = 0;
  SYSCTL_RCGC2_R |= 0x00000020;      // (a) activate clock for port F
  FallingEdges = 0;             	   // (b) initialize counter
  GPIO_PORTF_LOCK_R   =  0x4C4F434B; //     unlock PortF
  GPIO_PORTF_DIR_R   &= ~0x01;       // (c) make PF0 in (built-in button)
	GPIO_PORTF_DIR_R   |=  0x02;       // (c) make PF1 out
  GPIO_PORTF_AFSEL_R &= ~0x03;       //     disable alt funct on PF0-1
  GPIO_PORTF_DEN_R   |=  0x03;       //     enable digital I/O on PF0-1
  GPIO_PORTF_PCTL_R  &= ~0x000000FF; //     configure PF0-1 as GPIO
  GPIO_PORTF_AMSEL_R &=  0x03;       //     disable analog functionality on PF0-1
	GPIO_PORTF_PUR_R   |=  0x01;       //     enable weak pull-up on PF0
	
	LED &= ~0x02; 	// set RED OFF;
	
	// In case using PF0 as edge-interrupt
//  GPIO_PORTF_IS_R    &= ~0x01;       // (d) PF0 is edge-sensitive
//  GPIO_PORTF_IBE_R   &= ~0x01;       //     PF0 is not both edges
//  GPIO_PORTF_IEV_R   &= ~0x01;       //     PF0 falling edge event
//  GPIO_PORTF_ICR_R   |=  0x01;       // (e) clear flag0
//  GPIO_PORTF_IM_R    |=  0x01;       // (f) arm interrupt on PF0
//  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
//  NVIC_EN0_R |= 0x40000000;          // (h) enable interrupt 30 in NVIC
}

// Initialize SysTick with busy wait running at bus clock.
void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                   // disable SysTick during setup
  NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;  // maximum reload value
  NVIC_ST_CURRENT_R = 0;                // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000; // priority 1
                                        // enable SysTick with core clock
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE+NVIC_ST_CTRL_CLK_SRC;
}
void SysTick_Handler(void){
	static unsigned long counter_1 = 0;
	static unsigned char counter_2 = 0;
	
	/* counters are counting for 1 second interval
	 * (50,000,000 ticks of 50MHz = 1 sec)
	 * if a second meets and if BLINKING FLAG
	 * is active, toggle the RED LED.
	 * Otherwise, keep counting in background.
	 */
	if(counter_1 < 1000000){
		counter_1++;
	}
	else{
		counter_1 = 0;
		counter_2++;
		
		if(counter_2 >= 50){
			if(BLINK_FLAG){
				LED ^= LED; 			// Toggle Red LED
			}
		} 
			
	}
	
}

//---------------------main---------------------
int main(void){
	unsigned long PE3_ADC0_IN_DATA;
	unsigned char UART1_in_char;
	
	PLL_Init();
	SysTick_Init();								// Initialize Systick for Red LED
	PortB_UART1_Init();						// Initialize UART1
	PortE_AIN0_Init();						// Initialize AIN0 and ADC0
	PortF_Init();								  // Initialize SW0 and RED LED

	while(1){
		// Part 2.1
		/* Receiving input from PE3(AIN0) with Sequencer3
		 * 	then transfers the converted digital data 
		 *  through UART1_TX from this MCU_2 to MCU_1
		 * Delaying about 16.67ms before receiving new Analog
		 * 	data input, so the input incoming won't be too often.
		 */
		PE3_ADC0_IN_DATA = ADC0_InSeq3(); // Getting Input from Sequencer3
		UART1_OutUDec(PE3_ADC0_IN_DATA);
		UART1_OutChar(CR);
		
		// Part 2.2.a) receive 'r'
		UART1_in_char = UART1_NonBlockingInChar();
		
		// Part 2.2.b) Blinks the LED and sends confirmation message
		if(UART1_in_char == _r){ 
			BLINK_FLAG = 1;			                      // set Blinking RED LED flag
			UART1_OutString("Red LED is Blinking.");	// Sending Confirmation Message
			UART1_OutChar(CR);
		}
		
		// Part 2.2.c) Turn off the LED and sends confirmation message
		if(SW0 == LOW_ACTIVE){
			BLINK_FLAG = 0;														// Turn off Red LED
			UART1_OutChar(_$);												// Send a trigger for the message
			UART1_OutString("Red LED is off.");       // Sending Confirmation Message
			UART1_OutChar(CR);
		}
		
		// 16.67 ms delay.
		delay();			
		
	} // end superloop

}

