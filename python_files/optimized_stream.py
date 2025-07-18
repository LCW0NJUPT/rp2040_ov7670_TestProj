import serial
import numpy as np
import struct
import sys
import argparse
import time  # 添加time模块
from PIL import Image
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.font_manager as fm

# ===== macOS 字体配置 =====
# pingfang_path = '/Library/Fonts/仿宋_GB2312.ttf'

# 创建字体属性对象
# pingfang_prop = fm.FontProperties(fname=pingfang_path, size=10)

# 设置全局字体配置
# plt.rcParams['font.family'] = '仿宋_GB2312'
# plt.rcParams['axes.unicode_minus'] = False  # 正常显示减号
plt.rcParams['font.sans-serif'] = ['SimHei'] # 设置字体为黑体
plt.rcParams['axes.unicode_minus'] = False # 解决负号显示为方块的问题
# =========================

# 支持的分辨率模式
RESOLUTIONS = {
    'qcif': (176, 144),   # 153600 bytes at RGB565
    '160x120': (160, 120),# 38400 bytes
    'qvga': (320, 240)    # 153600 bytes
}

def optimize_camera_settings(ser, resolution):
    """发送优化后的摄像头配置（需根据实际硬件实现）"""
    print("提示：建议在硬件端配置以下优化：")
    print(f"1. 设置分辨率为 {resolution}")
    print("2. 关闭自动曝光/白平衡")
    print("3. 设置像素时钟为 12MHz")
    print("4. 启用帧率模式 0x11=0x01")

class OV7670Stream:
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
            timeout=0.5  # 降低超时提高响应速度
        )
        
        optimize_camera_settings(self.ser, resolution)

    def capture_frame(self):
        """高效捕获单帧数据"""
        self.ser.write(struct.pack('B', 0xCC))  # CMD_CAPTURE
        
        # 预分配缓冲区
        raw = bytearray()
        expected_size = self.width * self.height * 2
        
        # 分块读取提高效率
        while len(raw) < expected_size:
            chunk = self.ser.read(expected_size - len(raw))
            if not chunk:
                print("警告：超时未接收到完整数据包")
                return None
            raw.extend(chunk)
        
        # 使用 NumPy 高效处理数据
        arr = np.frombuffer(raw, dtype=np.uint16).reshape(self.height, self.width)
        
        # 向量化位操作
        r = self.lut_r[(arr >> 11) & 0x1F]
        g = self.lut_g[(arr >> 5) & 0x3F]
        b = self.lut_b[arr & 0x1F]
        
        # 合并通道
        rgb = np.stack([r, g, b], axis=-1)
        return Image.fromarray(rgb)

    def close(self):
        self.ser.close()

def update_frame(frame, display, stream):  # 修复参数顺序
    """实时更新帧"""
    frame = stream.capture_frame()
    if frame:
        display.set_array(frame)
    return display,

def main():
    parser = argparse.ArgumentParser(description='OV7670 实时视频流优化版')
    parser.add_argument('port', help='串口设备路径')
    parser.add_argument('--resolution', choices=RESOLUTIONS.keys(), default='qvga',
                        help='分辨率模式 (default: qvga)')
    parser.add_argument('--save', action='store_true', help='保存最后一帧')
    
    args = parser.parse_args()
    
    try:
        stream = OV7670Stream(args.port, args.resolution)
        
        # 初始化显示
        plt.style.use('dark_background')
        fig, ax = plt.subplots()
        
        # 使用苹方字体设置标题
        ax.set_title(f"OV7670 实时视频流 ({args.resolution}) - 按 Ctrl+C 停止"
                    )
        
        # 预先创建空白图像
        initial_img = Image.new('RGB', RESOLUTIONS[args.resolution.lower()])
        display = ax.imshow(initial_img)
        
        # 创建动画（修复参数传递）
        ani = animation.FuncAnimation(
            fig, update_frame, fargs=(display, stream),
            interval=50,  # 降低间隔时间提高帧率
            cache_frame_data=False
        )
        
        plt.show()
        
        if args.save:
            frame = stream.capture_frame()
            if frame:
                filename = f'capture_{args.resolution}_{int(time.time())}.png'
                frame.save(filename)
                print(f"已保存最终帧: {filename}")
                
    except serial.SerialException as e:
        print(f"串口错误: {e}")
    except KeyboardInterrupt:
        print("\n用户中断，退出程序")
    finally:
        try:
            stream.close()
        except:
            pass

if __name__ == "__main__":
    main()