# WORKFLOW TRIỂN KHAI HỆ THỐNG TRUYỀN THÔNG TIN SỐ TRÊN 3x ESP32-C3 SUPERMINI
*(Cập nhật theo yêu cầu: Mã hóa nguồn đa phương thức & Mã hóa kênh hệ thống)*

---

## 1. TỔNG QUAN KIẾN TRÚC HỆ THỐNG

Dự án hiện thực một hệ thống truyền thông tin số hoàn chỉnh theo mô hình Claude Shannon trên 3 bo mạch vi điều khiển ESP32-C3 SuperMini độc lập:
* **Node 1 (Transmitter - Tx):** Thu thập dữ liệu thô $\to$ **Mã hóa nguồn** (Huffman / Arithmetic / LZW) $\to$ **Mã hóa kênh hệ thống** (Mã khối tuyến tính hệ thống / Mã vòng hệ thống) $\to$ Đóng gói khung $\to$ Phát UART vật lý.
* **Node 2 (Channel Emulator - Ch):** Nhận khung $\to$ Giả lập nhiễu kênh truyền vật lý (mô hình BSC, đảo bit theo xác suất BER cấu hình) $\to$ Chuyển tiếp UART sang Rx.
* **Node 3 (Receiver - Rx):** Bắt khung & Đồng bộ $\to$ **Giải mã kênh hệ thống** (Phát hiện & Sửa lỗi) $\to$ **Giải mã nguồn** $\to$ Khôi phục dữ liệu gốc $\to$ Đánh giá Metrics (BER, CR, FER) $\to$ Hiển thị OLED / Serial.

```
===================================================================================
                                   MÁY PHÁT (TX)
===================================================================================
[ Dữ liệu thô (Sensor/Text/PC) ]
              │
              ▼
    [ 1. Mã hóa nguồn ]  ──> Nén dữ liệu:
                              • Mặc định: Thuật toán Huffman
                              • Mở rộng: Mã số học (Arithmetic) / LZW
              │
              ▼
    [ 2. Mã hóa kênh ]   ──> Thêm bit kiểm tra theo dạng HỆ THỐNG (Systematic):
                              • Lựa chọn A: Mã khối tuyến tính hệ thống [M | P]
                                (Systematic Hamming (7,4) hoặc (8,4) SEC-DED)
                              • Lựa chọn B: Mã vòng hệ thống c(x) = x^(n-k)m(x) + r(x)
                                (Systematic Cyclic Code (7,4) với g(x) = x^3 + x + 1)
              │
              ▼
    [ 3. Đóng gói khung ]──> Thêm Header (SOF), Payload Length, CRC16, Tail (EOF)
              │
              ▼
   [ Giao tiếp phần cứng ]──> Đẩy luồng byte ra UART1 TX (GPIO 21)
              │
              ▼
===================================================================================
                               KÊNH TRUYỀN (CHANNEL)
===================================================================================
              │
              ▼
   [ 4. Nhận dữ liệu UART1 RX (GPIO 20) ]
              │
              ▼
   [ 5. Giả lập nhiễu kênh (BSC) ] ──> Đảo bit theo xác suất BER (P_e)
              │
              ▼
   [ 6. Chuyển tiếp UART2 TX (GPIO 21) ]
              │
              ▼
===================================================================================
                                   MÁY THU (RX)
===================================================================================
              │
              ▼
   [ 7. Nhận dữ liệu UART1 RX (GPIO 20) & Đồng bộ khung ]
              │
              ▼
   [ 8. Giải mã kênh hệ thống ]  ──> Tính Syndrome vector S / Đa thức dư S(x)
                                 ──> Dò vị trí lỗi & Lật lại bit sai (Sửa lỗi)
                                 ──> Trích xuất trực tiếp k bit thông tin M
              │
              ▼
   [ 9. Giải mã nguồn ]          ──> Giải nén Huffman (hoặc Arithmetic/LZW tương ứng)
              │
              ▼
   [ 10. Khôi phục & Hiển thị ]  ──> Đo đạc BER, Tỷ số nén CR, Hiển thị OLED / Serial
```

---

## 2. THIẾT KẾ CHI TIẾT TẦNG 1: MÃ HÓA NGUỒN (SOURCE CODING)

