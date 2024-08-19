import PIL
import customtkinter as ctk
from tkinter import *
from tkinter import filedialog, IntVar, Label, messagebox
from PIL import Image, ImageTk, ImageDraw
import ctypes
from queue import Queue, Empty
import sys
import serial as ser
import time


def txt_to_bytes_vector(file_path):
    # Open and read the file
    with open(file_path, 'r') as file:
        content = file.read()

    # Convert the content to bytes
    bytes_vector = content.encode()

    return bytes_vector


def translate_script_to_machine_code(path):
    # Define the mapping of functions to machine code locally inside the function
    instruction_map = {     # todo: magic number
        "inc_lcd": 0x01,
        "dec_lcd": 0x02,
        "rra_lcd": 0x03,
        "set_delay": 0x04,
        "clear_lcd": 0x05,
        "stepper_deg": 0x06,
        "stepper_scan": 0x07,
        "sleep": 0x08,
        "EOF": 0xFF
    }
    with open(path, 'r') as file:
        script = file.read()
    # Split the script into lines and process each line
    lines = script.strip().split('\n')
    machine_code = []
    for line in lines:
        parts = line.split()
        if parts:
            function_name = parts[0]
            if function_name not in instruction_map:
                print(f"Unknown function: {function_name}")
                continue

            # Append the function code to the machine code list
            machine_code.append(instruction_map[function_name])

            # Append the arguments (if any) to the machine code list
            if len(parts) > 1:
                # For the 'servo_scan' function, we need to handle two arguments separated by a comma
                if function_name == 'stepper_scan':
                    if len(parts) == 2:
                        arg1, arg2 = map(int, parts[1].split(','))  # Split and convert arguments to integers
                        machine_code.extend([arg1, arg2])
                else:
                    arg = int(parts[1])
                    machine_code.append(arg)
    machine_code.append(0xFF)

    return machine_code


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
