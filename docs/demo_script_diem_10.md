# KỊCH BẢN THUYẾT TRÌNH BẢO VỆ ĐỒ ÁN ĐẠT ĐIỂM 10 TUYỆT ĐỐI
## Đề tài: Hệ thống truyền ảnh Bitmap 64x64 Tự thích nghi qua Kênh Nhiễu trên 3x ESP32-C3 SuperMini

---

## I. MỤC TIÊU BẢO VỆ & THÔNG ĐIỆP CHÍNH VỚI GIẢNG VIÊN

* **Đáp ứng chuẩn 100% đề bài:** Hệ thống thực thi vật lý trên 3 khối vi điều khiển độc lập: **Máy phát $\to$ Kênh $\to$ Máy thu**; có nén nguồn (Huffman) và mã hóa kênh hệ thống (Hamming & Cyclic); có giả lập nhiễu vật lý và đo lường metrics.
* **Tính năng vượt trội ("Show-off"):**
  1. **Sa bàn phần cứng hoạt động độc lập (Stand-alone):** Không cần mở laptop, mọi kết quả truyền nhận ảnh và biến động lỗi được hiển thị trực tiếp trên 3 màn hình OLED SSD1306.
  2. **Truyền ảnh trực quan:** Người xem thấy ngay ảnh gốc bên phát và ảnh được khôi phục nguyên vẹn bên thu.
  3. **Biến trở vật lý (Hardware Knob):** Xoay biến trở để trực tiếp tăng giảm tỷ lệ đảo bit $0\% \to 12\%$.
  4. **Mã hóa kênh tự thích nghi (Adaptive Channel Coding):** Hệ thống tự động nhận biết mức nhiễu thông qua đường phản hồi (Feedback) 2 chiều xuyên qua Kênh, tự động đổi chế độ mã hóa và bật 3 đèn LED tương ứng.

---

## II. KỊCH BẢN 4 BƯỚC BIỂU DIỄN THỰC NGHIỆM TRƯỚC GIẢNG VIÊN

### Bước 1: Giới thiệu Kiến trúc Phần cứng Sa bàn (Thời lượng: 1 phút)
* **Thao tác:** Cắm nguồn cổng USB cho sa bàn. Cả 3 màn hình OLED sáng lên:
  * OLED 1 (Máy phát): Hiển thị ảnh mẫu 64x64 (mặt cười) và chế độ hiện tại.
  * OLED 2 (Kênh truyền): Hiển thị thanh đo mức nhiễu (ban đầu ở mức $0.0\%$).
  * OLED 3 (Máy thu): Tái tạo trọn vẹn ảnh mặt cười và số bit lỗi $= 0$.
* **Lời thuyết minh:** *"Thưa thầy/cô, hệ thống của nhóm em được thiết kế theo mô hình Shannon chuẩn mực với 3 vi điều khiển ESP32-C3 SuperMini độc lập. Dữ liệu ảnh Bitmap 64x64 được chia thành 4 mảnh, nén Huffman và đóng khung truyền qua UART. Toàn bộ sa bàn chạy độc lập mà không cần bất kỳ máy tính hỗ trợ nào."*

---

### Bước 2: Thử nghiệm Trạng thái Kênh lý tưởng (Mode Direct - LED Xanh) (Thời lượng: 1 phút)
* **Thao tác:** Giữ biến trở ở vị trí vặn hết về bên trái ($BER = 0.0\%$).
* **Hiện tượng:**
  * LED Xanh ở Máy thu sáng rõ.
  * Màn hình OLED Máy phát báo `MODE: DIRECT`.
  * Ảnh nhận trên OLED 3 sắc nét tuyệt đối, không có bất kỳ điểm nhiễu nào.
* **Lời thuyết minh:** *"Khi kênh truyền ở điều kiện lý tưởng ($BER < 1\%$), hệ thống tự động chọn chế độ DIRECT. Lúc này thông lượng đường truyền đạt mức tối đa do không phải tốn bit dư thừa cho mã hóa kênh."*

---

