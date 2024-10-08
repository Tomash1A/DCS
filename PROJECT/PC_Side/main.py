import os
import threading
import customtkinter as ctk
import math
import serial as ser
from collections import deque
import PIL
import tkinter as tk
from tkinter import *
from tkinter import filedialog, IntVar, Label, messagebox
from PIL import Image, ImageTk, ImageDraw
import ctypes
from queue import Queue, Empty
import sys

import time
from utils import *
import threading
from enum import Enum


# MODES:
SCRIPT_MODE = "Script"
PAINT_MODE = "Paint"
JOYSTICK_CONTROL_MODE = "Control step motor"
CALIBRATE_MOTOR_MODE = "Calibrate step"

# CONSTANT STRINGS:
UPLOAD_SCRIPT_1 = "Upload Script 1"
UPLOAD_SCRIPT_2 = "Upload Script 2"
UPLOAD_SCRIPT_3 = "Upload Script 3"
RUN_SCRIPT_1 = "Run script 1"
RUN_SCRIPT_2 = "Run script 2"
RUN_SCRIPT_3 = "Run script 3"

# COMMUNICATION ROLES:
TRANSMITTER_SIDE = 0
RECEIVER_SIDE = 1

# STATES:
NEUTRAL_STATE = '0'
JOYSTICK_CONTROL_STATE = '1'
PAINTER_STATE = '2'
CALIBRATE_MOTOR_STATE = '3'
STATE_4 = '4'
SCRIPT_1_UPLOAD_STATE = '5'
SCRIPT_2_UPLOAD_STATE = '6'
SCRIPT_3_UPLOAD_STATE = '7'
SCRIPT_1_RUN_STATE = '8'
SCRIPT_2_RUN_STATE = '9'
SCRIPT_3_RUN_STATE = 'A'

UPLOAD_STATES = [SCRIPT_1_UPLOAD_STATE, SCRIPT_2_UPLOAD_STATE, SCRIPT_3_UPLOAD_STATE]
RUN_STATES = [SCRIPT_1_RUN_STATE, SCRIPT_2_RUN_STATE, SCRIPT_3_RUN_STATE]

# STATE to TRANSMITTER/RECEIVER MESSAGE:
STATE_TITLES = {
    NEUTRAL_STATE: ("Neutral", ),
    JOYSTICK_CONTROL_STATE: ("Control step motor with joystick", "step motor control ack"),
    PAINTER_STATE: ("Receive Joystick voltage", ),
    CALIBRATE_MOTOR_STATE: ("Calibrate step motor", ),
    STATE_4: ("Script mode", ),
    SCRIPT_1_UPLOAD_STATE: (f"{UPLOAD_SCRIPT_1} to flash", "Script 1 received"),
    SCRIPT_2_UPLOAD_STATE: (f"{UPLOAD_SCRIPT_2} to flash", "Script 2 received"),
    SCRIPT_3_UPLOAD_STATE: (f"{UPLOAD_SCRIPT_3} to flash", "Script 3 received"),
    SCRIPT_1_RUN_STATE: (RUN_SCRIPT_1, "Script 1 finished running on MCU"),
    SCRIPT_2_RUN_STATE: (RUN_SCRIPT_2, "Script 2 finished running on MCU"),
    SCRIPT_3_RUN_STATE: (RUN_SCRIPT_3, "Script 3 finished running on MCU")
}

NUM_SAMPLES_PAINTER = 20


