import os
import struct
import math
from lib.font import name_char_map
DIR_ENTRY_SIZE = 16 # address (2 bytes) + file name (10 bytes) + file length (4 bytes)
MAX_FS_SIZE = 125 * 1000 * 1000

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
    return root_entry


    

