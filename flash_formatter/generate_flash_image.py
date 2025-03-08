import os
import shutil
import freetype
from lib.font import generate_font 
from lib.file_table import generate_file_table, output_fs_to_file, output_fs_to_com


COM_PORT = "COM11"
OUTPUT_DIR = "output"
OUTPUT_FILE = "output/output.bin"
PSEUDO_FS = f"{OUTPUT_DIR}/pseudo_fs" # The directory that all the files should be stored in.
DATA_DIR = "data"                             # The main data folder.
IMAGE_DIR = f"{DATA_DIR}/images"              # The list of iamges to include.

SMALL_FONT = 35
MED_FONT = 100
LARGE_FONT = 200
TIME_FONT = 250

# Font settings
CHAR_LIST = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890.:;,'()+-*/= "
FONT_SIZE = 100                                 # Use 48pt font.
#FONT_FILE = "digital-7-mono.ttf"  # The current font to use.
# FONT_FILE = "Roboto-Regular.ttf"  # The current font to use.
# FONT_FILE = "Gupter-Regular.ttf"  # The current font to use.
# FONT_FILE = "IBMPlexMono-Regular.ttf"  # The current font to use.
FONT_FILE = "ShareTechMono-Regular.ttf"  # The current font to use.

# File system settings
PAGE_SIZE = 2048

def main():
	if os.path.isdir(OUTPUT_DIR):
			shutil.rmtree(OUTPUT_DIR)
	
	os.mkdir(OUTPUT_DIR)
	os.mkdir(f"{PSEUDO_FS}")
	
	# Generate the large font.
	os.mkdir(f"{PSEUDO_FS}/timfnt")
	generate_font(f"data/{FONT_FILE}", "0123456789:", TIME_FONT, f"{PSEUDO_FS}/timfnt", True)
	
	# Generate the large font.
	os.mkdir(f"{PSEUDO_FS}/lrgfnt")
	generate_font(f"data/{FONT_FILE}", CHAR_LIST, LARGE_FONT, f"{PSEUDO_FS}/lrgfnt", True)
	
	# Generate the medium font.
	os.mkdir(f"{PSEUDO_FS}/medfnt")
	generate_font(f"data/{FONT_FILE}", CHAR_LIST, MED_FONT, f"{PSEUDO_FS}/medfnt", True)
	
	# Generate the small font.
	os.mkdir(f"{PSEUDO_FS}/smlfnt")
	generate_font(f"data/{FONT_FILE}", CHAR_LIST, SMALL_FONT, f"{PSEUDO_FS}/smlfnt", True)
	
	file_table = generate_file_table(PSEUDO_FS, PAGE_SIZE)
	# output_fs_to_file(file_table, OUTPUT_FILE, PAGE_SIZE)
	output_fs_to_com(file_table, COM_PORT, PAGE_SIZE)
    

if __name__ == "__main__":
		print("Hello")
		main()