class HeadingVisualizer:
    IS_HEADING_OPEN = False

    def __init__(self, root, connection):
        self.root = root
        # self.main_frame = main_frame
        self.canvas = ctk.CTkCanvas(self.root, width=400, height=400)
        self.canvas.place(relx=0.5, rely=0.5, anchor=tk.CENTER)
        self.heading_text = self.canvas.create_text(200, 200, text="0°", font=("Arial", 36), fill="black")
        self.heading_value = 0
        self.latest_heading = 0

        self.heading_conn = connection
        self.close_conn = False

        self.data_lock = threading.Lock()
        self.serial_thread = None


    def open_new_thread(self):
        if not HeadingVisualizer.IS_HEADING_OPEN:
            HeadingVisualizer.IS_HEADING_OPEN = True
            self.heading_conn.flushInput()
            time.sleep(0.1)  # Short delay to ensure the connection is ready
            self.heading_conn.write(b'x')  # Send initial 'h' to start communication
            time.sleep(0.1)  # Short delay after sending 'h'

            self.serial_thread = threading.Thread(target=self.read_data_continuously)
            self.serial_thread.daemon = True
            self.serial_thread.start()
            self.root.after(10, self.update_heading)

    def read_data_continuously(self):
        while not self.close_conn:
            if self.heading_conn.in_waiting > 0:
                raw_data = self.heading_conn.readline()
                decoded_data = raw_data.decode().strip()
                if decoded_data == 'x':
                    self.close_conn = True
                    self.root.after(10, self.close)
                    return
                else:
                    try:
                        # decoded_data = raw_data.decode().strip()
                        self.latest_heading = (float(decoded_data)/90) %360
                        with self.data_lock:
                            self.heading_value = self.latest_heading
                        self.heading_conn.write(b'x')  # Signal ready for next sample
                    except Exception as e:
                        print(f"Error processing data: {e}")
                        self.heading_conn.write(b'x')  # Signal ready for next sample
                        if self.close_conn:
                            break
                time.sleep(0.01)


    def close(self):
        self.close_conn = True
        if self.serial_thread and self.serial_thread.is_alive():
            self.serial_thread.join(timeout=1.0)
        if self.heading_conn and self.heading_conn.is_open:
            self.heading_conn.close()
        self.root.destroy()
        HeadingVisualizer.IS_HEADING_OPEN = False

    def update_heading(self):
        if self.close_conn:
            self.close()
            return
        with self.data_lock:
            self.canvas.itemconfig(self.heading_text, text=f"{int(self.latest_heading)}°")
            # Draw a heading indicator
            self.canvas.delete("indicator")
            radius = 150
            x = 200 + radius * math.cos(math.radians(self.latest_heading-90))
            y = 200 + radius * math.sin(math.radians(self.latest_heading-90))
            self.canvas.create_line(200, 200, x, y, width=3, fill="black", tags="indicator")

        if not self.close_conn:
            self.root.after(10, self.update_heading)
        else:
            self.root.destroy()
            HeadingVisualizer.IS_HEADING_OPEN = False