Kiến trúc phần mềm sử dụng mẫu thiết kế **Strategy Pattern**, cho phép hoán đổi thuật toán nén thông qua cờ cấu hình hoặc lệnh điều khiển:

### 2.1. Phương pháp Huffman Coding (Cốt lõi - Mặc định)
* **Nguyên lý:** Gán từ mã có độ dài biến thiên (Variable-Length Code - VLC) dựa trên xác suất xuất hiện của ký tự (ký tự xuất hiện nhiều mang mã ngắn, ký tự ít xuất hiện mang mã dài).
* **Cây mã:** Sử dụng cây Huffman tĩnh (Static Canonical Tree) được thống nhất giữa Tx và Rx dựa trên tập ký tự mẫu (Standard ASCII/Sensor text).
* **Bit-Packer Buffer:** Thuật toán quản lý dịch bit gom các từ mã Huffman độ dài biến thiên thành các khối $k$-bit đồng nhất để nạp vào bộ mã hóa kênh.

### 2.2. Phương pháp Mã số học (Arithmetic Coding - Module mở rộng)
* **Nguyên lý:** Mã hóa toàn bộ chuỗi ký tự thành một số thực duy nhất trong khoảng nửa mở $[0, 1)$ dựa trên dải xác suất tích lũy (Cumulative Probability Distribution).
* **Đặc tính:** Đạt hiệu suất nén sát với giới hạn Entropy của Shannon hơn Huffman, thích hợp xử lý luồng tin dài.

### 2.3. Phương pháp Mã hóa LZW (Lempel-Ziv-Welch - Module mở rộng)
* **Nguyên lý:** Mã hóa từ điển (Dictionary-based), không cần biết trước xác suất xuất hiện của ký tự. Bảng từ điển được xây dựng động liên tục trong quá trình quét chuỗi dữ liệu.
* **Đặc tính:** Rất hiệu quả khi dữ liệu có nhiều cụm từ hoặc mẫu ký tự lặp lại (chu kỳ đo lường sensor).

---

## 3. THIẾT KẾ CHI TIẾT TẦNG 2: MÃ HÓA KÊNH HỆ THỐNG (SYSTEMATIC CHANNEL CODING)

> **Đặc tính bắt buộc của mã hệ thống (Systematic Code):**  
> Trong từ mã đầu ra $C$ có độ dài $n$ bit, thì $k$ bit thông tin gốc $M = (m_1, m_2, \dots, m_k)$ luôn xuất hiện tường minh và nguyên vẹn ở vị trí cố định (thường là $k$ bit đầu hoặc $k$ bit cuối). Các bit còn lại $(n-k)$ là các bit kiểm tra dư thừa (Parity bits) $P = (p_1, p_2, \dots, p_{n-k})$.  
> $\to$ **Dạng từ mã:** $C = [M \mid P]$ hoặc $C = [P \mid M]$.  
> *Lợi ích lớn:* Nếu kênh truyền không bị lỗi hoặc sau khi đã sửa xong lỗi, việc trích xuất dữ liệu gốc cực kỳ nhanh chóng mà không cần qua mạch giải ánh xạ phức tạp.

Hệ thống hỗ trợ 2 nhóm phương thức mã kênh hệ thống:

### 3.1. Phương thức A: Mã khối tuyến tính hệ thống (Systematic Linear Block Code)
* **Đại diện tiêu chuẩn:** **Systematic Hamming (7,4)** và **Systematic Extended Hamming (8,4) SEC-DED**.
* **Cơ chế ma trận sinh hệ thống:**  
  Ma trận sinh $G$ có dạng khối chuẩn:  
  $$G = [I_k \mid P_{k \times (n-k)}]$$  
  Trong đó $I_k$ là ma trận đơn vị cấp $k$.  
  Từ mã tạo ra: $C = M \cdot G = [M \cdot I_k \mid M \cdot P] = [M \mid P]$.
* **Ma trận kiểm tra hệ thống $H$:**  
  $$H = [P^T \mid I_{n-k}]$$  
* **Giải mã Syndrome tại Rx:**  
  * Nhận vector $R = C \oplus E$ (với $E$ là vector lỗi do kênh).
  * Tính hội chứng lỗi: $S = R \cdot H^T$.
  * Nếu $S = 0$: Không có lỗi, lấy ngay $k$ bit đầu làm dữ liệu.
  * Nếu $S \neq 0$: Tra bảng tìm vị trí bit sai $\to$ Lật bit sửa lỗi $\to$ Lấy $k$ bit dữ liệu.

