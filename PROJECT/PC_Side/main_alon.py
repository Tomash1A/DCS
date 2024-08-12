
import os

import PIL
import customtkinter as ctk
import tkinter as tk
from tkinter import *
from tkinter import filedialog, IntVar, Label, messagebox
from PIL import Image, ImageTk, ImageDraw
import ctypes
from queue import Queue, Empty
import sys
import serial as ser
import time
from utils import *

project_name = ""
input_file = ""
output_file = ""

def pc_side(inChar, input_file):
    s = ser.Serial('COM3', baudrate=9600, bytesize=ser.EIGHTBITS,
                   parity=ser.PARITY_NONE, stopbits=ser.STOPBITS_ONE,
                   timeout=1)  # timeout of 1 sec so that the read and write operations are blocking,
    # after the timeout the program continues
    enableTX = True

    # clear buffers
    s.reset_input_buffer()
    s.reset_output_buffer()
    # print_menu()
    response = 'F'
    phy_step = 0
    Vrx_global, Vry_global = 0, 0
    state = 0
    Vx_Vy_flag = 0


    while True:

        # TX
        while (s.out_waiting > 0 or enableTX):  # while the output buffer isn't empty

            if (response == 'F'):  # msp430 finished receiving /response is given as 'F' from initialization

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
                    time.sleep(0.25)#elay for accurate read/write operations on both ends
                    script_test = translate_script_to_machine_code(input_file)
                    s.write(script_test)  # Send user choice to the MSP430
                    print("State 5- Upload Script 1 to flash")
                    #    enableTX = False   #another optional ways of breaking the loop
                elif inChar == '6':
                    state = 6
                    s.write(bytesChar)  # Send user choice to the MSP430
                    time.sleep(0.25)#elay for accurate read/write operations on both ends
                    script_test = translate_script_to_machine_code(input_file)
                    s.write(script_test)  # Send user choice to the MSP430
                    print("State 6- Upload Script 2 to flash")

                elif inChar == '7':
                    state = 7
                    s.write(bytesChar)  # Send user choice to the MSP430
                    time.sleep(0.25)#elay for accurate read/write operations on both ends
                    script_test = translate_script_to_machine_code(input_file)
                    s.write(script_test)  # Send user choice to the MSP430
                    print("State 7- Upload Script 3 to flash")

                elif inChar == '8':
                    s.write(bytesChar)  # Send user choice to the MSP430
                    state = 8
                    print("State 8- run Script 1")

                elif inChar == '9':
                    s.write(bytesChar)  # Send user choice to the MSP430
                    state = 9
                    print("State 9- run Script 2")

                elif inChar == '10':
                    s.write(bytesChar)  # Send user choice to the MSP430
                    state = 10
                    print("State 10- run Script 3")



                else:
                    s.write(bytesChar)  # Send user choice to the MSP430
                    response = 'F'
                    state = 0

                # s.write(bytesChar)  # Send user choice to the MSP430
                enableTX = False
                time.sleep(0.25)  # delay for accurate read/write operations on both ends

            if s.out_waiting == 0:
                enableTX = False

         # RX
        while (s.in_waiting > 0):  # while the input buffer isn't empty

            if state == 0:
                continue

            elif state == 1:
                A = s.readline().decode().strip()
                if (A == '1'):
                    print("step motor control ack")
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
                numsteps = s.readline().decode().strip()
                print("Number of steps = ", numsteps)
                state = 0
                break

            elif state == 4:
                break

            elif state == 5:  # Upload Script 1
                A = s.readline().decode().strip()
                if (A == '5'):
                    print('Script 1 received')
                    break
            elif state == 6:  # Upload Script 2
                A = s.readline().decode().strip()
                if (A == '6'):
                    print('Script 2 received')
                    break
            elif state == 7:  # Upload Script 3
                A = s.readline().decode().strip()
                if (A == '7'):
                    print('Script 3 received')
                    break
            elif state == 8:  # Upload Script 3
                A = s.readline().decode().strip()
                if (A == '8'):
                    print('Script 1 finished running on MCU')
                    break
            if (s.in_waiting == 0):
                enableTX = True
        if (state != 3 ):
            break
    # Close the serial connection
    s.close()

def get_screen_size(): #גודל החלונית הראשי (נקבע לפי המחשב של המשתמש)
    user32 = ctypes.windll.user32
    screen_width = user32.GetSystemMetrics(0)
    screen_height = user32.GetSystemMetrics(1)
    return screen_width, screen_height

def clear_frame(frame):
    for widget in frame.winfo_children():
        widget.destroy()

def read_output(process, queue):
    for line in iter(process.stdout.readline, b''):
        queue.put(line.decode('utf-8'))
    process.stdout.close()

def on_sidebar_select(window_title):
    clear_frame(main_frame)
    if window_title == "Script":
        script_mode()
    elif window_title == "Paint":
        painter()
    elif window_title == "Calibrate step":
        pc_side('3',input_file)
    elif window_title == "Control step motor":
        pc_side('1',input_file)


