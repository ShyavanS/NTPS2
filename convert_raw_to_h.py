with open("NTPS2_Logo.raw", "rb") as f:
    data = f.read()

with open("ntps2_logo.h", "w") as f:
    f.write("unsigned char bg_raw[] = {")
    f.write(",".join(f"0x{b:02x}" for b in data))
    f.write("};\n")
    f.write(f"unsigned int bg_raw_len = {len(data)};")