#include  "../header/halGPIO.h"     // private library - HAL layer
#include "stdio.h"
#include <msp430.h>
#include <stdint.h>
#include <string.h>

// Global Variables
char string1[5];
char string2;
unsigned int i;
int j=0;
unsigned int delay_time = 500;
unsigned int hundred_ms = 100;
unsigned int five_ms = 5;
int char_from_pc ;
const unsigned int timer_half_sec = 65535;
unsigned int Vrx_global = 0x0000;
unsigned int Vry_global = 0x0000;
unsigned int phy_global;
int calib_flag = 0;


//For Joystick Voltage reading

volatile unsigned int adc_results[TOTAL_SAMPLES];   // Create an array to store the ADC results
volatile unsigned char sampling_complete = 0;

//For script states:
static unsigned int flash_address_script_1 = 0x1000;
int upload_scr_1_completed = 0;
//unsigned int reset_flag= 0;

//--------------------------------------------------------------------
//             System Configuration  
//--------------------------------------------------------------------
void sysConfig(void){ 
	GPIOconfig();
	StopAllTimers();
	lcd_init();
	lcd_clear();
	UART_init();
}
//--------------------------------------------------------------------
//--------------------------------------------------------------------
// 				Set Byte to Port
//--------------------------------------------------------------------
void print2RGB(char ch){
    RGBArrPortOut = ch;
} 

//---------------------------------------------------------------------
//            General Function
//---------------------------------------------------------------------

void uart_putc(unsigned char c) {
    while (!(IFG2 & UCA0TXIFG));        // Wait for TX buffer to be ready
    UCA0TXBUF = c;                      // Send character
    IE2 |= UCA0TXIE;                    // Enable USCI_A0 TX interrupt
}

void uart_puts(const char* str) {
    while(*str) uart_putc(*str++);
    uart_putc('\n');
    IE2 &= ~UCA0TXIE;                    // Disable USCI_A0 TX interrupt
}


void ADC_Joystick_sample(unsigned int* Vrx_result, unsigned int* Vry_result) {
    sampling_complete = 0;  // Reset the flag
    int  iterator = 0;
    unsigned int Vrx_temp = 0;
    unsigned int Vry_temp = 0;
    // Configure ADC10
     ADC10CTL0 = SREF_0 + ADC10SHT_2 + MSC + ADC10ON + ADC10IE;
     ADC10CTL1 = INCH_3 + CONSEQ_3 + ADC10DIV_0 + ADC10SSEL_0;
     ADC10AE0 |= BIT0 + BIT3;    // Enable A0 (P1.0) and A3 (P1.3) as analog inputs
     ADC10DTC1 = TOTAL_SAMPLES;  // Number of transfers

     // Configure DTC
     ADC10SA = (unsigned int)adc_results; // Start address for storage

    ADC10CTL0 &= ~ENC;              // Disable ADC conversion
    while (ADC10CTL1 & BUSY);       // Wait if ADC10 core is active

    // Configure channel sequence
    ADC10CTL1 &= ~(INCH_3);         // Clear channel select
    ADC10CTL1 |= INCH_3;            // Set to highest channel (A3)

    ADC10CTL0 |= ENC + ADC10SC;     // Enable and start conversion

    // Wait for sampling to complete
    while (!sampling_complete) {
        __bis_SR_register(LPM0_bits + GIE);  // Enter LPM0 with interrupts enabled
    }
    for(iterator = 0; iterator<4; iterator++){
        Vrx_temp += adc_results[iterator *4];
        Vry_temp += adc_results[3 +iterator *4];
    }

    Vrx_global= (Vrx_temp >> 2) + 1;    //+1 for PC_side code (crashes at 0).
    Vry_global = (Vry_temp >> 2) + 1;
}


