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
        script_1 = r"C:\Users\user1\University_ 4th year\DCS\PROJECT\PC_Side\script1.txt"
        # script_2 = "C:\DCS_project_script_files\scripts\script2.txt"
        # script_3 = "C:\DCS_project_script_files\scripts\script3.txt"

        # Example usage
        input_file = r'C:\Users\user1\University_ 4th year\DCS\PROJECT\PC_Side\script_1_code.txt'
        output_file = r"C:\Users\user1\University_ 4th year\DCS\PROJECT\PC_Side\script_1_out.txt"
        # translate_file(input_file, output_file)

        while True:
            # RX
            while (s.in_waiting > 0):  # while the input buffer isn't empty

                if state == 0:
                    continue

                elif state == 1:
                    break

                elif state == 2:
                    voltages = s.readline().decode().strip()
                    if voltages == 'P':
                        print('JPB was pushed!')
                    else:
                        if Vx_Vy_flag == 0:
                            Vrx_global = voltages
                            Vx_Vy_flag = 1
                            print('Vrx = ', Vrx_global)
                        elif Vx_Vy_flag == 1:
                            Vry_global = voltages
                            Vx_Vy_flag = 0
                            print('Vry = ', Vry_global, '\n')

                elif state == 3:
                    break

                elif state == 4:
                    break

                elif state == 5:        #Upload Script 1 completed
                    A = s.readline().decode().strip()
                    if (A == '5'):
                        print('Script 1 received')


                if (s.in_waiting == 0):
                    enableTX = True

            # TX
            while (s.out_waiting > 0 or enableTX):  # while the output buffer isn't empty

                if (response == 'F'):  # msp430 finished receiving /response is given as 'F' from initialization
                    inChar = input("Enter a digit (1-8) to select a task: ")  # Receive input from user
                    bytesChar = bytes(inChar, 'ascii')
                    if inChar == '0':
                        s.write(bytesChar)  # Send user choice to the MSP430
                        state = 0
                        print("State 0- Neutral")
                    elif inChar == '1':
                        s.write(bytesChar)  # Send user choice to the MSP430
                        state = 1
                        print("State 1- Control step motor with joystick")
                    elif inChar == '2':
                        s.write(bytesChar)  # Send user choice to the MSP430
                        state = 2
                        response = '2'
                        print("State 2- Receive Joystick voltage")
                    elif inChar == '3':
                        s.write(bytesChar)  # Send user choice to the MSP430
                        state = 3
                        print("State 3- Calibrate step motor")
                    elif inChar == '4':
                        s.write(bytesChar)  # Send user choice to the MSP430
                        state = 4
                        print("State 4- Script mode")
                    elif inChar == '5':
                        state = 5
                        s.write(bytesChar)  # Send user choice to the MSP430
                        time.sleep(0.25)  # delay for accurate read/write operations on both ends
                        script_test = translate_script_to_machine_code(input_file)
                        s.write(script_test)  # Send user choice to the MSP430
                        print(script_test)
                        print("State 5- Upload Script 1 to flash")

                    elif inChar == '8':
                        state = 8
                        s.write(bytesChar)  # Send user choice to the MSP430
                        time.sleep(0.25)  # delay for accurate read/write operations on both ends
                        print("State 8- Execute Script 1 from flash memory")
                    else:
                        s.write(bytesChar)  # Send user choice to the MSP430
                        response = 'F'
                        state = 0

                    # s.write(bytesChar)  # Send user choice to the MSP430
                    enableTX = False
                    time.sleep(0.25)  # delay for accurate read/write operations on both ends

                if s.out_waiting == 0:
                    enableTX = False

        # Close the serial connection
        s.close()


if __name__ == '__main__':
    main()