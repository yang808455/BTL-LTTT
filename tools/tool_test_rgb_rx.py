# -*- coding: utf-8 -*-
"""
Công cụ kiểm tra LED RGB trên bo ESP32-C3 Máy thu (Node 3 - Rx).
Đấu nối phần cứng:
  - GPIO 5: Chân ĐỎ (Red)        - Qua trở 220Ω
  - GPIO 6: Chân XANH LÁ (Green)  - Qua trở 220Ω
  - GPIO 7: Chân XANH DƯƠNG (Blue)- Qua trở 220Ω
  - Chân chung (Cathode -): Nối vào GND bo Máy thu.
Độ sáng: Khống chế cố định 1% (PWM Duty tối đa 3 / 255).
"""

import sys
import time
import json
import serial
import serial.tools.list_ports

DEFAULT_PORT = "COM11"
DEFAULT_BAUD = 115200

class RGBTester:
    def __init__(self, port_name=DEFAULT_PORT, baudrate=DEFAULT_BAUD):
        self.port_name = port_name
        self.baudrate = baudrate
        self.ser = None

    def connect(self):
        try:
            self.ser = serial.Serial(self.port_name, self.baudrate, timeout=0.5, dsrdtr=False, rtscts=False)
            self.ser.dtr = False
            self.ser.rts = False
            time.sleep(1.2)  # Chờ ESP32 ổn định
            self.ser.reset_input_buffer()
            print(f"[OK] Đã kết nối thành công tới bo Rx qua cổng {self.port_name} ({self.baudrate} bps).")
            return True
        except Exception as e:
            print(f"[LỖI] Không thể kết nối tới {self.port_name}: {e}")
            return False

    def close(self):
        if self.ser and self.ser.is_open:
            try:
                # Trả lại quyền điều khiển LED cho hệ thống
                self.ser.write(b'{"cmd":"RELEASE_LED"}\n')
                self.ser.flush()
                time.sleep(0.1)
            except:
                pass
            self.ser.close()
            print("[INFO] Đã đóng cổng Serial và trả lại quyền điều khiển cho hệ thống.")

    def send_rgb(self, r, g, b, pct=1):
        """Gửi lệnh PWM bật LED RGB (độ sáng mặc định 1%)."""
        if not self.ser or not self.ser.is_open:
            return ""

        cmd = {
            "cmd": "SET_RGB",
            "r": int(r),
            "g": int(g),
            "b": int(b),
            "pct": int(pct)
        }
        cmd_str = json.dumps(cmd) + "\n"
        self.ser.write(cmd_str.encode('ascii'))
        self.ser.flush()

        start = time.time()
        while time.time() - start < 0.6:
            line = self.ser.readline().decode('utf-8', errors='ignore').strip()
            if "RGB_SET" in line:
                return line
        return ""

    def send_raw_digital(self, r, g, b):
        """Kích trực tiếp mức HIGH/LOW (1 hoặc 0) qua lệnh SET_RAW."""
        if not self.ser or not self.ser.is_open:
            return ""
        cmd = f'{{"cmd":"SET_RAW","r":{r},"g":{g},"b":{b}}}\n'
        self.ser.write(cmd.encode('ascii'))
        self.ser.flush()

        start = time.time()
        while time.time() - start < 0.6:
            line = self.ser.readline().decode('utf-8', errors='ignore').strip()
            if "RAW_SET" in line:
                return line
        return ""

    def run_auto_cycle(self):
        """Chạy chu kỳ kiểm tra tự động lần lượt các màu (độ sáng 1%)."""
        sequence = [
            ("ĐỎ (GPIO 5)", 255, 0, 0),
            ("XANH LÁ (GPIO 6)", 0, 255, 0),
            ("XANH DƯƠNG (GPIO 7)", 0, 0, 255),
            ("VÀNG (Đỏ + Xanh lá)", 255, 255, 0),
            ("TÍM (Đỏ + Xanh dương)", 255, 0, 255),
            ("XANH LƠ (Xanh lá + Xanh dương)", 0, 255, 255),
            ("TRẮNG (Cả 3 kênh)", 255, 255, 255),
            ("TẮT LED", 0, 0, 0)
        ]

        print("\n" + "=" * 60)
        print("  BẮT ĐẦU CHU KỲ TEST TỰ ĐỘNG LED RGB (ĐỘ SÁNG 1%)")
        print("=" * 60)
        print("Mỗi màu sẽ sáng trong 1.5 giây...\n")

        try:
            for name, r, g, b in sequence:
                print(f"-> Đang bật: {name:30s} (PWM 1% - Duty: 3/255)")
                resp = self.send_rgb(r, g, b, pct=1)
                time.sleep(1.5)
            print("\n[OK] Hoàn thành chu kỳ kiểm tra tự động!")
        except KeyboardInterrupt:
            print("\n[!] Đã dừng chu kỳ test.")
            self.send_rgb(0, 0, 0)

