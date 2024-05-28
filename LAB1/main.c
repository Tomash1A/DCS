#include  "api.h"    		// private library - API layer
#include  "app.h"    		// private library - APP layer

enum FSMstate state;
enum SYSmode lpm_mode;
int currentLEDposition =0x01;
unsigned int val = 1;

void main(void){
  
  state = state0;  // start in idle state on RESET
  lpm_mode = mode0;     // start in idle state on RESET
  sysConfig();
  
  while(1){
	switch(state){
	  case state0:
		printSWs2LEDs();
                enterLPM(lpm_mode);
		break;
	
                // Using PB0: Increase LED       
	  case state1: 
		disable_interrupts();
                volatile unsigned int j;
	        for(j=20; j>0; j--){
                  if(PBsArrIntPend & PB0){
                    if (val == 1){
                      val = -1;
                    } else if (val ==-1){
                    val = 1;
                    }
                    PBsArrIntPend &= 0xFE;
                  }
                  incLEDs(val);
		  delay(LEDs_SHOW_RATE);	
                  }
                depressInterrupt(0xFE);
		enable_interrupts();
		break;
		
                // Using PB1: move single LED right to left
	  case state2:
		disable_interrupts();
                volatile unsigned int i;
	        for(i=14; i>0; i--){
                  currentLEDposition <<= 1; //move the bit to the left
                  if (currentLEDposition == 256){
                      currentLEDposition = 1;
                  }
                  setLEDs(currentLEDposition);
                  delay(LEDs_SHOW_RATE);
                }
                depressInterrupt(0xFD);
                enable_interrupts();
		break;
                
          case state3: //PB2
          // Do all math calc here to avoid more cycles inside  pwm func
      
         unsigned int freq = 4;
                float dutyCycle = 0.84;
                float period_time = 92.0/freq;
                unsigned int Ton =  dutyCycle * period_time - 2;
                unsigned int Toff = (1 - dutyCycle) * period_time - 5;
                while(state == state3){
          
       disable_interrupts();
         
        P2OutputPort |= 0x80;  // Set P2.7 to '1'
          
       delay(Ton);
         
        P2OutputPort &= 0x7F;    // Set P2.7 to '0'
         
        delay(Toff);
         
        enable_interrupts();}
                break;
	}
 //        case state4: //PB3 
                
      
  }
}
