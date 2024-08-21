#include  "../header/bsp.h"    // private library - BSP layer

//-----------------------------------------------------------------------------  
//           GPIO configuration
//-----------------------------------------------------------------------------
void GPIOconfig(void){
  WDTCTL = WDTHOLD | WDTPW;		// Stop WDT

  // LCD configuration
  LCD_DATA_WRITE &= ~0xFF;
  LCD_DATA_DIR |= 0xF0;    // P1.4-P1.7 To Output('1')
  LCD_DATA_SEL &= ~0xF0;   // Bit clear P1.4-P1.7
  LCD_CTL_SEL  &= ~0xE0;   // Bit clear P2.5-P2.7
  
  // Joystick PB configuration (Input PB)
  JoystickPBPortSEL     &= ~JPBMask;    //Set I/O mode
  JoystickPBPortDir     &= ~JPBMask;    //Set as input
  JoystickPBPortOut     |= JPBMask;    //set out as 0
  JoystickPBIntEn       &= ~JPBMask;    //Disable Interrupts (enable them again upon state being called)
//  P2OUT |= BIT4;              // Set P2.4 output to high (pull-up resistor)
  P2REN |= BIT4;              // Enable pull-up resistor for P2.4
  JoystickPBIntEdgeSel  |= JPBMask;    //set as PD mode (for PU: |= JPBMask) (PD: &= ~JPBMask;)
  JoystickPBIntPend     &= ~JPBMask;    //Set as no Interrupt pending.


  // Step Motor Configuration (Output from P2.0-3)
  StepMotorPortSEL      &= ~StepMotorMask; //Set I/O mode
  StepMotorPortDir      |=  StepMotorMask;  //Set as Output
  StepMotorPortOut      &= ~StepMotorMask; //Set outputs to '0'


  _BIS_SR(GIE);                     // enable interrupts globally
}
//-------------------------------------------------------------------------------------
//            Stop All Timers
//-------------------------------------------------------------------------------------
void StopAllTimers(void){
    TACTL = MC_0; // halt timer A

}
//-------------------------------------------------------------------------------------
//            ADC configuration -from lab4
//-------------------------------------------------------------------------------------
void ADC_config(void){
      ADC10CTL0 = ADC10SHT_2 + ADC10ON+ SREF_0 + ADC10IE;  // 16*ADCLK+ Turn on, set ref to Vcc and Gnd, and Enable Interrupt
      ADC10CTL1 = INCH_3 + ADC10SSEL_3;     // Input A3 and SMCLK, was |
      ADC10AE0 |= BIT3;                         // P1.3 ADC option select
}
//
////-------------------------------------------------------------------------------------
////            ADC for Joystick configuration
////-------------------------------------------------------------------------------------
void ADC_Joystick_config(unsigned int mask){
      ADC10CTL0 = ADC10SHT_2 + ADC10ON+ SREF_0 + ADC10IE;  // 16*ADCLK+ Turn on, set ref to Vcc and Gnd, and Enable Interrupt
      ADC10CTL1 = INCH_3 + ADC10SSEL_3;     // Input A3 and SMCLK, was |
      ADC10AE0 |= mask;                         // P1.3 ADC option select
}


//-------------------------------------------------------------------------------------
//            Enable/Disable Joystick Push Button Interrupt
//-------------------------------------------------------------------------------------
void enable_JPB_interrupt(){
    JoystickPBIntEn |= JPBMask;
}

void disable_JPB_interrupt(){
    JoystickPBIntEn &= ~JPBMask;
}

//-------------------------------------------------------------------------------------
//            Timer A0 configuration
//-------------------------------------------------------------------------------------
void TIMER_A0_config(unsigned int counter){
    TACCR0 = counter; // (2^20/8)*345m = 45219 -> 0xB0A3
    TACCTL0 = CCIE;
    TA0CTL = TASSEL_2 + MC_1 + ID_3;  //  select: 2 - SMCLK ; control: 1 - Up ; divider: 3 - /8
}

//-------------------------------------------------------------------------------------
//            Timer A1 PWM configuration - For state 3
//-------------------------------------------------------------------------------------
    void TIMER1_A1_config(void){
        TA1CTL = TASSEL_2 + MC_1; // SMCLK, upmode - up to CCR0
        TA1CCTL2 =  OUTMOD_7; // TA1CCR1 reset/set;
        // OUTMOD_7 - PWM output mode: 7 - PWM reset/set */
    }

    void TIMER1_A1_stop(void){
        TA1CCR0 = 0x00;
        TA1CCR2 = 0x01;
        TA1CTL |= MC_0; //halt
    }
//-------------------------------------------------------------------------------------
//                              UART init
//-------------------------------------------------------------------------------------
void UART_init(void){
    if (CALBC1_1MHZ==0xFF)                  // If calibration constant erased
      {
        while(1);                               // do not load, trap CPU!!
      }
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
    DCOCTL = CALDCO_1MHZ;

//    P2DIR = 0xFF;                             // All P2.x outputs
//    P2OUT = 0;                                // All P2.x reset
    P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
    P1SEL2 = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
    P1DIR |= RXLED + TXLED;
    P1OUT &= 0x00;

    UCA0CTL1 |= UCSSEL_2;                     // CLK = SMCLK
    UCA0BR0 = 104;                           //
    UCA0BR1 = 0x00;                           //
    UCA0MCTL = UCBRS0;               //
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}