//----------------------Int to String---------------------------------
void int2str(char *str, unsigned int num){
    int strSize = 0;
    long tmp = num, len = 0;
    int j;
    // Find the size of the intPart by repeatedly dividing by 10
    while(tmp){
        len++;
        tmp /= 10;
    }

    // Print out the numbers in reverse
    for(j = len - 1; j >= 0; j--){
        str[j] = (num % 10) + '0';
        num /= 10;
    }
    strSize += len;
    str[strSize] = '\0';
}
//----------------------Count Timer Calls---------------------------------
void timer_call_counter(int delay){

    unsigned int num_of_halfSec;

    num_of_halfSec = (int) delay / half_sec;
    unsigned int res;
    res = delay % half_sec;
    res = res * clk_tmp;

    for (i=0; i < num_of_halfSec; i++){
        TIMER_A0_config(timer_half_sec);
        __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
    }

    if (res > 1000){
        TIMER_A0_config(res);
        __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ int until Byte RXed
    }
}


void step_motor_mover(){
    if((Vrx_global >= 0x027F) && (Vrx_global <= 0x02FF)){
        clockwise_step(20);
    }
    else if((Vrx_global >= 0x02FF) && (Vrx_global <= 0x03FF)){
        clockwise_step(8);
    }
    //counter clock wise
    else if((Vrx_global >= 0x0100) && (Vrx_global <= 0x0180)){
        counter_clockwise_step(20);
    }
    else if((Vrx_global >= 0x0000) && (Vrx_global <= 0x0100)){
        counter_clockwise_step(8);
    }
}

void clockwise_step(unsigned int t){
    phase(Phase_A, t);
    phase(Phase_D, t);
    phase(Phase_C, t);
    phase(Phase_B, t);
}
void clockwise_half_step(int t){
    phase(Phase_D, t);
    phase(Phase_D+Phase_C, t);
    phase(Phase_C, t);
    phase(Phase_C+Phase_B, t);
    phase(Phase_B, t);
    phase(Phase_B+Phase_A, t);
    phase(Phase_A, t);
    phase(Phase_A+Phase_D, t);
}

void counter_clockwise_step(int t){
    phase(Phase_D, t);
    phase(Phase_A, t);
    phase(Phase_B, t);
    phase(Phase_C, t);
}

void phase(unsigned int phase, int t){
    StepMotorPortOut |= phase;
    timer_call_counter(t);
    StepMotorPortOut &= ~phase;
}

//---------------------------------------------------------------------
//            Enter from LPM0 mode (part of timer
//---------------------------------------------------------------------
void enterLPM(unsigned char LPM_level){
	if (LPM_level == 0x00) 
	  _BIS_SR(LPM0_bits);     /* Enter Low Power Mode 0 */
        else if(LPM_level == 0x01) 
	  _BIS_SR(LPM1_bits);     /* Enter Low Power Mode 1 */
        else if(LPM_level == 0x02) 
	  _BIS_SR(LPM2_bits);     /* Enter Low Power Mode 2 */
	else if(LPM_level == 0x03) 
	  _BIS_SR(LPM3_bits);     /* Enter Low Power Mode 3 */
        else if(LPM_level == 0x04) 
	  _BIS_SR(LPM4_bits);     /* Enter Low Power Mode 4 */
}
//---------------------------------------------------------------------
//            Enable/Disable interrupts
//---------------------------------------------------------------------
void enable_interrupts(){
  _BIS_SR(GIE);
}

void disable_interrupts(){
  _BIC_SR(GIE);
}


void send_num_steps_to_pc(int num){
   UCA0TXBUF = num;
   IE2 |= UCA0TXIE;
}
//----------------------------------------------------------------------
//                          FLASH Functions
//----------------------------------------------------------------------

void write_char_to_flash(char rx_char, int flash_address) {
    if(rx_char != 0xFF){
        char *flash_ptr;
        flash_ptr = (char *)flash_address;

        // Check if we're still within our defined limits

        FCTL3 = FWKEY;
        FCTL1 = FWKEY | WRT;

        // Write the character to flash memory
        *flash_ptr = rx_char;

        FCTL1 = FWKEY;
        FCTL3 = FWKEY | LOCK;
    }
    else{
       upload_scr_1_completed = 1;
    }
}


