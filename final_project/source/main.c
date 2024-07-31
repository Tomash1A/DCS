#include  "../header/api.h"    		// private library - API layer
#include  "../header/app.h"    		// private library - APP layer
#include  <stdio.h>

enum FSMstate state;
enum SYSmode lpm_mode;


void main(void){
  
  state = state8;  // start in idle state on RESET
  lpm_mode = mode0;     // start in idle state on RESET
  sysConfig();     // Configure GPIO, Stop Timers, Init LCD


  while(1){
	switch(state){
	case state8: //idle
	    IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
	    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ interrupt
	    break;

	case state1: //send Readings from Joystick to CPU.
	    //

		
	}
  }
}


///// state machine for lab4:
  
//  case state1: // Blink RGB LED
//      blinkRGB();
//      RGBArrPortOut = 0;
//      break;
//  case state2: // Count up onto LCD
//      count_up_LCD();
//      break;
//
//  case state3: ; // Count down onto LCD
//      gen_tone();
//      BuzzPortOut &= ~BUZZMASK;            // P2.4 out = '0'
//      break;
//
//  case state4: // Change Delay Time in ms
//      change_delay_time();
//      break;
//
//    case state5: // Measure Potentiometer 3-digit value
//        measure_pot();
//        break;
//
//    case state6: // Clear Counts and LCD
//        clear_counters();
//        break;
//
//    case state7: // Print Menu to PC
//        state = state8;
//        break;
//
//    case state9: // for realtime task
//        lcd_clear();
//        realtime();
//        break;
  
  
  
  