### 3.2. Phương thức B: Mã vòng hệ thống (Systematic Cyclic Code)
* **Đại diện tiêu chuẩn:** **Systematic Cyclic (7,4)** với đa thức sinh $g(x) = x^3 + x + 1$.
* **Cơ chế đa thức hệ thống:**  
  * Đa thức thông tin bậc $k-1$: $m(x) = m_0 + m_1 x + \dots + m_{k-1} x^{k-1}$.
  * Dịch $m(x)$ sang trái $(n-k)$ bậc: $x^{n-k} m(x)$.
  * Thực hiện phép chia đa thức trên trường nhị phân $GF(2)$ cho đa thức sinh $g(x)$:  
    $$\frac{x^{n-k} m(x)}{g(x)} = q(x) + \frac{r(x)}{g(x)}$$  
    Trong đó $r(x)$ là phần dư bậc bé hơn $n-k$.
  * Từ mã hệ thống thu được:  
    $$c(x) = x^{n-k} m(x) + r(x)$$  
    (Phần dư $r(x)$ chính là các bit parity ghép vào đuôi của các bit thông tin $m(x)$).
* **Giải mã tại Rx:**  
  * Nhận đa thức $r'(x) = c(x) + e(x)$.
  * Chia $r'(x)$ cho $g(x)$ để tìm đa thức hội chứng $S(x)$.
  * Dùng thuật toán thanh ghi dịch Meggitt Decoder hoặc bảng tra Syndrome để định vị và sửa bit lỗi.

---

## 4. THIẾT KẾ PHẦN CỨNG & KẾT NỐI (3x ESP32-C3 SUPERMINI)

```
[Node 1: Tx]                           [Node 2: Channel]                      [Node 3: Rx]
(ESP32-C3 #1)                          (ESP32-C3 #2)                          (ESP32-C3 #3)
+---------------+                      +---------------+                      +---------------+
| GPIO 21 (TX1) | -------------------> | GPIO 20 (RX1) |                      |               |
|               |                      | GPIO 21 (TX2) | -------------------> | GPIO 20 (RX1) |
| GND           | --------+----------- | GND           | --------+----------- | GND           |
+---------------+         |            +---------------+         |            +---------------+
                          |                                      |
                          +--------------------------------------+ (Chung Mass bắt buộc)
```

* **Giao tiếp liên bo:** UART phần cứng (Baudrate 115200 bps, 8N1).
* **Cổng USB Type-C:** Kết nối đồng thời 3 bo về PC qua USB Hub để theo dõi 3 cổng COM độc lập và tương tác lệnh.

---

## 5. THIẾT KẾ ĐÓNG GÓI KHUNG (FRAMING PROTOCOL)

Để đảm bảo máy thu đồng bộ được luồng byte và biết được chế độ mã hóa đang sử dụng:
```
+-------+-------+---------+----------+----------+-----------------------+--------+-------+
|  SOF  |  SEQ  | SRC_ALG | CHN_ALG  |  LENGTH  | PAYLOAD (CODEWORDS)   | CRC16  |  EOF  |
| 1 byte| 1 byte| 1 byte  | 1 byte   |  1 byte  |       N bytes         | 2 bytes| 1 byte|
+-------+-------+---------+----------+----------+-----------------------+--------+-------+
```
* `SOF`: `0x7E` (Bắt đầu khung)
* `SEQ`: Số thứ tự gói tin (0 - 255)
* `SRC_ALG`: Cờ thuật toán nén nguồn (`0x01`: Huffman, `0x02`: Arithmetic, `0x03`: LZW)
* `CHN_ALG`: Cờ thuật toán mã kênh (`0x01`: Systematic Hamming (8,4), `0x02`: Systematic Cyclic (7,4))
* `LENGTH`: Số lượng byte của Payload
* `PAYLOAD`: Luồng các từ mã hệ thống đã sinh ra
* `CRC16`: Kiểm tra toàn vẹn khung ở tầng liên kết
* `EOF`: `0x7F` (Kết thúc khung)

