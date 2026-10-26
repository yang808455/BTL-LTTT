# HỆ THỐNG TRUYỀN ẢNH BITMAP 64x64 TỰ THÍCH NGHI QUA KÊNH NHIỄU (ADAPTIVE IMAGE TRANSMISSION)
## Dự án Sa bàn phần cứng độc lập trên 3x ESP32-C3 SuperMini với 3 màn hình OLED & Biến trở điều khiển

---

## 🌟 Giới thiệu tổng quan
Dự án là một hệ thống truyền thông tin số hoàn chỉnh theo kiến trúc Shannon, được đóng gói độc lập trên sa bàn phần cứng:
1. **Máy phát (Node 1 - Tx):** Nguồn ảnh Bitmap 64x64 $\to$ Nén nguồn Huffman $\to$ Mã hóa kênh tự thích nghi $\to$ Đóng khung UART $\to$ Hiển thị OLED 1.
2. **Kênh truyền (Node 2 - Channel):** Nhận khung $\to$ Đọc biến trở ADC chỉnh mức BER thời gian thực $\to$ Đảo bit giả lập nhiễu vật lý BSC $\to$ Hiển thị OLED 2 $\to$ Chuyển tiếp 2 chiều (Data xuôi và Feedback ngược).
3. **Máy thu (Node 3 - Rx):** Nhận khung $\to$ Giải mã kênh hệ thống (Syndrome decoding, tự sửa lỗi 1-bit) $\to$ Giải nén Huffman $\to$ Tái tạo ảnh Bitmap 64x64 lên OLED 3 $\to$ Điều khiển 3 LED Mode (Xanh/Vàng/Đỏ) $\to$ Gửi Feedback ngược về Máy phát.

---

## 📂 Cấu trúc Thư mục

```text
out/esp32_c3_adaptive_image_system/
├── KE_HOACH_TRIEN_KHAI_DIEM_10.md    # Kế hoạch tổng thể & đặc tả chi tiết
├── README.md                         # Hướng dẫn tổng quan & thao tác nhanh
│
├── shared/                           # ==================== [DÙNG CHUNG (SHARED)] ====================
│   ├── protocol_defs.h               # Giao thức đóng khung UART (Data 0x7E, Feedback 0x7C)
│   ├── sample_images.h               # Mảng ảnh Bitmap 64x64 mẫu (Smile, Điểm 10) trong PROGMEM
│   ├── crc16.h                       # Checksum CRC16 CCITT
│   └── crc16.c
│
├── app_dashboard.py                  # 🚀 App Python GUI Desktop (PyQt6) quản lý đồng thời 3 khối
├── chay_app_dashboard.bat            # Phím tắt click đúp để mở App Dashboard
│
├── 01_transmitter/                   # ==================== [1. MÁY PHÁT - TX] ====================
│   ├── platformio.ini                # Cấu hình ESP32-C3 + U8g2 OLED
│   ├── include/tx_config.h           # Phân chân phần cứng (SDA: 8, SCL: 9, TX: 21, RX: 20, BTN: 10)
│   ├── src/main.cpp                  # Firmware chính: OLED 1, nén Huffman, mã hóa thích nghi, đổi ảnh
│   └── ...                           # Các module thuật toán nén và mã hóa
│
├── 02_channel/                       # ==================== [2. KÊNH TRUYỀN - CH] =================
│   ├── platformio.ini                # Cấu hình ESP32-C3 + U8g2 OLED
│   ├── include/channel_config.h      # Phân chân phần cứng (ADC Biến trở: GPIO 0, 2 cổng UART)
│   ├── src/main.cpp                  # Firmware chính: Đọc biến trở lọc số, OLED 2, đảo bit BSC, định tuyến 2 chiều
│   └── ...                           # Bộ sinh nhiễu BSC
│
├── 03_receiver/                      # ==================== [3. MÁY THU - RX] =====================
│   ├── platformio.ini                # Cấu hình ESP32-C3 + U8g2 OLED
│   ├── include/rx_config.h           # Phân chân phần cứng (LED: 6, 7, 10; OLED: 8, 9; UART: 20, 21)
│   ├── src/main.cpp                  # Firmware chính: OLED 3, 3 LED Mode, sửa lỗi Syndrome, Feedback
│   └── ...                           # Các module giải mã kênh và giải nén
│
└── docs/                             # ==================== [TÀI LIỆU HƯỚNG DẪN] ===================
    ├── wiring_diagram.md             # Sơ đồ câu dây chi tiết trên tấm phíp đồng đục lỗ
    └── demo_script_diem_10.md        # Kịch bản thuyết trình và câu hỏi vấn đáp đạt điểm 10
```

---

## ⚡ Hướng dẫn Nạp Code & Vận hành

### 1. Nạp Firmware cho 3 bo mạch qua cáp USB Type-C
Mở terminal tại thư mục dự án và chạy các lệnh PlatformIO:
```bash
# 1. Nạp cho Máy phát
cd 01_transmitter
pio run -t upload

# 2. Nạp cho Kênh truyền
cd ../02_channel
pio run -t upload

# 3. Nạp cho Máy thu
cd ../03_receiver
pio run -t upload
```

### 2. Sử dụng Ứng dụng Quản lý Giao diện Python Desktop (PyQt6 Dashboard)
Ứng dụng cho phép kết nối song song cả 3 cổng COM (hoặc click nút `⚡ Tự Động Quét & Nhận Diện 3 Bo`), hiển thị trực quan:
- **Cột Máy phát:** Xem ảnh 64x64 gốc, nút chọn ảnh (Mặt cười / Điểm 10), cưỡng bức chế độ mã hóa, chỉ số nén.
- **Cột Kênh truyền:** Đồng hồ BER %, thanh thước đo, biểu đồ đường Matplotlib vẽ BER thời gian thực, số bit bị đảo, thanh trượt can thiệp mức nhiễu hoặc trả về biến trở.
- **Cột Máy thu:** Tái tạo ảnh 64x64 theo từng mảnh (chunk) cập nhật tức thì, 3 đèn LED ảo (Xanh - Vàng - Đỏ), số bit đã sửa, BER đo được.
- **Bảng Log & Xuất CSV:** Lưu và xem toàn bộ chuỗi gói tin Telemetry với dấu thời gian mili-giây, hỗ trợ xuất file CSV để bảo vệ đồ án.

Cách chạy app:
```bash
python app_dashboard.py
# hoặc click đúp vào file chay_app_dashboard.bat
```

### 3. Thao tác Demo trực quan
1. Cắm nguồn USB cho toàn bộ sa bàn. Cả 3 màn hình OLED sẽ sáng lên hiển thị giao diện riêng biệt.
2. Vặn biến trở ở mức $0\%$: Màn hình Kênh báo $0.0\%$, Máy thu sáng LED Xanh (`MODE: DIRECT`), ảnh mặt cười nhận được sắc nét.
3. Vặn biến trở lên mức $2\% - 4\%$: Màn hình Kênh báo nhiễu tăng, Máy thu lập tức bật LED Vàng (`MODE: HAM (7,4)`), số bit đã sửa (`Fix:`) tăng lên liên tục, ảnh nhận vẫn nguyên vẹn không bị vỡ hạt.
4. Vặn biến trở lên $> 5\%$: Máy thu bật LED Đỏ (`MODE: CYC (7,4)`).
5. Bấm nút nhấn trên Máy phát (GPIO 10): Ảnh chuyển tức thì sang biểu tượng "Điểm 10", và Máy thu lập tức giải mã cập nhật ảnh Điểm 10 theo thời gian thực!
