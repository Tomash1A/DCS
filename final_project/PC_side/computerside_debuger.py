import sys
import serial as ser
import time
from utils import *




def main():
    if __name__ == "__main__":
        s = ser.Serial('COM3', baudrate=9600, bytesize=ser.EIGHTBITS,
                       parity=ser.PARITY_NONE, stopbits=ser.STOPBITS_ONE,
                       timeout=1)  # timeout of 1 sec so that the read and write operations are blocking,
        # after the timeout the program continues
        enableTX = True

        # clear buffers
        s.reset_input_buffer()
        s.reset_output_buffer()
        print_menu()
        response = 'F'
        phy_step = 0
        Vrx_global, Vry_global = 0, 0
        state = 0
        Vx_Vy_flag = 0

        while True:
            # RX
            while (s.in_waiting > 0):  # while the input buffer isn't empty

                if state ==0:
                    response = s.readline().decode().strip()
                    if response == '1':
                        state = 1
                    elif response == '2':
                        state= 2
                        print("State 2- Receive Joystick voltage")
                    elif response == '3':
                        state= 3
                        print("State 3- Calibrate step motor")
                    elif response == '4':
                        state= 4
                        print("State 4- Script mode")
                    elif (response == 'FF' or response == 'FFF'):
                        print("State 0- Neutral")
                        response = 'F'
                        state = 0
                    else:
                        response = 'F'
                        state = 0
                    print("Response:" + response, 'state = ', state)

                elif state ==1:
                    break

                elif state == 2:
                    voltages = s.readline().decode().strip()
                    if Vx_Vy_flag == 0:
                        Vrx_global = voltages
                        Vx_Vy_flag = 1
                        print('Vrx = ', Vrx_global)
                    elif Vx_Vy_flag == 1:
                        Vry_global = voltages
                        Vx_Vy_flag = 0
                        print('Vry = ',Vry_global,'\n')

                elif state ==3:
                    break

                elif state ==4:
                    break

                if (s.in_waiting == 0):
                    enableTX = True

            # TX
            while (s.out_waiting > 0 or enableTX):  # while the output buffer isn't empty


                if (response == 'F'):  # msp430 finished receiving /response is given as 'F' from initialization
                    inChar = input("Enter a digit (1-8) to select a task: ")  # Receive input from user
                    bytesChar = bytes(inChar, 'ascii')
                    print(state)
                    s.write(bytesChar)  # Send user choice to the MSP430
                    enableTX = False
                    time.sleep(0.25)  # delay for accurate read/write operations on both ends

                if s.out_waiting == 0:
                    enableTX = False

        # Close the serial connection
        s.close()


if __name__ == '__main__':
    main()
