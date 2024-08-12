import freetype
import copy

char_name_map = {
        '.':"dot",
        ':':"colon",
        ';':"scolon",
        ',':"comma",
        "'":"apos",
        '(':"opar",
        ')':"cpar",
        '+':"plus",
        '-':"minus",
        '*':"star",
        '/':"slash",
        '=':"equal",
        ' ':"space",
        }

name_char_map = {
        "dot":'dot',
        "colon":':',
        "scolon":';',
        "comma":',',
        "apos":"'",
        "opar":'(',
        "cpar":')',
        "plus":'+',
        "minus":'-',
        "star":'*',
        "slash":'/',
        "equal":'=',
        'space':" ",
        }
def generate_font(font_file, char_list, font_size, output_directory, time_digit):
    face = freetype.Face(font_file)
    face.set_char_size(font_size * 64)  # Setting the character size to 48 points
    fixed_width = None
    fixed_height = None
    
    for char in char_list:
        char_filename = char
        if char in char_name_map.keys():
            char_filename = char_name_map[char]

        if char_filename.isupper():
            char_filename = f"upper_{char_filename.lower()}"

        if len(char_filename) > 7:
            raise ValueError('The filename must be at MOST 7 characters long')

        face.load_char(char)  # Load the glyph for character 'A'
        bitmap = face.glyph.bitmap
        width, height, pitch = bitmap.width, bitmap.rows, bitmap.pitch
        buffer = bitmap.buffer

        if fixed_width is None and fixed_height is None:
            fixed_width = width
            fixed_height = height
        
        small_width = int(width / 4) if width % 4 == 0 else int(width / 4) + 1
        # The first two values in DAT at the height and the width (NOTE WIDTH IS IN BYTES NOT BITS)
        print(f"{height=} top={face.glyph.bitmap_top} left={face.glyph.bitmap_left}")
        output_data = [height,(small_width*4), face.glyph.bitmap_top, face.glyph.bitmap_left if face.glyph.bitmap_left > 0 else 0]
        for h in range(height):
            for w in range(small_width):
                value = 0x00
                for i in range(4):
                    if (w * 4) + i >= width:
                        # Shift the data to the left the rest of the way
                        value = value << (((3 - i) * 2) + 2)
                        break

                    if i != 0 or i != 3:
                        value = value << 2

                    # Only display time digits in binary format.
                    if time_digit:
                        if buffer[(h * width) + (w * 4) + i] > 100:
                            value = value | 0x3
                    else:
                        if buffer[(h * width) + (w * 4) + i] > 180:
                            value = value | 0x3
                        elif buffer[(h * width) + (w * 4) + i] > 130:
                            value = value | 0x1
                        elif buffer[(h * width) + (w * 4) + i] > 65:
                            value = value | 0x2

                output_data.append((~value) & 0xFF)
        """
        if time_digit and fixed_height != height and fixed_width != width:
            old_output_data = copy.deepcopy(output_data)
            fixed_small_width = int(fixed_width / 4) if fixed_width % 4 == 0 else int(fixed_width / 4) + 1
            # The first two values in DAT at the height and the width (NOTE WIDTH IS IN BYTES NOT BITS)
            start_width = int((fixed_small_width / 2) - (small_width / 2))
            end_width = int((fixed_small_width / 2) + (small_width / 2))
            start_height = int((fixed_height / 2) - (height / 2))
            end_height = int((fixed_height / 2) + (height / 2))
            output_data = [fixed_height,(fixed_small_width*4)]
            for h in range(height):
                for w in range(fixed_small_width):
                    if h >= start_height and h <= end_height and w >= start_width and w <= end_width:
                        # output_data.append(old_output_data[((h * fixed_small_width) - (start_height * fixed_small_width)) + w - start_width])
                        output_data.append(old_output_data[((h - start_height) * small_width) + w - start_width])

                    else:
                        output_data.append(0xFF)
            # Figure out where the small rectangle first in the bigger one.
            # Put the small data in the big data.
        """

        with open(f"{output_directory}/{char_filename}.CHR", "wb") as file:
            file.write(bytes(output_data))