class PaintApp:
    IS_PAINTER_OPEN = False

    def __init__(self, root, connection):
        self.root = root
        self.root.title("Joystick Controlled Paint App")

        self.canvas = tk.Canvas(self.root, width=600, height=400, bg="white")
        self.canvas.pack()

        self.pen_color = "black"
        self.pen_size = 2
        self.eraser_size = 10

        self.current_x = 300  # Start at the center of the canvas
        self.current_y = 200
        self.old_x = self.current_x
        self.old_y = self.current_y

        self.vrx_values = deque(maxlen=NUM_SAMPLES_PAINTER)
        self.vry_values = deque(maxlen=NUM_SAMPLES_PAINTER)
        self.vrx_values = deque([1.65 for i in range(NUM_SAMPLES_PAINTER)])
        self.vry_values = deque([1.65 for i in range(NUM_SAMPLES_PAINTER)])
        self.btn_val = 2  # Start in Neutral mode

        self.painter_conn = connection
        self.close_conn = False

        self.data_lock = threading.Lock()
        self.serial_thread = None

        # Cursor to show current position
        self.cursor = self.canvas.create_oval(self.current_x - 5, self.current_y - 5,
                                              self.current_x + 5, self.current_y + 5,
                                              fill="green", outline="green")

    def open_new_thread(self):
        if not PaintApp.IS_PAINTER_OPEN:
            PaintApp.IS_PAINTER_OPEN = True
            self.painter_conn.flushInput()
            time.sleep(0.1)  # Short delay to ensure the connection is ready
            self.painter_conn.write(b'x')  # Send initial 'x' to start communication
            time.sleep(0.1)  # Short delay after sending 'x'

            self.serial_thread = threading.Thread(target=self.read_data_continuously)
            self.serial_thread.daemon = True
            self.serial_thread.start()
            self.root.after(10, self.update_paint)

    def paint(self):
        if self.btn_val == 0:  # Write mode
            self.canvas.create_line(self.old_x, self.old_y, self.current_x, self.current_y,
                                    width=self.pen_size, fill=self.pen_color,
                                    capstyle=tk.ROUND, smooth=tk.TRUE)
        elif self.btn_val == 1:  # Erase mode
            self.canvas.create_oval(self.current_x - self.eraser_size, self.current_y - self.eraser_size,
                                    self.current_x + self.eraser_size, self.current_y + self.eraser_size,
                                    fill="white", outline="white")

    def read_data_continuously(self):
        while not self.close_conn:
            if self.painter_conn.in_waiting > 0:
                raw_data = self.painter_conn.readline()
                try:
                    decoded_data = raw_data.decode().strip()
                    voltages = decoded_data.split(':')
                    if len(voltages) == 3:
                        self.vrx_values.popleft()
                        self.vry_values.popleft()
                        with self.data_lock:
                            self.vrx_values.append(float(int(voltages[0])*(3.3/1024)))
                            self.vry_values.append(float(int(voltages[1])*(3.3/1024)))
                            self.btn_val = int(voltages[2])
                    self.painter_conn.write(b'x')  # Signal ready for next sample
                except Exception as e:
                    print(f"Error processing data: {e}")
                    if self.close_conn:
                        break
            time.sleep(0.01)

    def calculate_movement(self):
        if len(self.vrx_values) < NUM_SAMPLES_PAINTER or len(self.vry_values) < NUM_SAMPLES_PAINTER:
            return 0, 0  # Not enough samples yet

        vrx_mean = sum(self.vrx_values) / len(self.vrx_values)
        vry_mean = sum(self.vry_values) / len(self.vry_values)
        print(vrx_mean,vry_mean)

        dx = (vrx_mean - 1.715) * 1  # Assuming 1.65V is the center position
        dy = (vry_mean - 1.55) * 1  # Adjust multiplier for sensitivity

        return dx, dy

    def update_paint(self):
        if self.close_conn:
            return
        with self.data_lock:
            dx, dy = self.calculate_movement()

            # Update current position
            self.old_x, self.old_y = self.current_x, self.current_y
            self.current_x = max(0, min(600, self.current_x - dx))
            self.current_y = max(0, min(400, self.current_y - dy))  # Invert Y-axis

            if self.btn_val == 0:  # Write mode
                self.paint()
                cursor_color = "black"
            elif self.btn_val == 1:  # Erase mode
                self.paint()
                cursor_color = "red"
            else:  # Neutral mode
                cursor_color = "green"

            # Update cursor position and color
            self.canvas.delete(self.cursor)
            if self.btn_val == 1:  # Erase mode
                self.cursor = self.canvas.create_oval(self.current_x - self.eraser_size,
                                                      self.current_y - self.eraser_size,
                                                      self.current_x + self.eraser_size,
                                                      self.current_y + self.eraser_size,
                                                      outline=cursor_color)
            else:
                self.cursor = self.canvas.create_oval(self.current_x - 5, self.current_y - 5,
                                                      self.current_x + 5, self.current_y + 5,
                                                      fill=cursor_color, outline=cursor_color)

        if not self.close_conn:
            self.root.after(10, self.update_paint)
        else:
            self.painter_conn.close()
            PaintApp.IS_PAINTER_OPEN = False

    def close(self):
        self.close_conn = True
        if self.serial_thread and self.serial_thread.is_alive():
            self.serial_thread.join(timeout=1.0)
        if self.painter_conn and self.painter_conn.is_open:
            self.painter_conn.close()
        PaintApp.IS_PAINTER_OPEN = False


