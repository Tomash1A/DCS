#include  "../header/api.h"    		// private library - API layer
#include  "../header/halGPIO.h"     // private library - HAL layer
#include "stdio.h"


unsigned int count_up = 0;
char count_up_str[5];
unsigned int* count_up_address = &count_up;
char ascii_char[3] = "";
int num_steps = 0;


void Paint(){
    //objectives:
    //1. send the averaged value of Vrx,y_global to computer relatively fast for updating
    //2. enable interrupts from JPB so it could change the pens function (write/erase/null)

}


//----------------Move Step Motor according to Joystick--------------------------
void StepMotor_by_Joystick(){
    //should take the avg'd readings from the global variables
    // and decide on the movement of the motor (speed, direction)
    while(state==2){
    read_avg_Joystick(&Vrx_global, &Vry_global);
    //
    }
}


////----------------Move Step Motor according to Joystick PB--------------------------
void StepMotor_phy_calibration(){
    //Notice: the StepMotor should be initialized to the zero position with StepMotor_by_Joystick
    //this function should wait on a first JPB click, start circling around
    //and stop upon second JPB click.
    //motor speed should be relatively slow so it won't be hard to stop on the second JPB click
    //there should be a global variable that receives phy's value
    num_steps = 0;  //initialize a counter to count number of steps in 360 deg.
    enable_JPB_interrupt();
    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ interrupt
    while(calib_flag==1){
        //build an HAL function to move step motor one step in a relatively slow pace
        num_steps++;
    }
    //calculate phy
}

//-------------------------------------------------------------
//                9. Real Time Task
//-------------------------------------------------------------
void realtime(){
    char_from_pc = (int)string2;  // Get delay time from user
    int2str(ascii_char, char_from_pc);
    lcd_puts(ascii_char);
    state = state8;
    }



//-------------------------------------------------------------
//                2. Count up to 65535
//------------------------------------------------------------
void count_up_LCD(){
    while(state==state2){
        lcd_clear();
        lcd_home();
        lcd_puts("Count Up: ");
        lcd_new_line;
        int2str(count_up_str, *count_up_address);
        lcd_puts(count_up_str);
        timer_call_counter();

        *count_up_address = (*count_up_address + 1) % 65536;
    }
}

//-------------------------------------------------------------
//                4. Change Delay Time [ms]
//------------------------------------------------------------
void change_delay_time(){
    delay_time = atoi(string1);  // Get delay time from user
    state = state8;
}


//-------------------------------------------------------------
//                6. Clear LCD and init counters
//------------------------------------------------------------
void clear_counters(){
    disable_interrupts();
    lcd_clear();
    lcd_home();
    count_up = 0;
    enable_interrupts();
    state = state8;
}