void erase_segment(int address){
    char *flash_ptr;
    unsigned int i;
    flash_ptr = (char *)address;
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | ERASE;
    *flash_ptr = 0;
    FCTL1 = FWKEY | WRT;
    for(i = 0; i < 0x0050; i++)
        *flash_ptr++ = 0XFF;
    FCTL1 = FWKEY;
    FCTL3 = FWKEY | LOCK;
}


//----------------------------------------------------------------------
//                          Exexute Script
//----------------------------------------------------------------------
void scriptEx(int flash_address){
    char *flash_ptr;
    flash_ptr = (char *)flash_address;
    int i=0; // index
    int finish_flag = 0;
    int command;
    int arg1;
    int arg2;
    while((i < 70) && (finish_flag ==0)){  //execute ten
//        command = *(flash_ptr+i);
//        command = (*(flash_ptr+i) << 8) | *(flash_ptr+i+1);
        command = *(flash_ptr + i);
        i++;
        switch(command){
        case 0x01:
            arg1 = *(flash_ptr + i);
            count_up_LCD(arg1);
            break;
        case 0x02:
            arg1 = *(flash_ptr + i);
            count_down_LCD(arg1);
            break;
        case 0x03:
            arg1 = *(flash_ptr + i);
            Rotate_right(arg1);
//            Rotate_right(arg1);
            break;
        case 0x04: // set_delay
            arg1 = *(flash_ptr + i);
            delay_time = arg1* 10;
            break;
        case 0x05: // clear_lcd
            lcd_clear();
            lcd_home();
            break;
        case 0x06: // servo_de
            arg1 = *(flash_ptr + i);
            i++;        //only this incremnet is needed because this func has two arguments
//            arg1 = *(flash_ptr+i);
//            posInput = arg1 / 3; // posInput is in index
//            posInput = (posInput == MEAS_NUM) ? (MEAS_NUM-1) : posInput; // If arg1 is 180 degree
//            rotateToPosGrad(posInput);
//            sendAck();
//            telemeter();
//            delayWrapper(lon);
            break;
        case 0x07: // servo_scan
            arg1 = *(flash_ptr + i);
//            arg1 = *(pnt+i) / 3;
//            i++;
//            i++;
//            arg1 = word_from_flash(flash_ptr);
//            arg2 = *(pnt+i) / 3;
//            arg2 = (arg2 == MEAS_NUM) ? (MEAS_NUM-1) : arg2; // If arg2 is 180 degree
//            servo_scan(arg1, arg2);
//            delayWrapper(lon);
            break;
        case 0x08: // sleep
            return; // Exit function - Back to state7
        case 0xFF:
            finish_flag = 1;
        }
        i++;
    }
    finish_flag = 0; // Reset flag for communication
}

