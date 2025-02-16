with open("emotionengine.raw", "rb") as f:
    data = f.read()

with open("ntps2_font.h", "w") as f:
    f.write("unsigned char font_raw[] = {")
    f.write(",".join(f"0x{b:02x}" for b in data))
    f.write("};\n")
    f.write(f"unsigned int font_raw_len = {len(data)};")