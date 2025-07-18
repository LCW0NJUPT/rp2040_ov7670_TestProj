import serial
import struct
import sys
from PIL import Image
import matplotlib.pyplot as plt

def capture_image(port):
    # 初始化串口
    ser = serial.Serial(port, 1500000, timeout=2)
    
    try:
        # 发送捕获命令
        ser.write(struct.pack('B', 0xCC))  # CMD_CAPTURE
        
        # 读取图像数据 (QVGA 320x240 RGB565)
        raw = ser.read(320 * 240 * 2)
        if len(raw) != 320 * 240 * 2:
            raise ValueError(f"Expected {320*240*2} bytes, got {len(raw)}")
        
        # 创建图像
        img = Image.new('RGB', (320, 240))
        data = img.load()
        
        # 解析RGB565数据并进行颜色转换
        for y in range(240):
            for x in range(320):
                idx = y * 320 + x
                v = struct.unpack('<H', raw[2*idx:2*(idx+1)])[0]
                
                # 提取RGB分量
                r = (v >> 11) & 0x1F  # 提取5位红色
                g = (v >> 5) & 0x3F   # 提取6位绿色
                b = v & 0x1F          # 提取5位蓝色
                
                # 扩展颜色通道至8位
                data[x, y] = (
                    int(r / 0x1f * 0xff),  # 红色
                    int(g / 0x3f * 0xff),  # 绿色 
                    int(b / 0x1f * 0xff)   # 蓝色
                )
                
        # 显示图像
        plt.figure(figsize=(10, 8))
        plt.imshow(img)
        plt.title("实时图像")
        plt.axis('off')
        plt.show()

        
        # 保存图像
        img.save('captured_image.png')
        print("图像已保存为 captured_image.png")
        
    finally:
        ser.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"使用方法: python {sys.argv[0]} 串口设备路径")
        print("示例: python simple_capture.py /dev/tty.SLAB_USBtoUART")
        sys.exit(1)
        
    capture_image(sys.argv[1])