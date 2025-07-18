import atexit
import IPython
import math
from PIL import Image
import serial
import struct
import sys
import matplotlib.pyplot as plt

CMD_REG_WRITE = 0xAA
CMD_REG_READ = 0xBB
CMD_CAPTURE = 0xCC

ser = serial.Serial(sys.argv[1], 1500000)
atexit.register(lambda: ser.close())

# ======================
# Actual device commands
# ======================
def reg_write(reg, value):
    ser.reset_output_buffer()
    ser.reset_input_buffer()

    assert isinstance(reg, int)
    assert 0x00 <= reg <= 0xFF
    assert isinstance(value, int)
    assert 0x00 <= value <= 0xFF

    ser.write(struct.pack('BBB', CMD_REG_WRITE, reg, value))

def reg_read(reg):
    ser.reset_output_buffer()
    ser.reset_input_buffer()

    assert isinstance(reg, int)
    assert 0x00 <= reg <= 0xFF

    ser.write(struct.pack('BB', CMD_REG_READ, reg))

    return ser.read(1)[0]

def capture(filename):
    ser.reset_output_buffer()
    ser.reset_input_buffer()

    ser.write(struct.pack('B', CMD_CAPTURE))

    raw = ser.read(320*240*2)
    
    img = Image.new('RGB', (320, 240))
    width, height = img.size
    data = img.load()
    print('img content:',data[0,0])
    for y in range(height):
        for x in range(width):
            idx = y * width + x
            v = struct.unpack('<H', raw[2*idx:2*(idx+1)])[0]

            r, g, b = v >> (5 + 6), (v >> 5) & 0b111111, v & 0b11111 


            r = math.floor(r / 0x1f * 0xff)
            g = math.floor(g / 0x3f * 0xff)
            b = math.floor(b / 0x1f * 0xff)
            # r = math.floor(r* 8 ) same effect as the code above
            # g = math.floor(g* 4 )
            # b = math.floor(b* 8 )
            data[x, y] = (r, g, b)
    plt.plot()
    plt.imshow(img)
    plt.show()
    img.save(filename)

# =======
# Helpers
# =======
def reg_write_list(l):
    for reg, value in l:
        reg_write(reg, value)

def reg_set_bit(reg, bit):
    value = reg_read(reg)
    value |= 1 << bit
    reg_write(reg, value)

def reg_clear_bit(reg, bit):
    value = reg_read(reg)
    value &= 0xFF ^ (1 << bit)
    reg_write(reg, value)

def reg_get_bit(reg, bit):
    value = reg_read(reg)
    return (value >> bit) & 1

# ====================
# Camera configuration
# ====================
def set_hflip(hflip):
    if hflip:
        reg_set_bit(0x1e, 5)
    else:
        reg_clear_bit(0x1e, 5)

def set_vflip(vflip):
    if vflip:
        reg_set_bit(0x1e, 4)
    else:
        reg_clear_bit(0x1e, 4)

IPython.embed()