---

## 6. KẾ HOẠCH TRIỂN KHAI THEO CÁC PHA (ACTION PLAN)

* **Pha 1: Xây dựng bộ thư viện lõi C/C++ thuần (Core Engine)**
  * `source_coding/`: `huffman.c/h`, `arithmetic.c/h`, `lzw.c/h`.
  * `channel_coding/`: `systematic_hamming.c/h`, `systematic_cyclic.c/h`.
  * `framing/`: `frame_packer.c/h`, `frame_parser.c/h`.
  * Bộ test suite chạy trực tiếp trên PC để kiểm chứng độ chính xác 100% trước khi nạp vào mạch.
* **Pha 2: Lập trình Firmware Node 1 (Tx)**
  * Nhận input (Text mẫu hoặc cảm biến) $\to$ Mã hóa nguồn $\to$ Mã hóa kênh hệ thống $\to$ Bắn UART1.
* **Pha 3: Lập trình Firmware Node 2 (Channel Emulator)**
  * Nhận frame $\to$ Bơm nhiễu BSC (đảo bit theo BER điều khiển được qua Serial PC) $\to$ Đẩy sang Node 3.
* **Pha 4: Lập trình Firmware Node 3 (Rx)**
  * Bắt khung $\to$ Giải mã kênh hệ thống (dò tìm syndrome, sửa bit) $\to$ Giải mã nguồn $\to$ Tính Metrics.
* **Pha 5: Tích hợp hệ thống, đo đạc Metrics & Báo cáo**
  * So sánh hiệu quả giữa các thuật toán: Tỷ số nén của Huffman vs LZW vs Arithmetic; Khả năng sửa lỗi của Systematic Hamming vs Systematic Cyclic; Đo Channel BER vs Residual BER.

---

## 7. CẤU TRÚC CÂY THƯ MỤC DỰ ÁN (PROJECT DIRECTORY TREE)

Dự án được phân tách hoàn toàn thành **3 thư mục độc lập** tương ứng với 3 thành phần chính của hệ sinh thái (Máy phát, Kênh truyền, Máy thu), cùng với thư mục dùng chung (`shared/` cho giao thức & CRC):

