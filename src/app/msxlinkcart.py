import tkinter as tk
from tkinter import filedialog, messagebox
import serial
import serial.tools.list_ports
import os
import time


MAX_FILENAME_LEN = 32
MAX_FILE_SIZE = 32 * 1024  # 32KB
ROM_SIZE = 64 * 1024  # 64KB

class ROMBurnerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("MSXLinkCart烧录工具")
        self.rom1_path = None
        self.rom2_path = None
        self.error = False

        # 顶部区域
        top_frame = tk.Frame(root)
        top_frame.pack(pady=10)

        # 左边 ROM1
        left_frame = tk.Frame(top_frame)
        left_frame.pack(side=tk.LEFT, padx=20, pady=20)
        # 将Label和Button放在同一行
        row_frame = tk.Frame(left_frame, pady=10)
        row_frame.pack()
        tk.Label(row_frame, text="请选择 ROM1").pack(side=tk.LEFT)
        tk.Button(row_frame, text="选择文件", command=self.select_rom1).pack(side=tk.LEFT, padx=5)
        self.rom1_name_var = tk.StringVar()
        self.rom1_entry = tk.Entry(left_frame, textvariable=self.rom1_name_var,  width=28)
        self.rom1_entry.pack()

        # 右边 ROM2
        right_frame = tk.Frame(top_frame)
        right_frame.pack(side=tk.RIGHT, padx=20, pady=20)
        row_frame = tk.Frame(right_frame, pady=10)
        row_frame.pack()
        tk.Label(row_frame, text="请选择 ROM2").pack(side=tk.LEFT)
        tk.Button(row_frame, text="选择文件", command=self.select_rom2).pack(side=tk.LEFT, padx=5)
        self.rom2_name_var = tk.StringVar()
        self.rom2_entry = tk.Entry(right_frame, textvariable=self.rom2_name_var, width=28)
        self.rom2_entry.pack()

        # 中间串口选择
        middle_frame = tk.Frame(root)
        middle_frame.pack(pady=10)
        row_frame = tk.Frame(middle_frame, pady=10)
        row_frame.pack()
        tk.Label(row_frame, text="请选择串口").pack(side=tk.LEFT)
        self.port_var = tk.StringVar()
        self.port_menu = tk.OptionMenu(row_frame, self.port_var, *self.get_serial_ports())
        self.port_menu.pack(side=tk.LEFT)

        # 底部烧录按钮
        bottom_frame = tk.Frame(root)
        bottom_frame.pack(pady=20)
        self.burn_button = tk.Button(bottom_frame, text="烧录", state=tk.DISABLED, command=self.burn)
        self.burn_button.pack()

    def get_serial_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        return ports if ports else ["无可用串口"]

    def update_burn_button_state(self):
        if self.rom1_path or self.rom2_path:
            self.burn_button.config(state=tk.NORMAL)
        else:
            self.burn_button.config(state=tk.DISABLED)

    def select_file(self):
        file_path = filedialog.askopenfilename(filetypes=[("ROM files", "*.rom"), ("IMG files", "*.img"), ("BIN files", "*.bin"),("All files", "*.*")])
        if not file_path:
            return None, None

        if os.path.getsize(file_path) > MAX_FILE_SIZE:
            messagebox.showerror("文件过大", "文件大小不能超过32KB")
            return None, None

        filename = os.path.splitext(os.path.basename(file_path))[0][:MAX_FILENAME_LEN]
        return file_path, filename

    def select_rom1(self):
        path, name = self.select_file()
        if path:
            self.rom1_path = path
            self.rom1_name_var.set(name)
        else:
            self.rom1_path = None
            self.rom1_name_var.set("")
        self.update_burn_button_state()

    def select_rom2(self):
        path, name = self.select_file()
        if path:
            self.rom2_path = path
            self.rom2_name_var.set(name)
        else:
            self.rom2_path = None
            self.rom2_name_var.set("")
        self.update_burn_button_state()

    def burn(self):
        if not self.port_var.get() or self.port_var.get() == "无可用串口":
            messagebox.showerror("错误", "请选择有效串口")
            return

        image = bytearray([0xFF] * ROM_SIZE)

        try:
            if self.rom1_path:
                with open(self.rom1_path, 'rb') as f:
                    data = f.read()
                    first_half = data[:16 * 1024]
                    second_half = data[16 * 1024:]
                    image[0x4000:0x4000 + len(first_half)] = first_half
                    image[0x0000:0x0000 + len(second_half)] = second_half

            if self.rom2_path:
                with open(self.rom2_path, 'rb') as f:
                    data = f.read()
                    first_half = data[:16 * 1024]
                    second_half = data[16 * 1024:]
                    image[0xC000:0xC000 + len(first_half)] = first_half
                    image[0x8000:0x8000 + len(second_half)] = second_half

            # 发送数据
            self.send_data(image)
            if self.error:
                messagebox.showerror("错误", "烧录失败")
            else:
                messagebox.showinfo("完成", "烧录完成")
        except Exception as e:
            messagebox.showerror("错误", f"烧录失败: {e}")
    
    def send_data(self, image):
        try:
            ser = serial.Serial(self.port_var.get(), 115200, timeout=1)
            if not ser.is_open:
                ser.open()
            ser.reset_input_buffer()  # 清空输入缓冲区  
            ser.reset_output_buffer()  # 清空输出缓冲区
            ser.flush()  # 确保缓冲区清空
            time.sleep(3)  # 等待串口稳定

            # 发送指令字符串"W"
            ser.write(b'W')
            time.sleep(0.5) 
            if not self.wait_for_response(ser, b'W'):  # 等待客户端反馈代码W
                ser.close()
                return
            
            self.send_rom_name(ser, self.rom1_name_var.get())  # 发送ROM1名称
            if not self.wait_for_response(ser, b'W'):  # 等待客户端反馈代码W
                ser.close()
                return

            self.send_rom_name(ser, self.rom2_name_var.get())  # 发送ROM2名称
            if not self.wait_for_response(ser, b'W'):  # 等待客户端反馈代码W
                ser.close()
                return

            # 分块发送image内容，每次32字节
            for i in range(0, len(image), 32):
                ser.write(image[i:i+32])
                ser.flush()  # 确保数据发送出去
                if not self.wait_for_response(ser, b'N'):
                    ser.close()
                    return
        except Exception as e:
            self.error = True
            messagebox.showerror("错误", f"发送数据失败: {e}")
        finally:
            if ser.is_open:
                ser.close()
            
    def wait_for_response(self, ser, response_code):
        # 等待客户端反馈代码W
        while True:
            if ser.in_waiting > 0:  # 检查是否有数据可读
                response = ser.read(1)
                break
            time.sleep(0.01)

        if response != response_code:  # response_code检查
            self.error = True
            messagebox.showerror("错误", "未收到正确的反馈代码，请重新插拔MSXLinkCart或检查连接，如果问题依旧存在，请确认您插入的MSXLinkCart是否为最新版本。")
            return False
        return True
    
    def send_rom_name(self, ser, rom_name):
        if not rom_name:
            rom_name = " "
        else:
            rom_name = rom_name[:28]
        ser.write(rom_name.encode('utf-8'))
        ser.flush()
        time.sleep(0.1)

# 主程序入口
if __name__ == "__main__":
    root = tk.Tk()
    app = ROMBurnerApp(root)
    root.mainloop()