### Bước 3: Vặn biến trở lên mức nhiễu vừa $\to$ Kích hoạt Mã Hamming (7,4) (Thời lượng: 1.5 phút)
* **Thao tác:** Từ từ vặn biến trở sang phải sao cho màn hình OLED Kênh hiển thị $BER \approx 2.0\% - 3.5\%$.
* **Hiện tượng:**
  * Màn hình Kênh hiển thị số lượng bit bị đảo tăng lên liên tục.
  * Máy thu phát hiện lỗi $\to$ Gửi gói Feedback ngược qua Kênh về Máy phát.
  * **Đèn LED Vàng lập tức bật sáng** (tắt đèn Xanh).
  * OLED Máy phát tự động nhảy sang `MODE: HAM (7,4)`.
  * Trên OLED Máy thu, chỉ số `Fix:` liên tục đếm số bit lỗi 1-bit đã được sửa thành công.
  * **Điểm đắt giá:** Dù kênh truyền đang có nhiễu, ảnh mặt cười trên OLED 3 vẫn hiển thị tròn trịa, không hề bị vỡ pixel!
* **Lời thuyết minh:** *"Khi em tăng độ nhiễu lên khoảng $2\%$, máy thu phát hiện lỗi thông qua Syndrome vector và kích hoạt cơ chế tự thích nghi. Đèn LED Vàng sáng báo hiệu hệ thống đã tự động chuyển sang Mã khối tuyến tính hệ thống Hamming (7,4). Các lỗi 1-bit trên từng block được sửa chữa hoàn toàn, bảo vệ độ toàn vẹn của bức ảnh."*

---

### Bước 4: Vặn biến trở lên mức nhiễu nặng $\to$ Kích hoạt Mã Vòng Cyclic (7,4) & Đổi ảnh (Thời lượng: 1.5 phút)
* **Thao tác:** Vặn biến trở lên mức cao ($BER \ge 5\% - 8\%$), đồng thời bấm nút chuyển ảnh trên bo Máy phát.
* **Hiện tượng:**
  * **Đèn LED Đỏ bật sáng** (báo hiệu mức nhiễu nguy hiểm).
  * OLED Máy phát chuyển sang `MODE: CYC (7,4)`.
  * Khi bấm nút trên Máy phát (GPIO 10), ảnh trên OLED 1 chuyển sang icon "Điểm 10 Đồ án", và ngay lập tức ảnh trên OLED 3 cũng cập nhật sang biểu tượng "Điểm 10" sau khi giải mã thành công!
* **Lời thuyết minh:** *"Ở mức nhiễu cao ($BER > 4\%$), hệ thống tự động nâng cấp mức bảo vệ lên Mã vòng hệ thống Cyclic (7,4) với đa thức sinh $g(x) = x^3 + x + 1$. Đồng thời em bấm nút đổi ảnh để kiểm tra khả năng bắt khung và giải nén liên tục của hệ thống. Kết quả ảnh Điểm 10 được khôi phục chính xác."*

---

## III. BẢNG CÂU HỎI VẤN ĐÁP THƯỜNG GẶP & CÁCH TRẢ LỜI ĐIỂM 10

1. **Câu hỏi:** *Tại sao lại dùng mã hóa kênh tự thích nghi (Adaptive Coding) thay vì cố định một loại mã mạnh nhất?*
   * **Trả lời:** *"Mã sửa lỗi càng mạnh thì độ dư thừa (Redundancy) càng lớn, làm giảm tốc độ truyền (Code Rate $R = k/n$). Cơ chế thích nghi cho phép hệ thống đạt được sự cân bằng tối ưu: khi kênh tốt thì ưu tiên tốc độ tối đa, khi kênh xấu thì tăng cường bảo vệ để đảm bảo độ tin cậy."*

2. **Câu hỏi:** *Đường phản hồi (Feedback) từ Máy thu về Máy phát có vi phạm mô hình 3 khối Máy phát – Kênh – Máy thu không?*
   * **Trả lời:** *"Dạ không ạ. Tín hiệu Feedback từ Máy thu được gửi ngược xuyên qua bo mạch Kênh rồi Kênh mới chuyển tiếp về Máy phát. Điều này phản ánh chính xác mô hình kênh truyền thông tin hai chiều (Full-duplex Channel) trong thực tế mà không phá vỡ tính độc lập của 3 khối."*

3. **Câu hỏi:** *Làm thế nào để Máy thu giải mã sửa lỗi nhanh trên vi điều khiển mà không gây nghẽn tốc độ?*
   * **Trả lời:** *"Nhóm em sử dụng thuật toán tính Syndrome và giải mã hệ thống tối ưu. Với Hamming (8,4), Syndrome vector $S$ xác định chính xác vị trí bit lỗi và sửa bằng một phép XOR đơn giản $O(1)$, giúp vi điều khiển xử lý theo thời gian thực mà không bị trễ khung."*
