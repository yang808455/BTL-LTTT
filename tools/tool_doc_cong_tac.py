#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Công cụ đọc và giám sát trạng thái Công tắc gạt 3 nấc (Baudrate Switch) thời gian thực
Hỗ trợ tự động nhận diện và đọc từ BẤT KỲ node nào đang cắm cáp USB (TX, CH hoặc RX).
Dự án: Hệ thống truyền ảnh Bitmap 64x64 Tự thích nghi ESP32-C3
"""

import sys
import time
import json
import os

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[LOI] Chua cai dat thu vien 'pyserial'. Vui long chay: pip install pyserial")
    sys.exit(1)

def find_active_port():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None
    # Uu tien cac cong quen thuoc cua du an neu co
    for target in ('COM11', 'COM10', 'COM9'):
        for p in ports:
            if p.device == target:
                return target
    # Neu khong, chon cong USB Serial dau tien tim thay
    for p in ports:
        if "USB" in p.description or "Serial" in p.description:
            return p.device
    return ports[0].device

def get_baud_visual(baud):
    if baud == 9600:
        return (
            "Nấc 1: GẠT TRÁI",
            "GPIO 2 = 0, GPIO 3 = 1",
            "\033[1;31m  9600 bps (CHẬM - Quan sát từng mảnh)\033[0m",
            "[🔴━━━━⚪━━━━⚪]"
        )
    elif baud == 38400:
        return (
            "Nấc 2: Ở GIỮA",
            "GPIO 2 = 1, GPIO 3 = 1",
            "\033[1;33m 38400 bps (TRUNG BÌNH)\033[0m",
            "[⚪━━━━🟡━━━━⚪]"
        )
    elif baud == 115200:
        return (
            "Nấc 3: GẠT PHẢI",
            "GPIO 2 = 1, GPIO 3 = 0",
            "\033[1;32m115200 bps (CAO NHẤT - Siêu mượt)\033[0m",
            "[⚪━━━━⚪━━━━🟢]"
        )
    else:
        return (
            "Không xác định",
            "---",
            f"{baud} bps",
            "[⚪━━━━⚪━━━━⚪]"
        )

def main():
    # Kich hoat che do mau ANSI tren Windows Terminal
    os.system("")

    port = sys.argv[1] if len(sys.argv) > 1 else find_active_port()
    if not port:
        print("[LOI] Khong tim thay bat ky cong COM nao duoc ket noi vao may tinh!")
        return

    baudrate = 115200
    print("=" * 72)
    print("      CÔNG CỤ GIÁM SÁT CÔNG TẮC 3 NẤC CHUYỂN BAUDRATE THỜI GIAN THỰC")
    print("       (Tuong thich 100% voi bat ky Node nao dang cam USB: TX, CH, RX)")
    print("=" * 72)
    print(f"[*] Dang ket noi cong COM: {port} (Baudrate USB CDC: {baudrate})...")

    try:
        ser = serial.Serial(port, baudrate, timeout=1.0)
        time.sleep(0.4)
        ser.reset_input_buffer()
        print("[+] Ket noi thanh cong! Hay gat cong tac 3 nac tren sa ban de xem ket qua.")
        print("[+] Nhan Ctrl + C de dung chuong trinh va xem lich su gat cong tac.\n")
    except Exception as e:
        print(f"[LOI] Khong the mo cong {port}: {e}")
        return

    current_baud = None
    node_name = "Chưa rõ"
    switch_history = []
    start_time = time.time()

    try:
        while True:
            raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not raw_line or not raw_line.startswith('{'):
                continue

            try:
                data = json.loads(raw_line)
            except json.JSONDecodeError:
                continue

            # Nhan dien Node dang noi chuyen
            if "node" in data:
                node_code = data["node"]
                node_map = {"TX": "Máy Phát (Tx)", "CH": "Kênh Truyền (Channel)", "RX": "Máy Thu (Rx)"}
                node_name = node_map.get(node_code, node_code)

            # Bat su kien chuyen Baudrate tuc thi
            if data.get("event") == "BAUD_CHANGE" and "baud" in data:
                new_baud = int(data["baud"])
                t_str = time.strftime("%H:%M:%S")
                pos_name, pin_state, baud_str, visual = get_baud_visual(new_baud)
                
                print(f"\n\a[🔔 SỰ KIỆN GẠT CÔNG TẮC lúc {t_str}]")
                print(f"  • Node phat hien : \033[1;36m{node_name}\033[0m")
                print(f"  • Vi tri gat     : \033[1m{pos_name}\033[0m  {visual}")
                print(f"  • Trang thai chan: {pin_state}")
                print(f"  • Toc do moi     : {baud_str}")
                print("-" * 72)
                
                current_baud = new_baud
                switch_history.append((t_str, pos_name, new_baud))

            # Hoac tu cac thong so khac neu chua co su kien
            if current_baud is None:
                # Mặc định lúc khởi động hiển thị trạng thái chờ hoặc trạng thái hiện tại
                current_baud = 115200

            # Dong trang thai dong (cap nhat lien tuc)
            pos_name, pin_state, baud_str, visual = get_baud_visual(current_baud)
            sys.stdout.write(
                f"\r\033[K[Dang ket noi: {node_name}] "
                f"Vi tri: \033[1m{pos_name}\033[0m {visual} | "
                f"Baud: {baud_str}"
            )
            sys.stdout.flush()

    except KeyboardInterrupt:
        print("\n\n" + "=" * 72)
        print("                   LỊCH SỬ GẠT CÔNG TẮC ĐÃ GHI NHẬN")
        print("=" * 72)
        if switch_history:
            print(f"Tong so lan gat cong tac: {len(switch_history)}")
            for idx, (t, pos, b) in enumerate(switch_history, 1):
                print(f"  #{idx:02d} | Luc {t} | {pos:20s} | Baudrate: {b} bps")
        else:
            print("Chua ghi nhan lan gat cong tac nao trong phien lam viec nay.")
            print("(Cong tac dang giu nguyen o nac hien tai).")
        print("=" * 72)
    finally:
        ser.close()
        print("[*] Da dong cong COM an toan.")

if __name__ == "__main__":
    main()
