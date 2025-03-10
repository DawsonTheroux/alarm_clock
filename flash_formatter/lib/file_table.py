import os
import struct
import math
import serial
import time 

from lib.font import name_char_map
DIR_ENTRY_SIZE = 16 # address (2 bytes) + file name (10 bytes) + file length (4 bytes)
# MAX_FS_SIZE = 125 * 1000 * 1000 # 1GBit
MAX_FS_SIZE = 32 * 1000 * 1000

# NOTE: This could probably be designed better. 
#       But I think this modular approach is easily modifyable.
#       


#
# convert_filename
#
# Given a path, returns the filename portion of the name.
# Right now some of the rules include:
#    Names are converted back into single characters according lib.find name_char_map
#    Names that include upper_ are changed simply to the uppercase character they represent.
#
# Returns the bytearray representation of the filename.
def convert_filename(filename):
    # Get the name portion of the file. IF the name has a corresponding char in name_char_map, set it to that value.
    filename = filename.split("\\")[-1].split(".")[0]
    if "upper_" in filename:
        filename = filename[6:].upper()
    return bytearray(name_char_map[filename] if filename in name_char_map.keys() else filename, encoding="ASCII")


def create_entry_bytes(entry, is_current_dir, page_size):
    entry_bytes = bytearray(DIR_ENTRY_SIZE)

    entry_bytes[0:2] = bytearray(struct.pack('<H', entry["offset"]))
    entry_bytes[2:6] = bytearray(struct.pack('<I', entry["length"]))
    filename = convert_filename(entry["name"]) if not is_current_dir else bytearray('.', encoding="ASCII")
    entry_bytes[6:6 + len(filename)] = filename
    if entry["is_dir"]:
        entry_bytes[13:16] = bytearray([0xFF, 0xFF, 0xFF])
    else:
        entry_bytes[13:16] = bytearray(entry["name"].split("\\")[-1].split(".")[1], encoding="ASCII")
    return entry_bytes



#
# add_fs_directory 
#
def add_fs_directory(fs_bin, directory_entry, page_size):
    current_offset = directory_entry["offset"] * page_size
    fs_bin[current_offset: current_offset + DIR_ENTRY_SIZE] = create_entry_bytes(directory_entry, True, page_size)
    for entry in directory_entry["entries"]:
        current_offset += DIR_ENTRY_SIZE
        fs_bin[current_offset: current_offset + DIR_ENTRY_SIZE] = create_entry_bytes(entry, False, page_size)

#
# add_fs_file
#
def add_fs_file(fs_bin, file_entry, page_size):
    with open(file_entry["name"], "rb") as file:
        fs_bin[(file_entry["offset"] * page_size):(file_entry["offset"] * page_size) + file_entry["length"]] = bytearray(file.read())

#
# output_fs_to_file
#
def output_fs_to_file(root_dir, output_file, page_size):
    print("Outputting file to fs")
    fs_bin = bytearray(MAX_FS_SIZE)
    to_visit = []
    current_dir = root_dir
    while current_dir != None:
        add_fs_directory(fs_bin, current_dir, page_size)
        for file_entry in current_dir["entries"]:
            if file_entry["is_dir"]:
                to_visit.append(file_entry)
            else:
                add_fs_file(fs_bin, file_entry, page_size)

        if len(to_visit) != 0:
            current_dir = to_visit[0]
            to_visit = to_visit[1:]
        else:
            break

    # Write the binary data to the output file.
    with open(output_file, "wb") as fs_file:
        fs_file.write(fs_bin)

#
# send_uart_payload
#
def send_uart_payload(uart, payload, offset, page_size):
		# Extract the bytes for the offset
		bytes_written = 0
		# current_offset = offset
		current_length = 0
		offset_in_payload = 0
		length_remaining = len(payload) 

		while length_remaining > 0:
				# Set the current length to the either the page size of the length remaining to send.
				current_length = length_remaining if (length_remaining <= page_size) else page_size

				# Create the header.
				index = 0
				header = [0, 0, 0, 0, 0]
				# # The offset.
				# header[0] = (current_offset & 0xFF00) >> 8 # MSB of the offset
				# header[1] = (current_offset & 0xFF)        # LSB of the offset
				# # The size.
				# header[2] = (current_length & 0x0000ff) >> 16
				# header[3] = (current_length & 0x00ff00) >> 8
				# header[4] = (current_length & 0x0000ff)

				header[0] = ((offset + offset_in_payload) & 0xFF00) >> 8 # MSB of the offset
				header[1] = ((offset + offset_in_payload) & 0xFF)        # LSB of the offset
				# The size.
				header[2] = (current_length & 0x0000FF) >> 16
				header[3] = (current_length & 0x00FF00) >> 8
				header[4] = (current_length & 0x0000FF)
				header = bytearray(header)

				# Send the header
				print(f"\nSending package for header - {offset_in_payload=} - {current_length=} - {header=}")
				if uart.write(header) != 5:
						print("uart.write didn't send 5 bytes")

				# Wait for ACK of the header
				if (value:= uart.read(len("PROG_ACK") + 8))[0:len("PROG_ACK")] != "PROG_ACK".encode('ascii'):
						print("FAILED TO RECEIVE ACK ON PACKET")
						time.sleep(10)
						print(f"{offset_in_payload=} {current_length=} - RECEIVED: {value}{uart.read(uart.in_waiting)}")
						exit()
				print(f"Received ACK for header - {offset_in_payload=} - {current_length=} - {header=} - {value=}")
				print(f"\nSending payload of size {len(payload[offset_in_payload * page_size :((offset_in_payload *page_size)+ current_length)])}")
				# Send the payload
				if uart.write(payload[offset_in_payload * page_size :((offset_in_payload * page_size) + current_length)]) != len(payload[offset_in_payload * page_size :((offset_in_payload *page_size)+ current_length)]):
						print("uart.write didn't send full payload")

				#	Wait for ACK from device. (Maybe this could also double check the header from the device)
				uart.reset_input_buffer()
				# if (value:= uart.read_until("PROG_ACK".encode('ascii'), 50)) != "PROG_ACK".encode('ascii'):
				if (value:= uart.read(len("PROG_DONE") + 8))[0:len("PROG_DONE")] != "PROG_DONE".encode('ascii'):
						print("FAILED TO RECEIVE DONE ON PACKET")
						time.sleep(10)
						print(f"{offset_in_payload=} {current_length=} - RECEIVED: {value}{uart.read(uart.in_waiting)}")
						exit()
				print(f"Received DONE for package with header - {offset_in_payload=} - {current_length=} - {header=} - {value=}")

				# Remove the length written from the length remaining
				offset_in_payload += (int)(current_length / page_size)
				length_remaining -= current_length
		
