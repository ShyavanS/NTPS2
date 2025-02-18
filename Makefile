# Makefile for NTPS2. Compiles into two binaries, one compressed, another uncompressed.
# Mostly replicated from makefile for wLaunchELF.

# Make Binaries
EE_BIN = NTPS2-UNC.ELF
EE_BIN_PKD = NTPS2.ELF

# Object files
EE_OBJS = main.o DEV9_irx.o NETMAN_irx.o SMAP_irx.o time_math.o ntp.o net.o controller.o graphics.o

# Build Directories
EE_OBJS_DIR = obj/
EE_ASM_DIR = asm/
EE_OBJS := $(EE_OBJS:%=$(EE_OBJS_DIR)%)

# Include directories
EE_INCS := -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/ports/include

# Compilation Flags and Libraries
EE_LDFLAGS := -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ports/lib -s
EE_LIBS = -lgskit -ldmakit -ljpeg_ps2_addons -lpng -ljpeg -lpad -lmc -lkbd -lm \
		  -lpatches -lpoweroff -lsior -lps2ips -lps2ip -lnetman
EE_CFLAGS := -mno-gpopt -G0 -fno-strict-aliasing

# Include dependant headers for files
main.o: main.c time_math.h ntp.h net.h controller.h graphics.h
graphics.o: ntps2_logo.h ntps2_font.h

all: githash.h $(EE_BIN_PKD)

$(EE_BIN_PKD): $(EE_BIN)
	ps2-packer $(EE_BIN) $(EE_BIN_PKD)

$(EE_BIN): $(EE_OBJS)
	$(EE_CC) $(EE_LDFLAGS) -o $@ $^

run: all
	ps2client -h 192.168.0.10 -t 1 execee host:$(EE_BIN)
reset: clean
	ps2client -h 192.168.0.10 reset

format:
	find . -type f -a \( -iname \*.h -o -iname \*.c \) | xargs clang-format -i

format-check:
	@! find . -type f -a \( -iname \*.h -o -iname \*.c \) | xargs clang-format -style=file -output-replacements-xml | grep "<replacement " >/dev/null

githash.h:
	printf '#ifndef ULE_VERDATE\n#define ULE_VERDATE "' > $@ && \
	git show -s --format=%cd --date=local | tr -d "\n" >> $@ && \
	printf '"\n#endif\n' >> $@
	printf '#ifndef GIT_HASH\n#define GIT_HASH "' >> $@ && \
	git rev-parse --short HEAD | tr -d "\n" >> $@ && \
	printf '"\n#endif\n' >> $@

clean:
	rm -rf githash.h $(EE_BIN) $(EE_BIN_PKD) $(EE_ASM_DIR) $(EE_OBJS_DIR)

rebuild: clean all

# Create Directories
$(EE_ASM_DIR):
	@mkdir -p $@

$(EE_OBJS_DIR):
	@mkdir -p $@

# Compiler Command
$(EE_OBJS_DIR)%.o: $(EE_SRC_DIR)%.c | $(EE_OBJS_DIR)
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

# Include Samples
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
