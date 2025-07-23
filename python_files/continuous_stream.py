import serial
import numpy as np
import struct
import sys
import argparse
import time
from PIL import Image
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.font_manager as fm

# 系统字体配置
if sys.platform == 'win32':
    import ctypes
    ctypes.windll.shcore.SetProcessDpiAwareness(True)
    plt.rcParams['font.sans-serif'] = ['Microsoft YaHei']
    plt.rcParams['axes.unicode_minus'] = False
elif sys.platform == 'darwin':
    pingfang_path = '/Library/Fonts/仿宋_GB2312.ttf'
    pingfang_prop = fm.FontProperties(fname=pingfang_path, size=10)

# 支持的分辨率模式
RESOLUTIONS = {
    'qcif': (176, 144),
    '160x120': (160, 120),
    'qvga': (320, 240)
}

class OV7670ContinuousStream:
    def __init__(self, port, resolution='160x120'):
        self.port = port
        self.width, self.height = RESOLUTIONS[resolution.lower()]
        self.lut_r = np.array([(x * 255) // 31 for x in range(32)], dtype=np.uint8)
        self.lut_g = np.array([(x * 255) // 63 for x in range(64)], dtype=np.uint8)
        self.lut_b = np.array([(x * 255) // 31 for x in range(32)], dtype=np.uint8)
        
        # 初始化串口
        self.ser = serial.Serial(
            port=port,
            baudrate=1500000,
            timeout=0.5
        )
        
    def start_stream(self):
        """启动连续流模式"""
        self.ser.write(struct.pack('B', 0xDD))  # CMD_STREAM
        
    def stop_stream(self):
        """停止连续流模式"""
        self.ser.write(struct.pack('B', 0x00))  # 停止命令
        
    def get_frame(self):
        """高效获取一帧数据"""
        expected_size = self.width * self.height * 2
        # 确保读取完整帧数据
        raw = bytearray()
        while len(raw) < expected_size:
            chunk = self.ser.read(expected_size - len(raw))
            if not chunk:
                print(f"警告: 只收到 {len(raw)}/{expected_size} 字节")
                return None
            raw.extend(chunk)
        
        # 调试输出
        print(f"收到完整帧: {len(raw)}字节 | 首字节: {raw[0]:02x} {raw[1]:02x}...")
        
        try:
            # 将原始数据转换为uint16数组并正确reshape
            arr = np.frombuffer(raw, dtype=np.uint16).reshape(self.height, self.width).T
            
            # 直接向量化处理RGB565转换
            rgb = np.empty((self.width, self.height, 3), dtype=np.uint8)
            rgb[..., 0] = self.lut_r[(arr >> 11) & 0x1F]  # R
            rgb[..., 1] = self.lut_g[(arr >> 5) & 0x3F]    # G
            rgb[..., 2] = self.lut_b[arr & 0x1F]          # B
            
            # 水平翻转解决镜像问题
            rgb = np.fliplr(rgb)
            
            return rgb
        except Exception as e:
            print(f"图像处理错误: {e}")
            return None

    def close(self):
        self.ser.close()

def update_frame(i, display, stream):
    """更新帧显示"""
    frame = stream.get_frame()
    if frame is not None:
        display.set_array(frame)
    return display,

def main():
    parser = argparse.ArgumentParser(description='OV7670 连续视频流')
    parser.add_argument('port', help='串口设备路径')
    parser.add_argument('--resolution', choices=RESOLUTIONS.keys(), default='qvga',
                      help='分辨率模式 (default: qvga)')
    
    args = parser.parse_args()
    
    try:
        stream = OV7670ContinuousStream(args.port, args.resolution)
        stream.start_stream()
        
        # 初始化显示
        plt.style.use('dark_background')
        fig, ax = plt.subplots()
        ax.set_title(f"OV7670 连续视频流 ({args.resolution}) - 按 Ctrl+C 停止",
                    fontproperties=pingfang_prop if sys.platform == 'darwin' else None)
        
        # 创建初始空白图像(红色用于调试)
        initial_img = np.zeros((*RESOLUTIONS[args.resolution.lower()], 3), dtype=np.uint8)
        initial_img[..., 0] = 64  # 红色背景便于调试
        display = ax.imshow(initial_img)
        plt.draw()  # 强制立即渲染
        
        # 创建动画并优化性能
        ani = animation.FuncAnimation(
            fig, update_frame, fargs=(display, stream),
            interval=16,  # ~60fps
            blit=True,
            cache_frame_data=False
        )
        # 禁用不必要的图形功能
        fig.canvas.toolbar = None
        fig.canvas.header_visible = False
        fig.canvas.footer_visible = False
        fig.canvas.resizable = False
        plt.tight_layout()
        
        # 禁用工具栏以提升性能
        plt.rcParams['toolbar'] = 'None'
        
        plt.show()
        
    except serial.SerialException as e:
        print(f"串口错误: {e}")
    except KeyboardInterrupt:
        print("\n用户中断，退出程序")
    finally:
        try:
            stream.stop_stream()
            stream.close()
        except:
            pass

if __name__ == "__main__":
    main()