//---------------------------------------------------------------------
//            LCD
//---------------------------------------------------------------------
//******************************************************************
// send a command to the LCD
//******************************************************************
void lcd_cmd(unsigned char c){

    LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

    if (LCD_MODE == FOURBIT_MODE)
    {
        LCD_DATA_WRITE &= ~OUTPUT_DATA;// clear bits before new write
        LCD_DATA_WRITE |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;
        lcd_strobe();
        LCD_DATA_WRITE &= ~OUTPUT_DATA;
        LCD_DATA_WRITE |= (c & (0x0F)) << LCD_DATA_OFFSET;
        lcd_strobe();
    }
    else
    {
        LCD_DATA_WRITE = c;
        lcd_strobe();
    }
}
//******************************************************************
// send data to the LCD
//******************************************************************
void lcd_data(unsigned char c){

    LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h

    LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_RS(1);
    if (LCD_MODE == FOURBIT_MODE)
    {
            LCD_DATA_WRITE &= ~OUTPUT_DATA;
            LCD_DATA_WRITE |= ((c >> 4) & 0x0F) << LCD_DATA_OFFSET;
            lcd_strobe();
            LCD_DATA_WRITE &= (0xF0 << LCD_DATA_OFFSET) | (0xF0 >> 8 - LCD_DATA_OFFSET);
            LCD_DATA_WRITE &= ~OUTPUT_DATA;
            LCD_DATA_WRITE |= (c & 0x0F) << LCD_DATA_OFFSET;
            lcd_strobe();
    }
    else
    {
            LCD_DATA_WRITE = c;
            lcd_strobe();
    }

    LCD_RS(0);
}
//******************************************************************
// write a string of chars to the LCD
//******************************************************************
void lcd_puts(const char * s){

    while(*s)
        lcd_data(*s++);
}
//******************************************************************
// initialize the LCD
//******************************************************************
void lcd_init(){

    char init_value;

    if (LCD_MODE == FOURBIT_MODE) init_value = 0x3 << LCD_DATA_OFFSET;
    else init_value = 0x3F;

    LCD_RS_DIR(OUTPUT_PIN);
    LCD_EN_DIR(OUTPUT_PIN);
    LCD_RW_DIR(OUTPUT_PIN);
    LCD_DATA_DIR |= OUTPUT_DATA;
    LCD_RS(0);
    LCD_EN(0);
    LCD_RW(0);

    DelayMs(15);
    LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_DATA_WRITE |= init_value;
    lcd_strobe();
    DelayMs(5);
    LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_DATA_WRITE |= init_value;
    lcd_strobe();
    DelayUs(200);
    LCD_DATA_WRITE &= ~OUTPUT_DATA;
    LCD_DATA_WRITE |= init_value;
    lcd_strobe();

    if (LCD_MODE == FOURBIT_MODE){
        LCD_WAIT; // may check LCD busy flag, or just delay a little, depending on lcd.h
        LCD_DATA_WRITE &= ~OUTPUT_DATA;
        LCD_DATA_WRITE |= 0x2 << LCD_DATA_OFFSET; // Set 4-bit mode
        lcd_strobe();
        lcd_cmd(0x28); // Function Set
    }
    else lcd_cmd(0x3C); // 8bit,two lines,5x10 dots

    lcd_cmd(0xF); //Display On, Cursor On, Cursor Blink
    lcd_cmd(0x1); //Display Clear
    lcd_cmd(0x6); //Entry Mode
    lcd_cmd(0x80); //Initialize DDRAM address to zero
}
//******************************************************************
// lcd strobe functions
//******************************************************************
void lcd_strobe(){
  LCD_EN(1);
  asm("NOP");
 // asm("NOP");
  LCD_EN(0);
}
//---------------------------------------------------------------------
//                     Polling delays
//---------------------------------------------------------------------
//******************************************************************
// Delay usec functions
//******************************************************************
void DelayUs(unsigned int cnt){

    unsigned char i;
    for(i=cnt ; i>0 ; i--) asm("nop"); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
// Delay msec functions
//******************************************************************
void DelayMs(unsigned int cnt){

    unsigned char i;
    for(i=cnt ; i>0 ; i--) DelayUs(1000); // tha command asm("nop") takes raphly 1usec

}
//******************************************************************
//            Polling based Delay function
//******************************************************************
void delay(unsigned int t){  //
    volatile unsigned int i;

    for(i=t; i>0; i--);
}
//---------------**************************----------------------------
//               Interrupt Services Routines
//---------------**************************----------------------------
//*********************************************************************
//    Port2 Interrupt Service Routine - only JPB uses it
//*********************************************************************
#pragma vector=PORT2_VECTOR  // For Push Buttons
  __interrupt void PBs_handler(void){
    delay(debounceVal);
    //add a state dependence in the condition
    IE2 &= ~UCA0TXIE;                    // Disable USCI_A0 TX interrupt

    //state2- paint

    if((JoystickPBIntPend & JPBMask) && (state==state2)){
        //should transmit the click and let the computer deal with it
        JoystickPBIntPend &= ~JPBMask;
        UCA0TXBUF = 'P';
        IE2 |= UCA0TXIE;

    }
    //state3- calibrate phy mission
    else if((JoystickPBIntPend & JPBMask) && (state==state3)){
        disable_JPB_interrupt();
        //first click should start step motor, second one should stop it
        //in here it only increments the calib_flag, the rest is done in the function
        calib_flag++;
        JoystickPBIntPend &= ~JPBMask;
    }
    else if((JoystickPBIntPend & JPBMask) && (state==state1)){
        JoystickPBIntPend &= ~JPBMask;
    }

//---------------------------------------------------------------------
//            Exit from a given LPM
//---------------------------------------------------------------------
        switch(lpm_mode){
        case mode0:
         LPM0_EXIT; // must be called from ISR only
         break;

        case mode1:
         LPM1_EXIT; // must be called from ISR only
         break;

        case mode2:
         LPM2_EXIT; // must be called from ISR only
         break;

        case mode3:
         LPM3_EXIT; // must be called from ISR only
         break;

        case mode4:
         LPM4_EXIT; // must be called from ISR only
         break;
    }

}
//*********************************************************************
//                        TIMER A0 ISR
//*********************************************************************
#pragma vector = TIMER0_A0_VECTOR // For delay
__interrupt void TimerA_ISR (void)
{
    StopAllTimers();
    LPM0_EXIT;
}


//*********************************************************************
//                         ADC10 ISR
//*********************************************************************
#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
    sampling_complete = 1;  // Set the flag
    __bic_SR_register_on_exit(CPUOFF);
}

