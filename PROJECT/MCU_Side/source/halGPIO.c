#include  "../header/halGPIO.h"     // private library - HAL layer
#include "stdio.h"
#include <msp430.h>
#include <stdint.h>
#include <string.h>

// Global Variables
//int j=0;
unsigned int delay_time = 500;
const unsigned int timer_half_sec = 65535;
unsigned int Vrx_global = 0x0000;
unsigned int Vry_global = 0x0000;
unsigned int JPB_counter = 0;
unsigned int PC_ready = 0;
int phy_global = 70; //one step equals to phy_global degrees
unsigned int heading_global = 0;  // what is the heading of the motor, in degrees, where the blue line is referred as 0.
char heading_str[5];
int calib_flag = 0;


//For Joystick Voltage reading

volatile unsigned int adc_results[TOTAL_SAMPLES];   // Create an array to store the ADC results
volatile unsigned char sampling_complete = 0;

//For script states:
static unsigned int flash_address_script_1 = 0x1000;
int upload_scr_1_completed = 0;
static unsigned int flash_address_script_2 = 0x1040;
int upload_scr_2_completed = 0;
static unsigned int flash_address_script_3 = 0x1080;
int upload_scr_3_completed = 0;
static unsigned int PHY_FLASH = 0x10BF;
unsigned int *phy_ptr = (unsigned int *) 0x10BF;


//--------------------------------------------------------------------
//             System Configuration  
//--------------------------------------------------------------------
void sysConfig(void){ 
	GPIOconfig();
	StopAllTimers();
	lcd_init();
	lcd_clear();
	UART_init();
//	erase_segment(0x1000);
//	erase_segment(0x1040);
//	erase_segment(0x1080);
//	update_phy(63);
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
        Vry_temp += adc_results[iterator *4];
        Vrx_temp += adc_results[3 +iterator *4];
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


void voltage2str(char *str, unsigned int Vx, unsigned int Vy, unsigned int JPB) {
    unsigned int values[] = {Vx, Vy, JPB};
    int strIndex = 0;
    int i = 0 ;
    for (i = 0; i < 3; i++) {
        unsigned int value = values[i];
        char buffer[10];
        int bufIndex = 0;

        // Convert integer to string
        do {
            buffer[bufIndex++] = (value % 10) + '0';
            value /= 10;
        } while (value > 0);

        // Reverse and copy to output string
        while (bufIndex > 0) {
            str[strIndex++] = buffer[--bufIndex];
        }

        // Add separator (':' or '\0' for the last value)
        str[strIndex++] = (i < 2) ? ':' : '\0';
    }
}

//----------------------Count Timer Calls---------------------------------
void timer_call_counter(int delay){
    unsigned int i = 0;
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
        counter_clockwise_step(20);
    }
    else if((Vrx_global >= 0x02FF) && (Vrx_global <= 0x03FF)){
        counter_clockwise_step(8);
    }
    //counter clock wise
    else if((Vrx_global >= 0x0100) && (Vrx_global <= 0x0180)){
        clockwise_step(20);
    }
    else if((Vrx_global >= 0x0000) && (Vrx_global <= 0x0100)){
        clockwise_step(8);
    }
}

void counter_clockwise_step(int t){
//    int phy = *phy_ptr;
    phase(Phase_C, t);
    phase(Phase_B, t);
    phase(Phase_A, t);
    phase(Phase_D, t);
//    heading_global -= phy;
    heading_global -= phy_global;
    if(heading_global < 0){
        heading_global += (360*90);
    }
}

void counter_clockwise_half_step(int t){
//    int phy = *phy_ptr;
    phase(Phase_D, t);
    phase(Phase_D+Phase_C, t);
    phase(Phase_C, t);
    phase(Phase_C+Phase_B, t);
    phase(Phase_B, t);
    phase(Phase_B+Phase_A, t);
    phase(Phase_A, t);
    phase(Phase_A+Phase_D, t);
//    heading_global -= (phy/2);
    heading_global -= (phy_global/2);
    if(heading_global < 0){
        heading_global += (360*90);
    }
}

void clockwise_step(unsigned int t){
//    int phy = *phy_ptr;
    phase(Phase_D, t);
    phase(Phase_A, t);
    phase(Phase_B, t);
    phase(Phase_C, t);
//    heading_global += phy;
    heading_global += phy_global;
    if(heading_global >= (360*90)){
        heading_global -= (360*90);
    }
}

void clockwise_half_step(int t){
//    int phy = *phy_ptr;
    phase(Phase_A+Phase_D, t);
    phase(Phase_A, t);
    phase(Phase_B+Phase_A, t);
    phase(Phase_B, t);
    phase(Phase_C+Phase_B, t);
    phase(Phase_C, t);
    phase(Phase_D+Phase_C, t);
    phase(Phase_D, t);
//    heading_global += (phy/2);
    heading_global += (phy_global/2);
    if(heading_global >= (360*90)){
        heading_global -= (360*90);
    }
}

void phase(unsigned int phase, int t){
    StepMotorPortOut |= phase;
    timer_call_counter(t);
    StepMotorPortOut &= ~phase;
}


void stepper_deg(int arg, int scan_flag){
    arg *= 90;
//    int phy = *phy_ptr;
        int diff = heading_global - arg; //X - Y
        while(((diff > phy_global) && (diff > 0)) || ((diff < phy_global) && (diff < 0))){
//        while(((diff > phy) && (diff > 0)) || ((diff < phy) && (diff < 0))){
            if((PC_ready == 1) || scan_flag== 1){
            diff = heading_global - arg;
            if(diff > 0){                           // (X-Y > 0)
                    if(diff <= (180*90)){
                        counter_clockwise_step(8);
                    }
                    else{
                        clockwise_step(8);
                    }
                }
            else if(diff < 0){                      // (X-Y < 0)
                if(diff < -(180*90)){
                    counter_clockwise_step(8);
                }
                else{
                    clockwise_step(8);
                }
            }
            if(scan_flag == 0){
                int2str(heading_str, heading_global);
                uart_puts(heading_str);
                PC_ready = 0;
            }
        }
    }

}

