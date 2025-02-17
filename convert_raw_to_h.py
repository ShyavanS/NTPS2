# convert_raw_to_h.py
"""
A quick & dirty script to convert raw image files into a char array for packing
into a binary during c program compilation. Eliminates the need for including
texture data with binaries seperately.
"""

# Open the raw image as binary format
with open("emotionengine.raw", "rb") as f:
    data = f.read()

# Write it out to a c header file as an unsigned char array
with open("ntps2_font.h", "w") as f:
    f.write("unsigned char font_raw[] = {")
    f.write(",".join(f"0x{b:02x}" for b in data))
    f.write("};\n")
    f.write(f"unsigned int font_raw_len = {len(data)};")