```text
out/esp32_c3_comm_system/
├── WORKFLOW.md                             # Tài liệu đặc tả kiến trúc, quy trình & hướng dẫn
│
├── 01_transmitter/                         # ==================== [1. MÁY PHÁT - TX] ====================
│   ├── include/                            # Thư viện thuật toán phía Máy phát
│   │   ├── tx_config.h                     # Cấu hình GPIO (TX1=21), chu kỳ gửi, dữ liệu mẫu
│   │   ├── bit_packer.h                    # Gom bit nén thành khối cố định nạp cho mã kênh
│   │   ├── src_encoder_huffman.h           # Bộ mã hóa Huffman (Cây tĩnh chuẩn / Cây động)
│   │   ├── src_encoder_arithmetic.h        # Bộ mã hóa số học (Arithmetic Encoder)
│   │   ├── src_encoder_lzw.h               # Bộ mã hóa từ điển LZW
│   │   ├── chn_encoder_hamming.h           # Bộ mã hóa Systematic Hamming (7,4) & (8,4) SEC-DED
│   │   ├── chn_encoder_cyclic.h            # Bộ mã hóa Systematic Cyclic (7,4) với g(x)=x^3+x+1
│   │   └── frame_builder.h                 # Đóng gói khung UART (SOF, Length, Flags, CRC16, EOF)
│   ├── src/                                # Hiện thực mã nguồn C/C++ thuần
│   │   ├── bit_packer.c
│   │   ├── src_encoder_huffman.c
│   │   ├── src_encoder_arithmetic.c
│   │   ├── src_encoder_lzw.c
│   │   ├── chn_encoder_hamming.c
│   │   ├── chn_encoder_cyclic.c
│   │   └── frame_builder.c
│   ├── test_pc/                            # Unit test độc lập cho Máy phát trên PC
│   │   ├── Makefile                        # Biên dịch test máy phát trên PC
│   │   └── main_test_tx.c                  # Kiểm tra quy trình nén và mã hóa kênh từ input thô
│   ├── platformio.ini                      # Cấu hình PlatformIO nạp firmware ESP32-C3 SuperMini Tx
│   └── main_tx.cpp                         # Firmware chính Node 1 (Tx)
│
├── 02_channel/                             # ==================== [2. KÊNH TRUYỀN - CH] =================
│   ├── include/                            # Thư viện giả lập kênh vật lý
│   │   ├── channel_config.h                # Cấu hình GPIO (RX1=20, TX2=21), baudrate
│   │   ├── noise_generator.h               # Bộ sinh nhiễu BSC (Bernoulli bit-flip) & AWGN
│   │   └── frame_forwarder.h               # Bóc tách và chuyển tiếp khung an toàn
│   ├── src/                                # Hiện thực mã nguồn C/C++ thuần
│   │   ├── noise_generator.c
│   │   └── frame_forwarder.c
│   ├── test_pc/                            # Unit test độc lập cho Kênh truyền trên PC
│   │   ├── Makefile                        # Biên dịch test kênh trên PC
│   │   └── main_test_channel.c             # Giả lập bơm nhiễu theo các tỷ lệ BER khác nhau
│   ├── platformio.ini                      # Cấu hình PlatformIO nạp firmware ESP32-C3 SuperMini Kênh
│   └── main_channel.cpp                    # Firmware chính Node 2 (Channel) + CLI chỉnh BER qua USB
│
├── 03_receiver/                            # ==================== [3. MÁY THU - RX] =====================
│   ├── include/                            # Thư viện thuật toán phía Máy thu
│   │   ├── rx_config.h                     # Cấu hình GPIO (RX1=20), I2C OLED (SDA=8, SCL=9)
│   │   ├── frame_parser.h                  # Máy trạng thái FSM đồng bộ & bóc tách khung UART
│   │   ├── chn_decoder_hamming.h           # Bộ giải mã Systematic Hamming (tính Syndrome, sửa lỗi)
│   │   ├── chn_decoder_cyclic.h            # Bộ giải mã Systematic Cyclic (chia đa thức, sửa lỗi)
│   │   ├── bit_unpacker.h                  # Bóc tách luồng bit đã sửa đưa vào giải nén
│   │   ├── src_decoder_huffman.h           # Bộ giải nén Huffman
│   │   ├── src_decoder_arithmetic.h        # Bộ giải nén số học (Arithmetic Decoder)
│   │   ├── src_decoder_lzw.h               # Bộ giải nén LZW
│   │   ├── metrics_evaluator.h             # Tính toán BER, FER, Tỷ số nén, Tỷ lệ sửa lỗi thành công
│   │   └── display_oled.h                  # Giao diện xuất thông số lên OLED SSD1306
│   ├── src/                                # Hiện thực mã nguồn C/C++ thuần
│   │   ├── frame_parser.c
│   │   ├── chn_decoder_hamming.c
│   │   ├── chn_decoder_cyclic.c
│   │   ├── bit_unpacker.c
│   │   ├── src_decoder_huffman.c
│   │   ├── src_decoder_arithmetic.c
│   │   ├── src_decoder_lzw.c
│   │   ├── metrics_evaluator.c
│   │   └── display_oled.c
│   ├── test_pc/                            # Unit test độc lập cho Máy thu trên PC
│   │   ├── Makefile                        # Biên dịch test máy thu trên PC
│   │   └── main_test_rx.c                  # Kiểm tra sửa lỗi bit và giải nén dữ liệu
│   ├── platformio.ini                      # Cấu hình PlatformIO nạp firmware ESP32-C3 SuperMini Rx
│   └── main_rx.cpp                         # Firmware chính Node 3 (Rx)
│
├── shared/                                 # Định nghĩa chung giữa 3 khối (Header, Protocol, CRC)
│   ├── protocol_defs.h                     # Định dạng khung, các mã cờ SOF, EOF, SRC_ALG, CHN_ALG
│   ├── crc16.h                             # Thuật toán tính CRC16-CCITT dùng chung
│   └── crc16.c
│
└── docs/                                   # Tài liệu hướng dẫn & Báo cáo
    ├── wiring_diagram.md                   # Sơ đồ câu dây phần cứng giữa 3 bo ESP32-C3 SuperMini
    └── performance_benchmark.md            # Bảng so sánh kết quả thực nghiệm (BER, CR, FER)
```