void stepper_scan(int a1, int a2){
    lcd_clear();
    lcd_home();
    char a1_as_str[3];
    char a2_as_str[3];
    int2str(a1_as_str, a1);
    int2str(a2_as_str, a2);
    stepper_deg(a1, 1);
    lcd_puts("Start of scan:");
    lcd_new_line;
    lcd_puts(a1_as_str);
    stepper_deg(a2, 1);
    lcd_clear();
    lcd_home();
    lcd_puts("End of scan:");
    lcd_new_line;
    lcd_puts(a2_as_str);
    timer_call_counter(1000);
    lcd_clear();
    lcd_home();

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
   char num_as_str[4];
   int2str(num_as_str, num);
   uart_puts(num_as_str);
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
        if(state == state5){
            upload_scr_1_completed = 1;
        }
        else if(state == state7){
            upload_scr_2_completed = 1;
        }
        else if(state == state9){
            upload_scr_3_completed = 1;
        }
    }
}

void update_phy(int new_value) {
        unsigned int *flash_ptr;
        flash_ptr = (unsigned int *)PHY_FLASH;

        // Unlock flash memory
        FCTL3 = FWKEY ;
        FCTL1 = FWKEY | WRT;

//        *flash_ptr = 0xFF;
        // Write new value
        *flash_ptr = new_value;
        while (FCTL3 & BUSY);
        // Lock flash memory
        FCTL1 = FWKEY;
        FCTL3 = FWKEY | LOCK;
}

void erase_segment(int address){
    char *flash_ptr;
    unsigned int i;
    flash_ptr = (char *)address;
    FCTL3 = FWKEY;
    FCTL1 = FWKEY | ERASE;
    *flash_ptr = 0;
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
        command = *(flash_ptr + i);
        i++;
        if((command != 0x08) && (command != 0x05)){
            arg1 = *(flash_ptr + i);
            if(arg1==254){
                i++;
                arg1 += *(flash_ptr + i);
            }
            i++;
        }
        if(command == 0x07){
            arg2 = *(flash_ptr + i);
            if(arg2==254){
                i++;
                arg2 += *(flash_ptr + i);
            }
            i++;
        }
        switch(command){
        case 0x01:
            count_up_LCD(arg1);
            break;
        case 0x02:
            count_down_LCD(arg1);
            break;
        case 0x03:
            Rotate_right(arg1);
            break;
        case 0x04: // set_delay
            delay_time = arg1* 10;
            break;
        case 0x05: // clear_lcd
            lcd_clear();
            lcd_home();
            break;
        case 0x06: // servo_de
            uart_puts("x");
            stepper_deg(arg1,0);
            uart_puts("x");
            break;
        case 0x07: // servo_scan
            stepper_scan(arg1, arg2);
            break;
        case 0x08: // sleep
            finish_flag = 1;
            break;
        case 0xFF:
            finish_flag = 1;
            break;
        }
//        i++;
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
        JPB_counter = (JPB_counter + 1) % 3;
//        UCA0TXBUF = 'P';
//        IE2 |= UCA0TXIE;

    }
    //state3- calibrate phy mission
    else if((JoystickPBIntPend & JPBMask) && (state==state3)){
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
    if((state==state8) || (state==state1) || (state==state2) ||(state==state3) ||(state==state4)){    //If states 0-4
        if(UCA0RXBUF == '1'){
            state = state1;
            PC_ready = 0;
            UCA0TXBUF = '1';    //Send 'ack' to PC by TXing '1'
            IE2 |= UCA0TXIE;
        }
        else if(UCA0RXBUF == '2'){
            JPB_counter = 0;
            PC_ready = 0;
            state = state2;
            UCA0TXBUF = '2';
            IE2 |= UCA0TXIE;    //Notify PC side that Joystick voltage is arriving, by sending "2"
        }

        else if(UCA0RXBUF == '3'){
            state = state3;
            calib_flag = 0;
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
        }
        else if(UCA0RXBUF == '6'){
            state = state7;
            flash_address_script_2 = 0x1040;
            erase_segment(flash_address_script_2);
            upload_scr_2_completed = 0;
        }
        else if(UCA0RXBUF == '7'){
            state = state9;
            int tmp = *phy_ptr;
            flash_address_script_3 = 0x1080;
            erase_segment(flash_address_script_3);
            update_phy(tmp);
            upload_scr_3_completed = 0;
        }
        else if(UCA0RXBUF == '8'){
            state = state6;
        }
        else if(UCA0RXBUF == '9'){
            state = state10;
        }
        else if(UCA0RXBUF == 'x'){
            PC_ready = 1;
        }
        else if(UCA0RXBUF == 'A'){
            state = state11;
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
        else if ((state == state7) && (upload_scr_2_completed == 0)){
                write_char_to_flash(UCA0RXBUF, flash_address_script_2);
                flash_address_script_2++;
        }
        else if ((state == state9) && (upload_scr_3_completed == 0)){
                write_char_to_flash(UCA0RXBUF, flash_address_script_3);
                flash_address_script_3++;
        }
        else{   // will use states 6-10 for other scripts
            state = state8;
        }
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