def upload_script(window_title):
    global input_file, output_file
    input_file = filedialog.askopenfilename(
        title="Select a text file",
        filetypes=(("Text files", "*.txt"), ("All files", "*.*"))
    )
    if window_title == "Upload script 1":
        output_file = r'C:\Users\user1\University_ 4th year\DCS\PROJECT\PC_Side\script_1_out.txt'
        translate_file(input_file, output_file)
        pc_side('5', input_file)
    elif window_title == "Upload script 2":
        output_file = "C:\DCS_project_script_files\scripts\script2.txt"
        translate_file(input_file, output_file)
        pc_side('6', input_file)
    elif window_title == "Upload script 3":
        output_file = "C:\DCS_project_script_files\scripts\script3.txt"
        translate_file(input_file, output_file)
        pc_side('7', input_file)

def run_script(window_title):
    input_file = "C:\DCS_project_script_files\scripts\script1.txt"
    if window_title == "Run script 1":
        pc_side('8', input_file)
    elif window_title == "Run script 2":
        print("run_script2")
        pc_side('9', input_file)
    elif window_title == "Run script 3":
        print("run_script3")
        pc_side('10', input_file)


output_queue = Queue()

def enqueue_output(out, queue):
    for line in iter(out.readline, ''):
        queue.put(line)
    out.close()

def update_output_textbox():
    try:
        line = output_queue.get_nowait()
        output_textbox.insert("end", line)
        output_textbox.yview_moveto(1)
    except Empty:
        pass
    finally:
        root.after(100, update_output_textbox)


def script_mode():
    global project_name_entry, input_size_entry, epochs_entry, batch_size_entry, class_names_text, progress_bar, model_size_var, output_textbox, start_train_button
    main_frame.pack_forget()
    main_frame.pack(fill="both", expand=True)

    # script1
    ctk.CTkLabel(master=main_frame,  font=("Algerian", 18)).place(relx=0.2, rely=0.10, anchor=ctk.CENTER)
    upload_script_1_button = ctk.CTkButton(master=main_frame, text="Upload script 1", command=lambda: upload_script("Upload script 1"), border_color='black', border_width=2, font=("Algerian", 24), text_color='white')
    upload_script_1_button.place(relx=0.3, rely=0.2, relwidth=0.3, relheight=0.08, anchor=ctk.CENTER)

    # script2
    ctk.CTkLabel(master=main_frame,  font=("Algerian", 18)).place(relx=0.2, rely=0.10, anchor=ctk.CENTER)
    upload_script_2_button = ctk.CTkButton(master=main_frame, text="Upload script 2", command=lambda: upload_script("Upload script 2"), border_color='black', border_width=2, font=("Algerian", 24), text_color='white')
    upload_script_2_button.place(relx=0.3, rely=0.3, relwidth=0.3, relheight=0.08, anchor=ctk.CENTER)

    # script3
    ctk.CTkLabel(master=main_frame,  font=("Algerian", 18)).place(relx=0.2, rely=0.10, anchor=ctk.CENTER)
    upload_script_3_button = ctk.CTkButton(master=main_frame, text="Upload script 3", command=lambda: upload_script("Upload script 3"), border_color='black', border_width=2, font=("Algerian", 24), text_color='white')
    upload_script_3_button.place(relx=0.3, rely=0.4,relwidth=0.3, relheight=0.08, anchor=ctk.CENTER)


    # run script 1
    start_train_button = ctk.CTkButton(master=main_frame, text="Run script 1", command= lambda:run_script("Run script 1"), fg_color="chocolate1",border_color='black', border_width=3, font=("Algerian", 24, "bold"), text_color='white')
    start_train_button.place(relx=0.7, rely=0.2, relwidth=0.3, relheight=0.08, anchor=ctk.CENTER)

    # run script 2
    start_train_button = ctk.CTkButton(master=main_frame, text="Run script 2", command= lambda:run_script("Run script 2"), fg_color="chocolate1",border_color='black', border_width=3, font=("Algerian", 24, "bold"), text_color='white')
    start_train_button.place(relx=0.7, rely=0.3, relwidth=0.3, relheight=0.08, anchor=ctk.CENTER)

    # run script 3
    start_train_button = ctk.CTkButton(master=main_frame, text="Run script 3", command= lambda:run_script("Run script 3"), fg_color="chocolate1",border_color='black', border_width=3, font=("Algerian", 24, "bold"), text_color='white')
    start_train_button.place(relx=0.7, rely=0.4, relwidth=0.3, relheight=0.08, anchor=ctk.CENTER)

