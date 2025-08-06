#!/usr/bin/env python3
"""
用于控制RP2040 OV7670摄像头的简单脚本
"""

import serial
import time
import sys
import argparse

def send_stop_command(port, baudrate):
    """发送停止命令到设备"""
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud")
        time.sleep(2)
        
        # 发送停止命令 (0x00)
        stop_command = bytes([0x00])
        ser.write(stop_command)
        print("Stop command sent")
        
        ser.close()
        print("Connection closed")
        return True
        
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return False
    except Exception as e:
        print(f"An error occurred: {e}")
        return False

def send_restart_command(port, baudrate):
    """发送重启命令到设备"""
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud")
        time.sleep(2)
        
        # 发送任意字符重启流模式
        ser.write(b'r')
        print("Restart command sent")
        
        ser.close()
        print("Connection closed")
        return True
        
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        return False
    except Exception as e:
        print(f"An error occurred: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description="Control RP2040 OV7670 Camera")
    parser.add_argument("--port", "-p", default="/dev/tty.usbserial-0001", 
                        help="Serial port (default: /dev/tty.usbserial-0001)")
    parser.add_argument("--baudrate", "-b", type=int, default=1500000,
                        help="Baud rate (default: 1500000)")
    parser.add_argument("action", choices=["stop", "restart"], 
                        help="Action to perform")
    
    args = parser.parse_args()
    
    if args.action == "stop":
        success = send_stop_command(args.port, args.baudrate)
    elif args.action == "restart":
        success = send_restart_command(args.port, args.baudrate)
    
    if success:
        print(f"Successfully executed {args.action} command")
        sys.exit(0)
    else:
        print(f"Failed to execute {args.action} command")
        sys.exit(1)

if __name__ == "__main__":
    main()