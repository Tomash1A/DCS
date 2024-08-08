#ifndef _bsp_H_
#define _bsp_H_

#include  <msp430g2553.h>          // MSP430x2xx



#define   debounceVal      10000


// RGB abstraction
#define RGBArrPortOut      P2OUT
#define RGBArrPortDir      P2DIR
#define RGBArrPortSEL      P2SEL
#define RGBMASK            0x07 //P2.0-P2.2



// LCDs abstraction : Data P1.4-7, Ctl P2.5-7
#define LCD_DATA_WRITE     P1OUT
#define LCD_DATA_DIR       P1DIR
#define LCD_DATA_READ      P1IN
#define LCD_DATA_SEL       P1SEL
#define LCD_CTL_SEL        P2SEL


//   Buzzer abstraction - P2.4 = Timer1_A3.TA2
#define BuzzPortSel        P2SEL
#define BuzzPortDir        P2DIR
#define BuzzPortOut        P2OUT
#define BUZZMASK           BIT4     //P2.4

//// Joystick Abstraction -Vrx, Vry
//#define JoystickDataPort         P1IN
//#define JoystickDataPortOut      P1OUT
//#define JoystickDataPortSEL      P1SEL
//#define JoystickDataPortDir      P1DIR
//#define JoystickDataIntEn        P2IE
//#define JoystickDataIntEdgeSel   P2IES
#define VrxMask                  0x00 //Vrx -> P1.0
#define VryMask                  0x11 //Vry -> P1.3

// Joystick Abstraction - Push Button
#define JoystickPBPort         P2IN
#define JoystickPBPortSEL      P2SEL
#define JoystickPBPortDir      P2DIR
#define JoystickPBPortOut      P2OUT
#define JoystickPBIntPend      P2IFG
#define JoystickPBIntEn        P2IE
#define JoystickPBIntEdgeSel   P2IES    //for deciding pull up(1)/down(0)
#define JPBMask                BIT4 // JPB -> P2.4

// Step Motor Abstraction : P2.0-3
#define StepMotorPortSEL     P2SEL
#define StepMotorPBPortDir   P2DIR
#define StepMotorPBPortOut   P2OUT
#define StepMotorPBMask      (BIT0+BIT1+BIT2+BIT3)
#define Phase_A              BIT0 // Phase_A -> P2.0
#define Phase_B              BIT1 // Phase_B -> P2.1
#define Phase_C              BIT2 // Phase_C -> P2.2
#define Phase_D              BIT3 // Phase_D -> P2.3

//// PushButton 3 abstraction for Main Lab
//#define PB3sArrPort         P2IN
//#define PB3sArrIntPend      P2IFG
//#define PB3sArrIntEn        P2IE
//#define PB3sArrIntEdgeSel   P2IES
//#define PB3sArrPortSel      P2SEL
//#define PB3sArrPortDir      P2DIR
//#define PB3sArrPortOut      P2OUT

//// PushButtons abstraction
//#define PBsArrPort	       P1IN
//#define PBsArrIntPend	   P1IFG
//#define PBsArrIntEn	       P1IE
//#define PBsArrIntEdgeSel   P1IES
//#define PBsArrPortSel      P1SEL
//#define PBsArrPortDir      P1DIR
//#define PBsArrPortOut      P1OUT
//#define PB0                0x01   // P1.0 Optional Place
//#define PB1                0x02  //  P1.1 Optional Place
//#define PB2                0x04  //  P1.2 Optional Place
////#define PB3                0x08  //

#define TXLED BIT0
#define RXLED BIT6
#define TXD BIT2
#define RXD BIT1


extern void GPIOconfig(void);
extern void ADC_config(void);
extern void TIMER_A0_config(unsigned int counter);
extern void TIMER1_A1_config(void);
extern void TIMER1_A1_stop(void);
extern void TIMERB_config(void);
extern void TIMERB_config_Task3(void);
extern void StopAllTimers(void);
extern void UART_init(void);
extern void ADC_Joystick_config(unsigned int mask);


extern void disable_JPB_interrupt();
extern void enable_JPB_interrupt();
#endif



