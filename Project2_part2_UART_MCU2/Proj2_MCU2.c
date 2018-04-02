/***************************************************************************
    Name:       Chanartip Soonthornwan
                Samuel Poff
                        
    Email:      Chanartip.Soonthornwan@gmail.com
                spoff42@gmail.com
                        
    Project:    Project2_part2_UART
    Filename:   Proj2_MCU2.c
    Revision 1.0: Date: 3/21/2018
    Revision 1.1: Date: 3/21/2018
            - Added PortB_UART1_Init()
            - Added PortE_AIN0_Init()
    Revision 1.2: Date: 3/30/2018
            - Debounced SW0 toggles 'button_says_okay' flag.
            - SysStick counts up in 1ms increments. Every second
              it toggles a flag called 'timer_says_okay'.
    Revision 1.3: Date: 3/31/2018
            - Remove crude delay() by utilizing Systick to count
                a counter to 17ms (~60Hz)
              
***************************************************************************/



/***************************************************************************
Includes, defines, prototypes, global variables. 
***************************************************************************/

//#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "UART.h"

#define WhenIFeelLikeWorking while(1)
#define _r 0x72
#define _$ 0x24
#define _sh 0x23
#define LED (*((volatile unsigned long *)0x40025008))


/***************************************************************************
Inits
***************************************************************************/

// Enable red LED as output and SW0 as input.
void PortF_Init(void) { 
    volatile unsigned long delay1;
    SYSCTL_RCGCGPIO_R |= 0x00000020;  // 1) activate clock for Port F
    delay1 = SYSCTL_RCGCGPIO_R;       // allow time for clock to start
    GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
    GPIO_PORTF_CR_R = 0x03;           // allow changes to PF4-0
    GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog on PF
    GPIO_PORTF_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PF4-0
    GPIO_PORTF_DIR_R = 0x02;          // 5) PF4,PF0 in, PF3-1 out
    GPIO_PORTF_AFSEL_R = 0x00;        // 6) disable alt funct on PF7-0
    GPIO_PORTF_PUR_R = 0x01;          // enable pull-up on PF0 and PF4
    GPIO_PORTF_DEN_R = 0x03;          // 7) enable digital I/O on PF4-0
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

// Port E ADC Init.
void PortE_AIN0_Init(void) { volatile unsigned long delay;
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
    ADC0_ACTSS_R        &= ~0x0008;    // 9) disable sample sequencer 3
    ADC0_EMUX_R        &= ~0xF000;     // 10) seq3 is software trigger
    ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+0; // 11) channel Ain0 (PE3)
    ADC0_SSCTL3_R       =  0x0006;     // 12) no TS0 D0, yes IE0 END0
    ADC0_ACTSS_R       |=  0x0008;     // 13) enable sample sequencer 3
}


/***************************************************************************
ADC0 function.
***************************************************************************/

// 12 bit ADC converter to convert value read from
// potentiometer. 
unsigned long ADC0_InSeq3(void){  
    unsigned long result;
    ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
    while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
    result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
    ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
    return result;
}



/***************************************************************************
Interrupts, ISRs
***************************************************************************/

// Iterates the variable 'millis'. Every 1000ms 'timer_says_okay' is toggled
// and debounces the button / updates the 'button_says_okay' flag.
// Status of button, 1 or 0.
unsigned char status;
// Number the timer was on when debounce started. 
unsigned long last_debounce_time = 0;
// Current, debounced state of button.
unsigned char button_state = 0;
// Button state on last iteration. 
unsigned char last_debounce_state = 0;
// Flag set when debounce ensures the button 
// was pressed. It gets flipped every time the
// button is pressed. 
unsigned char button_says_okay = 0;
// Length of delay required in ms.
unsigned long debounce_delay = 15;
unsigned long millis = 0;
unsigned char timer_says_okay = 0;
unsigned char tell_off = 0x00;

unsigned char delay40Hz=0;
void SysTick_Handler(void){
    
    delay40Hz = (delay40Hz+1)%25; // increment up to 25ms for 40Hz delay
    
    millis += 1;
    if( millis == 1000 ) {
        millis = 0x00;
        timer_says_okay ^= 0x01;
    }
    // Debounce the button and update button_state.
    if( GPIO_PORTF_DATA_R & 0x01 ) status = 0;
    else status = 1;
    //status = ( GPIO_PORTF_DATA_R & 0x01 ) ? 0 : 1;
    // If current state of button differs from last, then
    // count how long it has been in current state.
    if( status != last_debounce_state ) {
        last_debounce_time = millis;
    }
    // If button has been pushed for long enough and it's
    // different from before, update status and LEDs. 
    if( ( millis - last_debounce_time ) > debounce_delay ) {
        if( status != button_state ) {
            button_state = status;
            if( button_state == 1 ) {
                button_says_okay = 0x00;
                tell_off = 0x01;
            }
        }
    }
    last_debounce_state = status;
    
    // Turn LED off and on.
    if( timer_says_okay & button_says_okay )
        LED |=  0x02;
    else
        LED &= ~0x02; 
}


/***************************************************************************
 Main function / loop.
***************************************************************************/
//unsigned long ADC_Val;
//unsigned char UART1_in_char;
int main( void ) {

    unsigned long ADC_Val;
    unsigned char UART1_in_char;
    
    PLL_Init();             // 50MHz
    PortE_AIN0_Init();      // ADC0
    UART1_Init();           // UART1
    PortF_Init();           // PF1 out, PF0 in.
    SysTick_Init( 50000 );  // 1ms Interrupts
    
    WhenIFeelLikeWorking {
        
        if(delay40Hz == 0){

            // Check an input from MCU1 if the character input is 'r'.
            //  if the input is 'r' and the LED is not blinking yet,
            //  then set the LED blinking flag, so the LED will be blinking
            //  before sending confirmation token('r') and then send a
            //  confirmation message "RED LED is On".
            UART1_in_char = UART1_NonBlockingInChar();
            if( ( UART1_in_char == _r ) & ( button_says_okay == 0 ) ) {
                button_says_okay = 0x01;
                UART1_OutChar(_r);
                UART1_OutString( "Red LED is On" );
                UART1_OutChar( CR );
                
            }
            // When SW0 on MCU2 is pressed and being debounced,
            //  tell_off would be set HIGH. Once this condition is true,
            //  MCU2 sends a token('$') before sending a confirmation
            //  message "RED LED is off".
            else if( tell_off == 0x01 ) {
                tell_off = 0x00;
                UART1_OutChar(_$);
                UART1_OutString("Red LED is off");
                UART1_OutChar( CR );
                
            }
            // If there is no input from MCU1 and the SW0 is not
            //  being pressed, then MCU2 will send ADC value 
            //  to MCU1.
            //  First, sends a token('#') to MCU1 stating that
            //  the next output will be an ADC Value, then 
            //  get ADC value before sending the ADC value to MCU1
            else{
                UART1_OutChar(_sh);
                
                // Get and transmit ADC data from pot.
                ADC_Val = ADC0_InSeq3(); 
                UART1_OutUDec( ADC_Val );
                UART1_OutChar( CR );
                                
            }
        }
       
       
  }
}

