# HƯỚNG DẪN ĐẤU NỐI PHẦN CỨNG & RÁP SA BÀN (WIRING DIAGRAM)
## Hệ thống truyền ảnh Bitmap 64x64 Tự thích nghi trên 3x ESP32-C3 SuperMini

---

## 1. Danh sách linh kiện sử dụng
1. **3x Bo mạch ESP32-C3 Super Mini (TENSTAR ROBOT)**
2. **3x Màn hình OLED SSD1306 0.96 inch (I2C: VCC, GND, SCL, SDA)**
3. **1x Biến trở xoay (Potentiometer) 10kΩ**
4. **3x Đèn LED (Xanh lá, Vàng, Đỏ) + 3x Điện trở 220Ω**
5. **1x Nút nhấn (Push button)**
6. **1x Tấm phíp đồng đục lỗ (Protoboard)**
7. **Rào cắm cái (Female header 8-pin) và dây bus câu mạch**

---

## 2. Bảng nối chân chi tiết giữa 3 mạch

| Chân ESP32-C3 | 1. Máy phát (Tx) | 2. Kênh truyền (Channel) | 3. Máy thu (Rx) | Ghi chú đấu nối |
|:---|:---|:---|:---|:---|
| **5V** | Nối nguồn 5V chung | Nối nguồn 5V chung | Nối nguồn 5V chung | Cấp nguồn từ cổng USB hoặc nguồn 5V ngoài |
| **GND** | Nối Mass (GND) chung | Nối Mass (GND) chung | Nối Mass (GND) chung | **Bắt buộc nối chung toàn bộ GND** |
| **3.3V** | Cấp nguồn VCC cho OLED 1 | Cấp VCC cho OLED 2 & chân 1 biến trở | Cấp nguồn VCC cho OLED 3 | Đường nguồn 3.3V ổn áp |
| **GPIO 8 (SDA)** | Nối chân SDA của **OLED 1** | Nối chân SDA của **OLED 2** | Nối chân SDA của **OLED 3** | Bus dữ liệu I2C |
| **GPIO 9 (SCL)** | Nối chân SCL của **OLED 1** | Nối chân SCL của **OLED 2** | Nối chân SCL của **OLED 3** | Bus xung nhịp I2C |
| **GPIO 0 (ADC)** | *(Để trống)* | **Chân giữa Biến trở 10kΩ** | *(Để trống)* | Đọc mức điện áp tương tự để chỉnh BER |
| **GPIO 1 (ADC)** | *(Để trống)* | *(Để trống)* | *(Để trống)* | Dự phòng ADC |
| **GPIO 20** | Nhận Feedback từ **TX0 (GPIO 21)** Kênh | Nhận Data ảnh từ **TX (GPIO 21)** Máy phát | Gửi Feedback sang **RX1 (GPIO 4)** Kênh | Đường UART |
| **GPIO 21** | Bắn Data ảnh sang **RX0 (GPIO 20)** Kênh | Forward Feedback về **RX (GPIO 20)** Máy phát | Nhận Data nhiễu từ **TX1 (GPIO 5)** Kênh | Đường UART |
| **GPIO 5** | *(Để trống)* | **TX1:** Phát Data nhiễu sang **RX (GPIO 20)** Máy thu | **LED RGB: Kênh R (Đỏ)** | Qua trở $220\Omega$, Cathode nối GND |
| **GPIO 4** | *(Để trống)* | **RX1:** Bắt Feedback từ **TX (GPIO 21)** Máy thu | *(Để trống)* | UART1 RX của Kênh |
| **GPIO 10** | Nút nhấn đổi ảnh (chân kia nối GND) | *(Để trống)* | *(Để trống)* | Nút bấm đổi ảnh TX |
| **GPIO 7** | *(Để trống)* | *(Để trống)* | **LED RGB: Kênh B (Xanh dương)** | Qua trở $220\Omega$, Cathode nối GND |
| **GPIO 6** | *(Để trống)* | *(Để trống)* | **LED RGB: Kênh G (Xanh lá)** | Qua trở $220\Omega$, Cathode nối GND |

---

## 3. Sơ đồ nguyên lý luồng dữ liệu 2 chiều

```
                                [ LUỒNG THUẬN: TRUYỀN ẢNH BITMAP 64x64 ]
               GPIO 21 (TX)  ──────────────────────>  GPIO 20 (RX0)
             [ MÁY PHÁT - TX ]                      [ KÊNH TRUYỀN ]  GPIO 5 (TX1) ───> GPIO 20 (RX) [ MÁY THU - RX ]
               GPIO 20 (RX)  <──────────────────────  GPIO 21 (TX0)                   GPIO 21 (TX) ───> GPIO 4  (RX1)
                                [ LUỒNG NGƯỢC: FEEDBACK CHẾ ĐỘ & BER ]
```

---

## 4. Bố trí sa bàn trên Phíp đồng đục lỗ (Top View)

```text
       Cột: A  B  C  D  E  F   G   H  I  J  K  L  M   N   O  P  Q  R  S  T   U
Hàng 18:   [=== Cổng USB ===]     [=== Cổng USB ===]     [=== Cổng USB ===]
Hàng 17:   . [5V]         [5 ] .   . [5V]         [5 ] .   . [5V]         [5 ] .
Hàng 16:   . [G ]         [6 ] .   . [G ]         [6 ] .   . [G ]         [6 ] . (LED Xanh)
Hàng 15:   . [3V]  ESP1   [7 ] .   . [3V]  ESP2   [7 ] .   . [3V]  ESP3   [7 ] . (LED Vàng)
Hàng 14:   . [4 ]  (TX)   [8 ] .   . [4 ]  (CH)   [8 ] .   . [4 ]  (RX)   [8 ] .
Hàng 13:   . [3 ]         [9 ] .   . [3 ]         [9 ] .   . [3 ]         [9 ] .
Hàng 12:   . [2 ]         [10] .   . [2 ]         [10] .   . [2 ]         [10] . (LED Đỏ)
Hàng 11:   . [1 ]         [20] .   . [1 ]         [20] .   . [1 ]         [20] .
Hàng 10:   . [0 ]         [21] .   . [0 ]         [21] .   . [0 ]         [21] .
           ---------------------------------------------------------------------
Hàng 08:       [ OLED 1 - TX ]        [ OLED 2 - CH ]        [ OLED 3 - RX ]
Hàng 07:       [  Ảnh gốc    ]        [ Thanh đo BER]        [ Ảnh sửa lỗi ]
           ---------------------------------------------------------------------
Hàng 03:     [ Nút đổi ảnh ]        [ Biến trở 10k ]       [ 3 LED Xanh/Vàng/Đỏ ]
```
