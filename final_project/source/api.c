#include  "../header/api.h"    		// private library - API layer
#include  "../header/halGPIO.h"     // private library - HAL layer
#include "stdio.h"


//unsigned int count_up = 0;
//char count_up_str[5];
//unsigned int* count_up_address = &count_up;
char ascii_char[3] = "";
int num_steps = 0;
char Vrx_char[6], Vry_char[6];

void Paint(){
    //objectives:
    //1. send the averaged value of Vrx,y_global to computer relatively fast for updating
    //2. enable interrupts from JPB so it could change the pens function (write/erase/null)

}

void send_JS_data_to_comp(){
    //objectives:
    //1. send the averaged value of Vrx,y_global to computer relatively fast for updating
    //2. enable interrupts from JPB so it could change the pens function (write/erase/null)
//    timer_hundred_ms_counter();  //give PC time to get 'ack' on '2'
    enable_JPB_interrupt();
    while(state==state2){
    read_avg_Joystick(&Vrx_global, &Vry_global);
    int2str(Vrx_char, Vrx_global);
    uart_puts(Vrx_char);
    timer_hundred_ms_counter();  //give PC time to get 'ack' on '2'
    read_avg_Joystick(&Vrx_global, &Vry_global);
    int2str(Vry_char, Vry_global);
    uart_puts(Vry_char);
    timer_hundred_ms_counter();  //give PC time to get 'ack' on '2'
    }
    disable_JPB_interrupt();
}

//----------------Move Step Motor according to Joystick--------------------------
void StepMotor_by_Joystick(){
    //should take the avg'd readings from the global variables
    // and decide on the movement of the motor (speed, direction)
    timer_hundred_ms_counter();  //give PC time to get 'ack' on '2'
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

void upload_script_1(){
    //maybe we should do a while "not ready", to hold it until script is uploaded to buffer
    // and then to use write_to_flash() in order to write to flash
    if(upload_scr_1_completed == 1){
        state = state8;
        UCA0TXBUF = '5';
        IE2 |= UCA0TXIE;
    }
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
//                Script- 0x01 Count up to argument
//------------------------------------------------------------
void count_up_LCD(int arg){
    lcd_clear();        //initialize LCD
    lcd_home();
    lcd_puts("Count Up: ");
    lcd_new_line;
    unsigned int count_up = 0;  //initialize Counter
    char count_up_str[3] = '000';   //initialize str buffer (LCD takes str)
    unsigned int* count_up_address = &count_up;
    while(count_up < arg){
        int2str(count_up_str, *count_up_address);
        lcd_new_line;
        lcd_puts(count_up_str);
        timer_call_counter();
        *count_up_address = (*count_up_address + 1) % 65536;
    }
}

//-------------------------------------------------------------
//                Script- 0x02 Count Down from argument
//------------------------------------------------------------
void count_down_LCD(int arg){
    lcd_clear();        //initialize LCD
    lcd_home();
    lcd_puts("Count Down: ");
    lcd_new_line;
    unsigned int count_down = arg;  //initialize Counter
    char count_down_str[3] = '000';   //initialize str buffer (LCD takes str)
    unsigned int* count_down_address = &count_down;
    while(count_down > 0){
        int2str(count_down_str, *count_down_address);
        lcd_new_line;
        lcd_puts(count_down_str);
        timer_call_counter();
        *count_down_address = (*count_down_address - 1);
    }
}
//-------------------------------------------------------------
//               Script- 0x03 Rotate char argument on LCD
//------------------------------------------------------------
void Rotate_right(int arg){
    lcd_clear();        //initialize LCD
    lcd_home();
    unsigned int *char_int = arg;  //initialize Counter
    char c[3] = '000';   //initialize str buffer (LCD takes str)
    int2str(c, char_int);
    lcd_puts(c);
    timer_call_counter();
    int rra_counter = 31;
    while((rra_counter >15) && ((rra_counter <= 31))){
        lcd_cursor_left();
        lcd_puts(" ");
        lcd_puts(c);
        timer_call_counter();
        rra_counter--;
        }
    lcd_clear();
    lcd_new_line;
    while((rra_counter <= 16) && (rra_counter >= 0)){
        lcd_cursor_left();
        lcd_puts(" ");
        lcd_puts(c);
        timer_call_counter();
        rra_counter--;
        }
    }

//-------------------------------------------------------------
//                6. Clear LCD and init counters
//------------------------------------------------------------
void clear_counters(){
    disable_interrupts();
    lcd_clear();
    lcd_home();
//    count_up = 0;
    enable_interrupts();
    state = state8;
}


