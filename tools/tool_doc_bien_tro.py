#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Công cụ đọc và giám sát giá trị biến trở thời gian thực từ Node 2 (Kênh truyền ESP32-C3)
Dự án: Hệ thống truyền ảnh Bitmap 64x64 Tự thích nghi
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

def find_channel_port():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None
    # Uu tien COM10 neu co
    for p in ports:
        if p.device == 'COM10':
            return 'COM10'
    # Neu khong co COM10, chon cong USB Serial dau tien
    for p in ports:
        if "USB" in p.description or "Serial" in p.description:
            return p.device
    return ports[0].device

def get_mode_name(ber_val):
    if ber_val < 0.01:
        return "\033[92mDIRECT (Kenh tot, BER < 1%)\033[0m"
    elif ber_val <= 0.04:
        return "\033[93mHAMMING (8,4) (Nhiễu vua, 1% <= BER <= 4%)\033[0m"
    else:
        return "\033[91mCYCLIC (7,4) (Nhiễu nang, BER > 4%)\033[0m"

def draw_bar(ber_val, width=30):
    ratio = min(max(ber_val / 0.12, 0.0), 1.0)
    filled = int(ratio * width)
    bar = "█" * filled + "░" * (width - filled)
    return f"[{bar}]"

def main():
    # Kich hoat mau ANSI tren Windows Terminal
    os.system("")
    
    port = sys.argv[1] if len(sys.argv) > 1 else find_channel_port()
    if not port:
        print("[LOI] Khong tim thay cong COM nao duoc ket noi!")
        return

    baudrate = 115200
    print("=" * 68)
    print("   CONG CU GIAM SAT BIEN TRO & BER THOI GIAN THUC (NODE 2 - CHANNEL)")
    print("=" * 68)
    print(f"[*] Dang ket noi cong: {port} (Baud: {baudrate})...")

    try:
        ser = serial.Serial(port, baudrate, timeout=1.0)
        time.sleep(0.5)
        # Xoa bo dem cu
        ser.reset_input_buffer()
        print("[+] Ket noi thanh cong! Nhan Ctrl + C de dung va xem thong ke.\n")
    except Exception as e:
        print(f"[LOI] Khong the mo cong {port}: {e}")
        return

    sample_count = 0
    pots = []
    bers = []

    try:
        while True:
            raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not raw_line or not raw_line.startswith('{'):
                continue

            try:
                data = json.loads(raw_line)
            except json.JSONDecodeError:
                continue

            pot_val = None
            ber_val = None

            if data.get("node") == "CH":
                if "pot" in data and "ber" in data:
                    pot_val = int(data["pot"])
                    ber_val = float(data["ber"])
                elif data.get("diag") == "ADC" and "raw" in data and "ber" in data:
                    pot_val = int(data["raw"])
                    ber_val = float(data["ber"])

            if pot_val is not None and ber_val is not None:
                sample_count += 1
                pots.append(pot_val)
                bers.append(ber_val)

                # Giu lai 200 mau gan nhat cho thong ke
                if len(pots) > 200:
                    pots.pop(0)
                    bers.pop(0)

                ber_pct = ber_val * 100.0
                bar = draw_bar(ber_val, width=25)
                mode_str = get_mode_name(ber_val)
                flipped = data.get("flipped", 0)

                # In tren mot dong co ghi de (carriage return)
                sys.stdout.write(
                    f"\r\033[K[#{sample_count:04d}] "
                    f"ADC: \033[1;36m{pot_val:4d}\033[0m/4095 | "
                    f"BER: \033[1;33m{ber_pct:5.2f}%\033[0m {bar} | "
                    f"Flip: {flipped:2d}b | {mode_str}"
                )
                sys.stdout.flush()

    except KeyboardInterrupt:
        print("\n\n" + "=" * 68)
        print("                   THONG KE DU LIEU THU THAP")
        print("=" * 68)
        if pots:
            min_p, max_p = min(pots), max(pots)
            avg_p = sum(pots) / len(pots)
            delta_p = max_p - min_p
            print(f"- Tong so mau ghi nhan : {sample_count}")
            print(f"- Gia tri ADC (pot)    : Min = {min_p} | Max = {max_p} | Trung binh = {avg_p:.1f}")
            print(f"- Do dao dong (Max-Min): {delta_p} nac ADC ({delta_p/40.95:.1f}% dai do)")
            print(f"- Ty le loi (BER)      : Min = {min(bers)*100:.2f}% | Max = {max(bers)*100:.2f}%")
        else:
            print("- Chua nhan duoc goi tin phu hop nao.")
        print("=" * 68)
    finally:
        ser.close()
        print("[*] Da dong cong COM.")

if __name__ == "__main__":
    main()