#
# output_fs_dir_to_com
#
def output_fs_dir_to_com(uart, directory_entry, page_size):
		payload = bytearray()
		payload += create_entry_bytes(directory_entry, True, page_size)
		for entry in directory_entry["entries"]:
				payload += create_entry_bytes(entry, False, page_size)

		send_uart_payload(uart, payload, directory_entry["offset"], page_size)

#
# output_fs_to_com
#
def output_fs_file_to_com(uart, file_entry, page_size):
		payload = None
		with open(file_entry["name"], "rb") as file:
				payload =  bytearray(file.read())

		if payload == None:
				print(f"Failed to read file: {file_entry['name']}")

		send_uart_payload(uart, payload, file_entry["offset"], page_size)

#
# output_fs_to_com
#
def output_fs_to_com(root_dir, com_port, page_size):
        uart = serial.Serial(com_port, 115200, timeout=10)
        print(uart.name)

        # Send PROG_START, receive PROG_ACK.
        uart.write("PROG_START".encode('ascii'))
        #uart.reset_input_buffer()
        if (error := uart.read_until("PROG_ACK".encode('ascii'), 20)) != "PROG_ACK".encode('ascii'):
            print(f"ERROR: FAILED TO RECEIVE ACK {error=}")
            time.sleep(10)
            print(f"{uart.read(uart.in_waiting)}")
            exit()
        print("Received PROG_ACK")


        # Receive PROG_READY.
        #uart.reset_input_buffer()
        uart.timeout = 300
        if (error := uart.read_until("PROG_READY".encode('ascii'), 20)) != "PROG_READY".encode('ascii'):
            print(f"FAILED TO RECEIVE PROG_READY {error=}")
            time.sleep(10)
            print(f"{uart.read(uart.in_waiting)}")
            exit()
        print("Received PROG_READY")

        # The data
        # Header = 5 bytes
        #		(2) target index in pages
        #   (3) Package size with max of 1 page
        # Payload = FS PAGE SIZE (Can be smaller)

        # Parse the tree.
        to_visit = []
        current_dir = root_dir
        while current_dir != None:
            print(f"\n---Writting directory: {current_dir['name']} to device.---")
            output_fs_dir_to_com(uart, current_dir, page_size)
            #add_fs_directory(fs_bin, current_dir, page_size)
            for file_entry in current_dir["entries"]:
                if file_entry["is_dir"]:
                    to_visit.append(file_entry)
                else:
                    print(f"\n---Writting file: {file_entry['name']} to device.---")
                    output_fs_file_to_com(uart, file_entry, page_size)
                    # add_fs_file(fs_bin, file_entry, page_size)
            if len(to_visit) != 0:
                current_dir = to_visit[0]
                to_visit = to_visit[1:]
            else:
                break

        uart.write("PROG_DONE".encode('ascii'))

#
# generate_file_table
#   
def generate_file_table(root_dir, page_size):
    to_visit = []
    root_entry = {"name": root_dir, 
                  "offset": 0, 
                  "length": (len(os.listdir(root_dir)) + 1) * DIR_ENTRY_SIZE,
                  "entries": [], 
                  "is_dir": True}
    current_dir = root_entry 
    current_offset = math.ceil((len(os.listdir(current_dir["name"])) * DIR_ENTRY_SIZE) / page_size)
    # current_offset = 0
    fs_bin = bytearray()

    # For the current directory, create the whole table.
    while current_dir != None:
        print(f"Looking at directory: {current_dir['name']}")
        child_files = os.listdir(current_dir["name"])
        # The size of the current dir is the length of all the children + itself.

        for filename in child_files:
            file_dict = {}
            file_dict["name"] = os.path.join(current_dir["name"], filename)
            file_dict["offset"] = current_offset
            file_dict["length"] = 0
            file_dict["entries"] = []

            if os.path.isdir(file_dict["name"]):
                # Append a pointer to the dictionary to be used later.
                # The size of the directory will be set when it is visited. 
                file_dict["is_dir"] = True
                file_dict["length"] = (len(os.listdir(file_dict["name"])) + 1) * DIR_ENTRY_SIZE
                assert file_dict["length"] <= page_size, "The size of a directory should not exceed the size of a page...because laziness"
                to_visit.append(file_dict)
            else:
                file_dict["is_dir"] = False
                file_dict["length"] = os.stat(file_dict["name"]).st_size

            # file_dict["offset"] = current_offset
            current_dir["entries"].append(file_dict)

            current_offset += math.ceil(file_dict["length"] / page_size)
        if len(to_visit) != 0:
            current_dir = to_visit[0]
            to_visit = to_visit[1:]
        else:
            break
    print(f"at end: {current_offset=}")
    return root_entry


    