def print_state_msg(state, side):
    print(f"State {state} - {STATE_TITLES[state][side]}")


def get_screen_size():
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


def on_sidebar_select(window_title, main_frame):
    clear_frame(main_frame)
    if window_title == SCRIPT_MODE:
        script_mode(main_frame)
    elif window_title == JOYSTICK_CONTROL_MODE:
        pc_side(JOYSTICK_CONTROL_STATE)
    elif window_title == PAINT_MODE:
        pc_side(PAINTER_STATE)
    elif window_title == CALIBRATE_MOTOR_MODE:
        pc_side(CALIBRATE_MOTOR_STATE)

def upload_script(window_title):
    input_file = filedialog.askopenfilename(
        title="Select a text file",
        filetypes=(("Text files", ".txt"), ("All files", ".*"))
    )

    if not input_file:  # User cancelled input file selection
        return

    # Ask user to choose the output directory
    output_dir = filedialog.askdirectory(title="Select output directory")

    if not output_dir:  # User cancelled output directory selection
        return

    if window_title == UPLOAD_SCRIPT_1:
        output_file = "script1.txt"
        choice = SCRIPT_1_UPLOAD_STATE
    elif window_title == UPLOAD_SCRIPT_2:
        output_file = "script2.txt"
        choice = SCRIPT_2_UPLOAD_STATE
    else:  # window_title == UPLOAD_SCRIPT_3
        output_file = "script3.txt"
        choice = SCRIPT_3_UPLOAD_STATE

    # Combine the selected directory with the output filename
    output_path = os.path.join(output_dir, output_file)

    translate_file(input_file, output_path)
    pc_side(choice,input_file)

def run_script(window_title):   # todo: can make as lambda
    if window_title == RUN_SCRIPT_1:
        pc_side(SCRIPT_1_RUN_STATE)
    elif window_title == RUN_SCRIPT_2:
        pc_side(SCRIPT_2_RUN_STATE)
    elif window_title == RUN_SCRIPT_3:
        pc_side(SCRIPT_3_RUN_STATE)


def enqueue_output(out, queue):
    for line in iter(out.readline, ''):
        queue.put(line)
    out.close()


def update_output_textbox(root, output_queue):
    try:
        line = output_queue.get_nowait()
        output_textbox.insert("end", line)
        output_textbox.yview_moveto(1)
    except Empty:
        pass
    finally:
        root.after(100, update_output_textbox, [root, output_queue])


def script_mode(main_frame):
    # todo: can remove globals or send to functions as non global
    global project_name_entry, input_size_entry, epochs_entry, batch_size_entry, class_names_text, progress_bar, model_size_var, output_textbox, start_train_button
    main_frame.pack_forget()
    main_frame.pack(fill = "both", expand = True)
    SCRIPT_LOOP_LOOP = [[UPLOAD_SCRIPT_1,upload_script, 0.3, 0.2,'dodgerblue'],
                        [UPLOAD_SCRIPT_2,upload_script, 0.3, 0.3,'dodgerblue'],
                        [UPLOAD_SCRIPT_3,upload_script, 0.3, 0.4,'dodgerblue'],
                        [RUN_SCRIPT_1,run_script,0.7, 0.2,"chocolate1"],
                        [RUN_SCRIPT_2,run_script,0.7, 0.3,"chocolate1"],
                        [RUN_SCRIPT_3,run_script,0.7, 0.4,"chocolate1"]]


    for txt1, func1, relx, rely, clr in SCRIPT_LOOP_LOOP[0:3]:
        script_up_button = ctk.CTkButton(master = main_frame,text=txt1, command=lambda txt= txt1: upload_script(txt),fg_color = clr, border_color = 'black',
                                             border_width = 2, font = ("Times New Roman", 24), text_color = 'white')
        script_up_button.place(relx = relx, rely = rely, relwidth = 0.3, relheight = 0.08, anchor = ctk.CENTER)

    for txt1, func1, relx, rely, clr in SCRIPT_LOOP_LOOP[3:]:
        script_run_button = ctk.CTkButton(master = main_frame,text=txt1, command=lambda txt= txt1: run_script(txt),fg_color = clr, border_color = 'black',
                                             border_width = 2, font = ("Times New Roman", 24), text_color = 'white')
        script_run_button.place(relx = relx, rely = rely, relwidth = 0.3, relheight = 0.08, anchor = ctk.CENTER)



