# HƯỚNG DẪN VẬN HÀNH & KIỂM THỬ HỆ THỐNG
## 3x ESP32-C3 SuperMini Digital Communication System

---

## 1. CÀI ĐẶT MÔI TRƯỜNG TRÊN MÁY TÍNH (PC)

### 1.1. Cài đặt thư viện Python cho Web Dashboards
Mở PowerShell tại thư mục dự án và cài đặt:
```powershell
pip install fastapi uvicorn pyserial websockets
```

---

## 2. NẠP FIRMWARE CHO 3 BO MẠCH ESP32-C3 SUPERMINI

Dự án đã cấu hình sẵn file `platformio.ini` cho từng bo mạch với cờ `ARDUINO_USB_CDC_ON_BOOT=1` (để cổng USB Type-C hoạt động như cổng COM Serial thực tế).

### 2.1. Nạp cho Node 1 (Máy phát):
* Cắm cáp USB của Node 1 vào máy tính.
* Mở terminal tại thư mục `01_transmitter/` và chạy lệnh PlatformIO:
```powershell
pio run -t upload
```

### 2.2. Nạp cho Node 2 (Kênh truyền):
* Cắm cáp USB của Node 2 vào máy tính.
* Mở terminal tại thư mục `02_channel/`:
```powershell
pio run -t upload
```

### 2.3. Nạp cho Node 3 (Máy thu):
* Cắm cáp USB của Node 3 vào máy tính.
* Mở terminal tại thư mục `03_receiver/`:
```powershell
pio run -t upload
```

---

## 3. KHỞI CHẠY 3 WEB DASHBOARD TRÊN PC

Mở 3 cửa sổ Terminal/PowerShell độc lập để chạy 3 ứng dụng tương ứng với 3 cổng Port:

* **Cửa sổ 1 - Chạy Dashboard Máy phát (Port 8001):**
  ```powershell
  cd out/esp32_c3_comm_system/01_transmitter
  python app_tx.py
  ```
  👉 Truy cập trình duyệt: **`http://localhost:8001`**

* **Cửa sổ 2 - Chạy Dashboard Kênh truyền (Port 8002):**
  ```powershell
  cd out/esp32_c3_comm_system/02_channel
  python app_channel.py
  ```
  👉 Truy cập trình duyệt: **`http://localhost:8002`**

* **Cửa sổ 3 - Chạy Dashboard Máy thu (Port 8003):**
  ```powershell
  cd out/esp32_c3_comm_system/03_receiver
  python app_rx.py
  ```
  👉 Truy cập trình duyệt: **`http://localhost:8003`**

---

## 4. KỊCH BẢN THÍ NGHIỆM ĐỀ XUẤT ĐỂ DEMO & NGHIÊN CỨU

### 🔹 Kịch bản 1: Kênh truyền lý tưởng (BER = 0%)
1. Trên Dashboard Node 2 (Port 8002): Kéo thanh trượt BER về `0.0%`.
2. Trên Dashboard Node 1 (Port 8001): Chọn mã nguồn là **Huffman**, mã kênh là **Hamming (8,4)** $\to$ Bấm **"Gửi 1 gói tin"**.
3. Quan sát:
   * **Node 1:** Hiển thị luồng bit nén, tỷ số nén $CR \approx 1.5 - 2.0x$.
   * **Node 2:** Số bit bị đảo = 0.
   * **Node 3 (Port 8003):** Huy hiệu xanh `CRC16 MATCH`, văn bản khôi phục 100% nguyên vẹn.

### 🔹 Kịch bản 2: Kênh có nhiễu (BER = 1.0%) – Trình diễn khả năng sửa lỗi của Systematic Hamming (8,4)
1. Trên Dashboard Node 2: Kéo thanh trượt BER lên mức chuẩn `1.0%` (hoặc `1.5%`).
2. Trên Dashboard Node 1: Phát gói tin.
3. Quan sát:
   * **Node 2:** Báo cáo có 1 đến 2 bit trong Payload đã bị đảo (nhìn thấy rõ ở dòng Hex đỏ).
   * **Node 3:** Báo cáo: `Đã sửa thành công X bit lỗi qua ma trận Syndrome!`.
   * **Kết quả:** Văn bản khôi phục **vẫn hoàn toàn chính xác**, minh chứng rõ nét cho vai trò của tầng mã hóa kênh hệ thống!

### 🔹 Kịch bản 3: So sánh hiệu suất nén giữa 3 thuật toán nguồn
1. Giữ nguyên chuỗi văn bản mẫu.
2. Lần lượt đổi giữa:
   * **Huffman Coding:** Nén tối ưu cho ký tự đơn lẻ.
   * **Arithmetic Coding:** Nén tối ưu theo dải xác suất tích lũy.
   * **LZW Coding:** Nén cực tốt cho dữ liệu lặp lại chu kỳ (bấm nút "Mẫu Lặp (LZW)").
3. So sánh trực tiếp thông số **Tỷ số nén (Compression Ratio - CR)** trên Dashboard Máy phát và Máy thu.
