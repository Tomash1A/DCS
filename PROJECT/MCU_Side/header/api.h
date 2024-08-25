#ifndef _api_H_
#define _api_H_

#include  "../header/halGPIO.h"     // private library - HAL layer


extern void StepMotor_by_Joystick();
extern void StepMotor_phy_calibration();
extern void count_up_LCD(int arg);
extern void count_down_LCD(int arg);
extern void Rotate_right(int arg);
extern void send_JS_data_to_comp();
extern void upload_script(int upload_script_completed, char tx_char);
#endif