def scan_ports():
    ports = serial.tools.list_ports.comports()
    return [p.device for p in ports]

def main():
    print("=" * 65)
    print("      CÔNG CỤ KIỂM TRA ĐÈN LED RGB BO MÁY THU (RX - COM11)")
    print("      - Chân Đỏ (R):        GPIO 5 (Qua trở 220Ω -> GND)")
    print("      - Chân Xanh lá (G):   GPIO 6 (Qua trở 220Ω -> GND)")
    print("      - Chân Xanh dương (B):GPIO 7 (Qua trở 220Ω -> GND)")
    print("      Độ sáng khống chế: Cố định 1% (Duty tối đa 3/255)")
    print("=" * 65)

    avail_ports = scan_ports()
    print(f"Các cổng COM đang kết nối: {avail_ports}")

    port = DEFAULT_PORT
    if port not in avail_ports and avail_ports:
        port = avail_ports[0]

    choice_port = input(f"Nhập cổng COM của bo Rx [Mặc định: {port}]: ").strip()
    if choice_port:
        port = choice_port.upper()

    tester = RGBTester(port)
    if not tester.connect():
        input("\nNhấn Enter để thoát...")
        sys.exit(1)

    try:
        while True:
            print("\n" + "-" * 55)
            print(" MENU ĐIỀU KHIỂN LED RGB (ĐỘ SÁNG 1%)")
            print("-" * 55)
            print("  1. TỰ ĐỘNG CHẠY CHU KỲ TEST (Đỏ -> Xanh lá -> Xanh dương...)")
            print("  2. Bật riêng màu ĐỎ         (GPIO 5) - Độ sáng 1%")
            print("  3. Bật riêng màu XANH LÁ    (GPIO 6) - Độ sáng 1%")
            print("  4. Bật riêng màu XANH DƯƠNG (GPIO 7) - Độ sáng 1%")
            print("  5. Bật màu TRẮNG            (GPIO 5+6+7) - Độ sáng 1%")
            print("  6. TẮT LED HOÀN TOÀN")
            print("  7. Test cưỡng bức Digital (100% HIGH 3.3V)")
            print("  0. Thoát chương trình (Trả quyền điều khiển cho hệ thống)")
            print("-" * 55)
            cmd = input("Chọn thao tác (0-7): ").strip()

            if cmd == "1":
                tester.run_auto_cycle()
            elif cmd == "2":
                print("-> Bật ĐỎ (GPIO 5) - 1%...")
                resp = tester.send_rgb(255, 0, 0, pct=1)
                if resp: print(f"   Rx phản hồi: {resp}")
            elif cmd == "3":
                print("-> Bật XANH LÁ (GPIO 6) - 1%...")
                resp = tester.send_rgb(0, 255, 0, pct=1)
                if resp: print(f"   Rx phản hồi: {resp}")
            elif cmd == "4":
                print("-> Bật XANH DƯƠNG (GPIO 7) - 1%...")
                resp = tester.send_rgb(0, 0, 255, pct=1)
                if resp: print(f"   Rx phản hồi: {resp}")
            elif cmd == "5":
                print("-> Bật TRẮNG (GPIO 5+6+7) - 1%...")
                resp = tester.send_rgb(255, 255, 255, pct=1)
                if resp: print(f"   Rx phản hồi: {resp}")
            elif cmd == "6":
                print("-> Tắt LED...")
                resp = tester.send_rgb(0, 0, 0)
                if resp: print(f"   Rx phản hồi: {resp}")
            elif cmd == "7":
                print("-> Kích thẳng 100% HIGH (3.3V) lên cả 3 chân 5, 6, 7...")
                resp = tester.send_raw_digital(1, 1, 1)
                if resp: print(f"   Rx phản hồi: {resp}")
            elif cmd == "0":
                print("Đang tắt đèn và thoát...")
                break
            else:
                print("[!] Lựa chọn không hợp lệ, vui lòng chọn lại.")

    finally:
        tester.close()
        print("[Xong] Đã dừng chương trình kiểm tra LED.")

if __name__ == "__main__":
    main()