//*********************************************************************
//                           TX ISR
//*********************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCI0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
}


//*********************************************************************
//                         RX ISR
//*********************************************************************
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    if((state==state8) || (state==state1) ||(state==state2) ||(state==state3) ||(state==state4)){    //If states 0-4
//    if(receiving_script==0){
        if(UCA0RXBUF == '1'){
            state = state1;
            UCA0TXBUF = '1';    //Send 'ack' to PC by TXing '1'
            IE2 |= UCA0TXIE;
        }
        else if(UCA0RXBUF == '2'){
            state = state2;
            UCA0TXBUF = '2';
            IE2 |= UCA0TXIE;    //Notify PC side that Joystick voltage is arriving, by sending "2"
        }

        else if(UCA0RXBUF == '3'){
            state = state3;
            calib_flag = 0;
            UCA0TXBUF = '3';
            IE2 |= UCA0TXIE;    //Return ack to computer, move to state 3
        }
        else if(UCA0RXBUF == '4'){
            state = state8;
            UCA0TXBUF = 'F';
            IE2 |= UCA0TXIE;    //Do nothing. Turn off CPU, nothing to do as script work has its own states.
        }
        else if(UCA0RXBUF == '5'){
            state = state5;
            flash_address_script_1 = 0x1000;
            erase_segment(flash_address_script_1);
            upload_scr_1_completed = 0;
//            UCA0TXBUF = 'F';
//            IE2 |= UCA0TXIE;    //Move to state 5 as an incoming script is on its way
        }
        else if(UCA0RXBUF == '8'){
            state = state6;
            UCA0TXBUF = 'F';
            IE2 |= UCA0TXIE;    //Move to state 5 as an incoming script is on its way
        }
        //wise guys proofing: (so it wont crash on invalid key)
        else {
            state = state8;
            UCA0TXBUF = 'F';
            IE2 |= UCA0TXIE;
        }
    }
    else{
        // In your main loop or UART receive interrupt handler:
        if ((state == state5) && (upload_scr_1_completed == 0)){
                write_char_to_flash(UCA0RXBUF, flash_address_script_1);
                flash_address_script_1++;
        }
//        else{   // will use states 6-10 for other scripts
//            break;
//        }
    }

    switch(lpm_mode){
    case mode0:
        LPM0_EXIT; // must be called from ISR only
        break;
    case mode1:
        LPM1_EXIT; // must be called from ISR only
        break;
    case mode2:
        LPM2_EXIT; // must be called from ISR only
        break;
    case mode3:
        LPM3_EXIT; // must be called from ISR only
        break;
    case mode4:
        LPM4_EXIT; // must be called from ISR only
        break;
    }
}


//*********************************************************************
//                         RESET ISR
//*********************************************************************
//#pragma vector = RESET_VECTOR
//__interrupt void RESET_ISR (void)
//{
//    state = state8;  // start in idle state on RESET
//    lpm_mode = mode0;     // start in idle state on RESET
//    reset_flag = 1;
//    sysConfig();     // Configure GPIO, Stop Timers, Init LCD
//
//}
