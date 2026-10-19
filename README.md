# BÀI TẬP LỚN LÝ THUYẾT THÔNG TIN: MÔ PHỎNG & HIỆN THỰC HỆ THỐNG TRUYỀN THÔNG TIN SỐ
## Triển khai trên 3x ESP32-C3 SuperMini với 3 Web Dashboard độc lập

Dự án hiện thực một mô hình truyền tin số hoàn chỉnh theo kiến trúc Shannon:
1. **Máy phát (Node 1 - Tx):** Nén dữ liệu nguồn (Huffman / Arithmetic / LZW) $\to$ Mã hóa kênh hệ thống (Systematic Hamming (8,4) SEC-DED / Systematic Cyclic (7,4)) $\to$ Đóng khung UART.
2. **Kênh truyền (Node 2 - Channel):** Giả lập môi trường vật lý có nhiễu (Binary Symmetric Channel - BSC), đảo bit theo xác suất BER mong muốn.
3. **Máy thu (Node 3 - Rx):** Bắt khung & đồng bộ UART $\to$ Giải mã kênh hệ thống (dò Syndrome, tự sửa lỗi 1-bit, phát hiện lỗi kép) $\to$ Giải nén nguồn $\to$ Khôi phục thông điệp gốc & Tính toán metrics (BER, FER, Tỷ số nén, Tỷ lệ sửa lỗi thành công).

---

## 📂 Cấu trúc Repository

```text
.
├── WORKFLOW.md                             # Bản đặc tả thiết kế, kiến trúc & kế hoạch hành động
│
├── 01_transmitter/                         # ==================== [1. MÁY PHÁT - TX] ====================
│   ├── include/                            # Thư viện mã nguồn C/C++: Huffman, Arithmetic, LZW, Hamming, Cyclic
│   ├── src/                                # Hiện thực thuật toán
│   ├── platformio.ini                      # Cấu hình nạp firmware ESP32-C3 Tx
│   ├── main_tx.cpp                         # Firmware chính Node 1
│   ├── app_tx.py                           # Web Dashboard FastAPI (Port 8001)
│   ├── requirements.txt
│   └── templates/index.html                # Giao diện Web điều khiển phát, xem/sửa dữ liệu & bitstream
│
├── 02_channel/                             # ==================== [2. KÊNH TRUYỀN - CH] =================
│   ├── include/                            # Bộ sinh nhiễu BSC xorshift32 & Bộ gom khung
│   ├── src/
│   ├── platformio.ini                      # Cấu hình nạp firmware ESP32-C3 Channel
│   ├── main_channel.cpp                    # Firmware chính Node 2
│   ├── app_channel.py                      # Web Dashboard FastAPI (Port 8002)
│   ├── requirements.txt
│   └── templates/index.html                # Giao diện thanh trượt BER, so sánh bit diff trực quan
│
├── 03_receiver/                            # ==================== [3. MÁY THU - RX] =====================
│   ├── include/                            # Bộ giải mã Hamming Syndrome, Cyclic, Giải nén nguồn, Metrics
│   ├── src/
│   ├── platformio.ini                      # Cấu hình nạp firmware ESP32-C3 Rx
│   ├── main_rx.cpp                         # Firmware chính Node 3
│   ├── app_rx.py                           # Web Dashboard FastAPI (Port 8003)
│   ├── requirements.txt
│   └── templates/index.html                # Giao diện văn bản khôi phục, đối chuẩn CRC, đồ thị Chart.js
│
├── shared/                                 # Giao thức dùng chung (Header, Frame format, CRC16)
│   ├── protocol_defs.h
│   ├── crc16.h
│   └── crc16.c
│
└── docs/                                   # Tài liệu hướng dẫn
    ├── wiring_diagram.md                   # Sơ đồ câu dây phần cứng chi tiết giữa 3 bo mạch
    └── user_guide.md                       # Hướng dẫn nạp code & các kịch bản thực nghiệm
```

---

## ⚡ Bắt đầu nhanh

### 1. Đấu nối phần cứng
* Nối `GPIO 21` (TX) của Node 1 sang `GPIO 20` (RX) của Node 2.
* Nối `GPIO 21` (TX) của Node 2 sang `GPIO 20` (RX) của Node 3.
* **Nối chung toàn bộ chân `GND`** giữa 3 bo mạch.
* Cắm 3 cáp USB Type-C vào PC. Xem chi tiết tại [`docs/wiring_diagram.md`](docs/wiring_diagram.md).

### 2. Nạp code cho 3 bo mạch
Dùng VS Code PlatformIO hoặc lệnh CLI:
```bash
# Node 1 (Tx)
cd 01_transmitter && pio run -t upload

# Node 2 (Channel)
cd ../02_channel && pio run -t upload

# Node 3 (Rx)
cd ../03_receiver && pio run -t upload
```

### 3. Cài đặt Python & Khởi chạy 3 Web Dashboard
```bash
pip install fastapi uvicorn pyserial websockets
```

Mở 3 terminal riêng biệt:
```bash
# Terminal 1 - Máy phát (Port 8001)
cd 01_transmitter && python app_tx.py

# Terminal 2 - Kênh truyền (Port 8002)
cd 02_channel && python app_channel.py

# Terminal 3 - Máy thu (Port 8003)
cd 03_receiver && python app_rx.py
```
Truy cập trên trình duyệt:
* Máy phát: `http://localhost:8001`
* Kênh truyền: `http://localhost:8002`
* Máy thu: `http://localhost:8003`

Chi tiết xem tại [`docs/user_guide.md`](docs/user_guide.md).
