# HƯỚNG DẪN ĐẤU NỐI PHẦN CỨNG (WIRING DIAGRAM)
## Hệ Thống Truyền Tin Số Trên 3x ESP32-C3 SuperMini

---

### 1. Sơ đồ khối đấu nối liên kết giữa 3 bo mạch

```
 [Node 1: MÁY PHÁT]               [Node 2: KÊNH TRUYỀN]               [Node 3: MÁY THU]
  ESP32-C3 SuperMini               ESP32-C3 SuperMini                 ESP32-C3 SuperMini
 ┌──────────────────┐             ┌──────────────────┐               ┌──────────────────┐
 │                  │             │                  │               │                  │
 │   GPIO 21 (TX1)  │────────────>│   GPIO 20 (RX0)  │               │                  │
 │                  │  (Dây Data) │   GPIO 21 (TX1)  │──────────────>│   GPIO 20 (RX0)  │
 │                  │             │                  │   (Dây Data)  │                  │
 │       GND        │──────┬──────│       GND        │───────┬───────│       GND        │
 └────────┬─────────┘      │      └────────┬─────────┘       │       └────────┬─────────┘
          │                │               │                 │                │
          │ (Cáp USB)      └───────────────┼─────────────────┘ (Chung Mass)   │ (Cáp USB)
          ▼                                ▼                                  ▼
 ┌──────────────────────────────────────────────────────────────────────────────────────┐
 │                     MÁY TÍNH (PC) - CẮM CHUNG 1 CỔNG / HUB USB                       │
 │  • Port 8001: Web Dashboard Tx   • Port 8002: Web Channel   • Port 8003: Web Rx     │
 └──────────────────────────────────────────────────────────────────────────────────────┘
```

---

### 2. Bảng phân công chân tín hiệu (Pin Mapping)

| Bo mạch | Tên Chân trên bo | Nối đến Bo mạch | Chân đích | Mục đích tín hiệu |
| :--- | :--- | :--- | :--- | :--- |
| **Node 1 (Tx)** | `GPIO 21` (TX1) | **Node 2 (Kênh)** | `GPIO 20` (RX0) | Đường truyền dữ liệu mã hóa từ Máy phát sang Kênh |
| **Node 2 (Kênh)** | `GPIO 21` (TX1) | **Node 3 (Rx)** | `GPIO 20` (RX0) | Đường truyền dữ liệu đã bơm nhiễu từ Kênh sang Máy thu |
| **Node 1 (Tx)** | `GND` | **Node 2 & 3** | `GND` | **BẮT BUỘC:** Nối chung Mass (Ground) giữa 3 bo để không bị trôi điện áp logic |
| **Tất cả các bo** | Cổng USB Type-C | **PC** | Cổng USB PC | Cấp nguồn 5V + Giao tiếp Serial USB CDC với 3 Web Dashboard độc lập |

---

### 3. Ngoại vi mở rộng (Tùy chọn)

* **Màn hình OLED SSD1306 (Gắn tại Node 3 - Máy thu):**
  * Chân `VCC` $\to$ `3V3` của Node 3.
  * Chân `GND` $\to$ `GND` của Node 3.
  * Chân `SDA` $\to$ `GPIO 8` của Node 3.
  * Chân `SCL` $\to$ `GPIO 9` của Node 3.

---

### 4. Quy trình kiểm tra an toàn trước khi cấp nguồn

1. **Kiểm tra thông mạch Mass:** Dùng đồng hồ VOM đo thang thông mạch giữa chân `GND` của cả 3 bo $\to$ Đảm bảo điện trở xấp xỉ $0\,\Omega$.
2. **Kiểm tra chéo TX - RX:**
   * TX của Node 1 nối vào RX của Node 2.
   * TX của Node 2 nối vào RX của Node 3.
   * **Tuyệt đối không nối TX vào TX.**
3. **Cắm cáp USB:** Cắm lần lượt từng bo vào cổng USB PC. Trên Windows mở `Device Manager` $\to$ mục `Ports (COM & LPT)` sẽ xuất hiện 3 cổng COM mới (ví dụ: `COM3`, `COM4`, `COM5`).