def change_appearance_mode(new_appearance_mode):
    ctk.set_appearance_mode(new_appearance_mode)


def transmitter(serial_conn, enableTX, enableRX, state, response, input_file):
    while serial_conn.out_waiting > 0 or enableTX:  # while the output buffer isn't empty
        if response == 'F':  # msp430 finished receiving /response is given as 'F' from read_dataization

            bytesChar = bytes(state, 'ascii')
            serial_conn.write(bytesChar)  # Send user choice to the MSP430
            if state == PAINTER_STATE:
                response = '2'
            elif state in {SCRIPT_1_UPLOAD_STATE, SCRIPT_2_UPLOAD_STATE, SCRIPT_3_UPLOAD_STATE}:
                time.sleep(0.25)  # delay for accurate read/write operations on both ends
                script_test = translate_script_to_machine_code(input_file)
                serial_conn.write(script_test)  # Send user choice to the MSP430
            elif state not in STATE_TITLES.keys():
                response = 'F'
                state = NEUTRAL_STATE

            print_state_msg(state, TRANSMITTER_SIDE)
            enableTX = False
            enableRX = True
            time.sleep(0.25)  # delay for accurate read/write operations on both ends

        if serial_conn.out_waiting == 0:
            enableTX = False
            enableRX = True

    return enableTX, enableRX, state, response


def receiver(serial_conn, enableRX, state):
    heading_visualizer = None

    while serial_conn.in_waiting > 0 or enableRX:  # while the input buffer isn't empty
        if state == NEUTRAL_STATE:
            print("DEBUG: In neutral state - infinite loop - might remove this 'if' or replace 'continue'")    # todo: remove
            continue

        if state == PAINTER_STATE:
            root = tk.Tk()
            paint_app = PaintApp(root, serial_conn)
            paint_app.open_new_thread()

            def on_closing():
                paint_app.close()
                root.destroy()

            root.protocol("WM_DELETE_WINDOW", on_closing)
            try:
                root.mainloop()
            except Exception as e:
                print(f"Error in main loop: {e}")
            finally:
                paint_app.close()

        elif state == CALIBRATE_MOTOR_STATE and serial_conn.in_waiting > 0:
            numsteps = serial_conn.readline().decode().strip()
            Phy = 360 / int(numsteps)
            print('Number of steps = ', numsteps)
            print('Phy =', Phy)
            break
        elif state == STATE_4:
            break

        elif state in RUN_STATES and serial_conn.in_waiting > 0:
            receiver_buffer = serial_conn.readline().decode().strip()
            print(receiver_buffer)

            if receiver_buffer == 'x' and not heading_visualizer:
                root = tk.Tk()
                heading_visualizer = HeadingVisualizer(root, serial_conn)
                heading_visualizer.open_new_thread()

                def on_closing():
                    heading_visualizer.close()
                    root.destroy()

                root.protocol("WM_DELETE_WINDOW", on_closing)
                try:
                    root.mainloop()
                except Exception as e:
                    print(f"Error in main loop: {e}")
                finally:
                    heading_visualizer.close()

            elif receiver_buffer == 'x' and heading_visualizer:
                heading_visualizer.close()
                root.destroy()
                heading_visualizer = None
                root = None


            elif receiver_buffer != 'x' and not heading_visualizer:
                print_state_msg(receiver_buffer, RECEIVER_SIDE)
                break

        elif state == JOYSTICK_CONTROL_STATE and serial_conn.in_waiting > 0:
            receiver_buffer = serial_conn.readline().decode().strip()
            print_state_msg(receiver_buffer, RECEIVER_SIDE)
            break

        elif state in UPLOAD_STATES:
            receiver_buffer = serial_conn.readline().decode().strip()
            print_state_msg(receiver_buffer, RECEIVER_SIDE)
            break

        elif state not in STATE_TITLES.keys() or serial_conn.in_waiting <= 0:
            enableRX = True
            time.sleep(0.25)  # delay for accurate read/write operations on both ends


    return enableRX

