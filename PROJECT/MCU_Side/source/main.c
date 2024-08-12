#include  "../header/api.h"    		// private library - API layer
#include  "../header/app.h"    		// private library - APP layer
#include  "../header/halGPIO.h"         // private library - APP layer
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

	case state1: //Control step motor with Joystick
	    StepMotor_by_Joystick();
	    state = state8;
	    break;

	case state2:
	    send_JS_data_to_comp();
	    break;
	case state3:
	    StepMotor_phy_calibration();
	case state5:
	    upload_script_1();
	    break;
	case state6:
	    scriptEx(0x1000);
	    state = state8;
		
	}
  }
}


  
  
  
