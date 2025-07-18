import serial
import struct
import sys
import time
from PIL import Image
import matplotlib.pyplot as plt

def capture_frame(ser):
    # 发送捕获命令
    ser.write(struct.pack('B', 0xCC))  # CMD_CAPTURE
    
    # 读取图像数据 (QVGA 320x240 RGB565)
    raw = ser.read(320 * 240 * 2)
    if len(raw) != 320 * 240 * 2:
        print(f"数据长度异常: {len(raw)}")
        return None

    # 创建图像
    img = Image.new('RGB', (320, 240))
    data = img.load()
    
    # 解析RGB565数据
    for y in range(240):
        for x in range(320):
            idx = y * 320 + x
            v = struct.unpack('<H', raw[2*idx:2*(idx+1)])[0]
            
            # 提取RGB分量
            r = (v >> 11) & 0x1F
            g = (v >> 5) & 0x3F
            b = v & 0x1F
            
            # 扩展到8位
            data[x, y] = (
                int(r * 255 / 31),
                int(g * 255 / 63),
                int(b * 255 / 31)
            )
    
    return img

def main(port):
    # 初始化串口
    ser = serial.Serial(port, 1500000, timeout=2)
    print("已连接，开始视频流捕获（按 Ctrl+C 退出）...")
    
    # 创建显示窗口
    plt.ion()
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.set_title("实时视频流（调整焦距）")
    display = ax.imshow(Image.new('RGB', (320, 240)))
    
    try:
        while True:
            frame = capture_frame(ser)
            if frame:
                display.set_data(frame)
                fig.canvas.draw()
                fig.canvas.flush_events()
    except KeyboardInterrupt:
        print("\n视频流结束")
    finally:
        ser.close()
        plt.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"使用方法: python {sys.argv[0]} 串口设备路径")
        print("示例: python video_stream.py /dev/tty.SLAB_USBtoUART")
        sys.exit(1)
        
    main(sys.argv[1])