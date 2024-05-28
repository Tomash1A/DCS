#ifndef _halGPIO_H_
#define _halGPIO_H_

#include  "bsp.h"    		// private library - BSP layer
#include  "app.h"    		// private library - APP layer


extern enum FSMstate state;   // global variable
extern enum SYSmode lpm_mode; // global variable

extern void sysConfig(void);
extern void print2LEDs(unsigned char);
extern void clrLEDs(void);
extern void toggleLEDs(char);
extern void setLEDs(char);
extern unsigned char readSWs(void);
extern void delay(unsigned int);

extern void depressInterrupt(unsigned char);

extern void enterLPM(unsigned char);
extern void incLEDs(char);

extern void enable_interrupts();
extern void disable_interrupts();

extern void GeneratePWM(unsigned int Ton, unsigned int Toff);

extern __interrupt void PBs_handler(void);

#endif