def pc_side(state, input_file=None):
    serial_conn = ser.Serial('COM3', baudrate=9600, bytesize = ser.EIGHTBITS,
                             parity = ser.PARITY_NONE, stopbits = ser.STOPBITS_ONE,
                             timeout = 1)  # timeout of 1 sec so that the read and write operations are blocking,
    # after the timeout the program continues
    enableTX = True
    enableRX = False

    # clear buffers
    serial_conn.reset_input_buffer()
    serial_conn.reset_output_buffer()
    response = 'F'

    Vx_Vy_flag = 0
    while True:
        # TX
        enableTX, enableRX, state, response = transmitter(serial_conn, enableTX, enableRX, state, response, input_file)
        # RX
        enableRX = receiver(serial_conn, enableRX, state)
        break
    # Close the serial connection
    serial_conn.close()


def main():
    screen_width, screen_height = get_screen_size()
    ctk.set_appearance_mode("dark")
    ctk.set_default_color_theme("green")

    root = ctk.CTk()
    root.title('DCS project')
    root.geometry(f"{screen_width}x{screen_height}")

    sidebar = ctk.CTkFrame(master = root, width = 380, corner_radius = 0)
    sidebar.pack(side = "left", fill = "y")

    main_frame = ctk.CTkFrame(master = root)
    main_frame.pack(fill = "both", expand = True, padx = 10, pady = 10)
    MAIN_FRAME_LIST = [["             Script            ", on_sidebar_select, SCRIPT_MODE, main_frame],
                       ["              Paint             ", on_sidebar_select, PAINT_MODE, main_frame],
                       ["     Calibrate step     ", on_sidebar_select, CALIBRATE_MOTOR_MODE, main_frame],
                       ["Control step motor", on_sidebar_select, JOYSTICK_CONTROL_MODE, main_frame]]
    # todo: make as function and call 4 times

    for txt1, func, mode, frame in MAIN_FRAME_LIST:
        main_frame_button = ctk.CTkButton(master=sidebar, text=txt1,
                                          command=lambda mode1=mode: func(mode1, frame),
                                          fg_color="dodgerblue",
                                          text_color="white", border_color='black', border_width=2,
                                          font=("Algerian", 20))
        main_frame_button.pack(pady=20)

    empty_space = ctk.CTkLabel(master = sidebar, text = "")
    empty_space.pack(fill = tk.BOTH, expand = True)

    appearance_mode_var = ctk.StringVar(value = "Light")
    appearance_mode_label = ctk.CTkLabel(master = sidebar, text = "Appearance Mode", font = ("Algerian", 12))
    appearance_mode_label.pack(padx = 10, pady = (0, 5), anchor = 'w')

    for txt in ["Light", "Dark"]:
        light_mode_radio = ctk.CTkRadioButton(master=sidebar, text=txt, variable=appearance_mode_var, value=txt,
                                              command=lambda txt_in=txt: change_appearance_mode(txt_in))
        light_mode_radio.pack(padx=10, pady=(0, 5), anchor='w')

    output_queue = Queue()

    root.after(100, update_output_textbox,[root, output_queue])
    root.mainloop()


if __name__ == "__main__":
    main()