def painter():
    class PaintApp:
        def __init__(self, root):
            self.root = root
            self.root.title("Joystick Controlled Paint App")

            # Initialize the state: 0 - pen, 1 - eraser, 2 - pointer
            self.state = 0

            # Set up the canvas
            self.canvas = tk.Canvas(self.root, bg='white', width=600, height=400)
            self.canvas.pack()

            # Bind mouse events
            self.canvas.bind("<Button-1>", self.change_state)
            self.canvas.bind("<Motion>", self.paint)

            # Default drawing settings
            self.pen_color = "black"
            self.eraser_color = "white"
            self.line_width = 8

        def change_state(self, event):
            # Change state on mouse click
            self.state = (self.state + 1) % 3

        def paint(self, event):
            # Draw only in the pen or eraser states
            if self.state == 0:  # Pen
                color = self.pen_color
                self.canvas.create_line(event.x, event.y, event.x + 1, event.y + 1, fill=color, width=self.line_width)
            elif self.state == 1:  # Eraser
                color = self.eraser_color
                self.canvas.create_line(event.x, event.y, event.x + 1, event.y + 1, fill=color, width=self.line_width)

            # State 2 is the pointer, so no drawing should occur

    if __name__ == "__main__":
        root = tk.Tk()
        app = PaintApp(root)
        root.mainloop()

def command_to_hex(command):
    # Dictionary mapping instructions to their OPC codes
    instruction_map = {
        "inc_lcd": "01",
        "dec_lcd": "02",
        "rra_lcd": "03",
        "set_delay": "04",
        "clear_lcd": "05",
        "stepper_deg": "06",
        "stepper_scan": "07",
        "sleep": "08"
    }

    # Split the command into instruction and operands
    parts = command.split()
    instruction = parts[0]

    # Get the OPC code for the instruction
    opc = instruction_map[instruction]

    # Convert operands to HEX and combine with OPC
    if len(parts) > 1:
        operands = parts[1].split(',')
        operands_hex = ''.join(f"{int(op):02X}" for op in operands)
        hex_code = opc + operands_hex
    else:
        hex_code = opc

    return hex_code

def translate_file(input_file, output_file):
    with open(input_file, 'r') as infile:
        commands = infile.readlines()

    hex_codes = [command_to_hex(cmd.strip()) for cmd in commands]

    with open(output_file, 'w') as outfile:
        for hex_code in hex_codes:
            outfile.write(hex_code + '\n')


def change_appearance_mode(new_appearance_mode):
    ctk.set_appearance_mode(new_appearance_mode)

screen_width, screen_height = get_screen_size()
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("green")

root = ctk.CTk()
root.title('DCS project')
root.geometry(f"{screen_width}x{screen_height}")

model_size_var = IntVar(value=1)

sidebar = ctk.CTkFrame(master=root, width=380, corner_radius=0)
sidebar.pack(side="left", fill="y")

main_frame = ctk.CTkFrame(master=root)
main_frame.pack(fill="both", expand=True, padx=10, pady=10)

script_mode_button = ctk.CTkButton(master=sidebar, text="             Script            ", command=lambda: on_sidebar_select("Script"), fg_color="dodgerblue", text_color="white", border_color='black', border_width=2, font=("Algerian", 20))
script_mode_button.pack(pady=20)

paint_mode_button = ctk.CTkButton(master=sidebar, text="              Paint             ", command=lambda: on_sidebar_select("Paint"), fg_color="dodgerblue", text_color="white", border_color='black', border_width=2, font=("Algerian", 20))
paint_mode_button.pack(pady=20)

callibrate_step_motor_button = ctk.CTkButton(master=sidebar, text="     Calibrate step     ", command=lambda: on_sidebar_select("Calibrate step"), fg_color="dodgerblue", text_color="white", border_color='black', border_width=2, font=("Algerian", 20))
callibrate_step_motor_button.pack(pady=20)

control_step_motor_button = ctk.CTkButton(master=sidebar, text="Control step motor", command=lambda: on_sidebar_select("Control step motor"), fg_color="dodgerblue", text_color="white", border_color='black', border_width=2, font=("Algerian", 20))
control_step_motor_button.pack(pady=20)

empty_space = ctk.CTkLabel(master=sidebar, text="")
empty_space.pack(fill=tk.BOTH, expand=True)

appearance_mode_var = ctk.StringVar(value="Light")
appearance_mode_label = ctk.CTkLabel(master=sidebar, text="Appearance Mode", font=("Algerian", 12))
appearance_mode_label.pack(padx=10, pady=(0, 5), anchor='w')

light_mode_radio = ctk.CTkRadioButton(master=sidebar, text="Light", variable=appearance_mode_var, value="Light", command=lambda: change_appearance_mode("Light"))
light_mode_radio.pack(padx=10, pady=(0, 5), anchor='w')

dark_mode_radio = ctk.CTkRadioButton(master=sidebar, text="Dark", variable=appearance_mode_var, value="Dark", command=lambda: change_appearance_mode("Dark"))
dark_mode_radio.pack(padx=10, pady=(0, 10), anchor='w')


if __name__ == "__main__":

    root.after(100, update_output_textbox)
    root.mainloop()
