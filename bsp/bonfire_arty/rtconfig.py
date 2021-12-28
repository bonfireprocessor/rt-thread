import os
ARCH     = 'risc-v'
CPU      = 'bonfire'
# toolchains options
CROSS_TOOL  = 'gcc'

#------- toolchains path -------------------------------------------------------
if os.getenv('RTT_CC'):
    CROSS_TOOL = os.getenv('RTT_CC')

if  CROSS_TOOL == 'gcc':
    PLATFORM    = 'gcc'
    EXEC_PATH   = r'/home/thomas/opt/riscv/bin'
else:
    print('Please make sure your toolchains is GNU GCC!')
    exit(0)

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

BUILD = 'debug'
#BUILD = 'release'

CORE = 'risc-v'
MAP_FILE = 'rtthread.map'
LINK_FILE = './bonfire-sdk/ld/ram_arty_axi.ld'
TARGET_NAME = 'rtthread.bin'

#------- GCC settings ----------------------------------------------------------
if PLATFORM == 'gcc':
    if os.getenv("TARGET_PREFIX"):
        PREFIX = os.getenv("TARGET_PREFIX") + "-"
        print(PREFIX)
    else:    
        PREFIX = 'riscv-none-embed-'

    CC = PREFIX + 'gcc'
    CXX= PREFIX + 'g++'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'elf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    INCDIRS = ' -I./bonfire-sdk/inc -I./bonfire-sdk/boards/ARTY_AXI -I./drivers '

    DEVICE = ' -march=rv32imac -mabi=ilp32 -mstrict-align -mcmodel=medany  -nostartfiles  -lc '
    CFLAGS = DEVICE
    CFLAGS += ' -save-temps=obj'
    CFLAGS +=  INCDIRS
    AFLAGS = '-c'+ DEVICE + ' -x assembler-with-cpp'
    AFLAGS += INCDIRS
    LFLAGS = DEVICE
    LFLAGS += ' -Wl,--gc-sections,-cref,-Map=' + MAP_FILE
    LFLAGS += ' -L./bonfire-sdk/ld  -T ' + LINK_FILE
   

    CPATH = ''
    LPATH = ''

    if BUILD == 'debug':
        print("Debug build")
        CFLAGS += ' -Og -g3'
        AFLAGS += ' -g3'
    else:
        CFLAGS += ' -g -O2'

    POST_ACTION = OBJCPY + ' -O binary $TARGET ' + TARGET_NAME + '\n'
    POST_ACTION += SIZE + ' $TARGET\n'
    POST_ACTION += OBJDUMP + ' -S -d $TARGET >rtthread.lst\n'
    if os.getenv("UPLOAD_DIR"):       
        POST_ACTION += 'cp ' + TARGET_NAME + ' ' + os.getenv("UPLOAD_DIR")
