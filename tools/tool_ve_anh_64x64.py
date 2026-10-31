# -*- coding: utf-8 -*-
"""
BẢNG VẼ ĐIỂM ẢNH PIXEL (64x64) CHO HỆ THỐNG TRUYỀN TIN ESP32-C3
- Kích thước chuẩn: 64 x 64 pixels (Monochrome 1-bit, 512 bytes).
- Nền đen chuẩn OLED, vẽ nét sáng (trắng hoặc xanh neon).
- Tất cả ảnh vẽ xong được tự động lưu vào thư mục: saved_images/
- Lưu song song 3 định dạng: .png (ảnh xem), .bin (512 bytes nhị phân), .h (mảng mã C).
- Danh sách ảnh đã lưu trong thư mục hiển thị trực tiếp để bấm nạp lại nhanh.
"""

import os
import sys
import time
import glob
import subprocess
from datetime import datetime
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from PIL import Image, ImageTk

WIDTH = 64
HEIGHT = 64
TOTAL_BYTES = (WIDTH * HEIGHT) // 8  # 512 bytes
SCALE = 8  # Mỗi pixel phóng to thành 8x8 px trên màn hình máy tính -> Canvas 512x512 px

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
parent_saved = os.path.join(os.path.dirname(SCRIPT_DIR), "saved_images")
local_saved = os.path.join(SCRIPT_DIR, "saved_images")
SAVED_DIR = parent_saved if os.path.exists(parent_saved) else local_saved
os.makedirs(SAVED_DIR, exist_ok=True)

class PixelCanvasApp:
    def __init__(self, root):
        self.root = root
        self.root.title("BẢNG VẼ PIXEL 64x64 - HỆ THỐNG TRUYỀN TIN SỐ ESP32-C3")
        self.root.geometry("980x730")
        self.root.minsize(920, 680)
        self.root.configure(bg="#121218")

        # Ma trận điểm ảnh 64x64 (0: Đen, 1: Sáng)
        self.grid_data = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]
        
        # Lịch sử Undo / Redo
        self.history = []
        self.history_index = -1
        self._push_history()

        # Trạng thái vẽ
        self.brush_size = 1
        self.draw_color = 1  # 1: Vẽ sáng, 0: Tẩy
        self.last_draw_pos = None
        self.show_grid = tk.BooleanVar(value=True)
        self.neon_mode = tk.BooleanVar(value=True)  # True: Màu xanh Neon OLED, False: Trắng

        self._build_ui()
        self._refresh_saved_list()

    def _get_draw_color_hex(self):
        return "#00E5FF" if self.neon_mode.get() else "#FFFFFF"

    def _build_ui(self):
        # 1. Header
        header = tk.Frame(self.root, bg="#1A1A24", pady=8, padx=16)
        header.pack(fill=tk.X)

        title_lbl = tk.Label(
            header, text="🎨 BẢNG VẼ PIXEL 64×64 (CHUẨN ẢNH OLED SSD1306 - 512 BYTES)", 
            font=("Segoe UI", 13, "bold"), fg="#00E5FF", bg="#1A1A24"
        )
        title_lbl.pack(side=tk.LEFT)

        folder_btn = tk.Button(
            header, text="📁 Mở Thư Mục Ảnh (saved_images)", bg="#252538", fg="#E0E0E0",
            font=("Segoe UI", 9, "bold"), relief=tk.FLAT, padx=10, pady=4,
            command=self._open_saved_folder
        )
        folder_btn.pack(side=tk.RIGHT)

        # 2. Main Content (Left: Canvas | Right: Controls & Preview & List)
        main_frame = tk.Frame(self.root, bg="#121218", padx=12, pady=12)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # --- CỘT TRÁI: CANVAS VẼ 512x512 ---
        left_frame = tk.Frame(main_frame, bg="#181822", padx=6, pady=6, relief=tk.SOLID, bd=1)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH)

        canvas_border = tk.Frame(left_frame, bg="#2A2A3E", padx=2, pady=2)
        canvas_border.pack()

        self.canvas = tk.Canvas(
            canvas_border, width=WIDTH * SCALE, height=HEIGHT * SCALE,
            bg="#000000", highlightthickness=0, cursor="crosshair"
        )
        self.canvas.pack()

        # Sự kiện vẽ chuột
        self.canvas.bind("<Button-1>", self._on_mouse_down_draw)
        self.canvas.bind("<B1-Motion>", self._on_mouse_drag_draw)
        self.canvas.bind("<ButtonRelease-1>", self._on_mouse_up)

        self.canvas.bind("<Button-3>", self._on_mouse_down_erase)
        self.canvas.bind("<B3-Motion>", self._on_mouse_drag_erase)
        self.canvas.bind("<ButtonRelease-3>", self._on_mouse_up)

        # Thanh hướng dẫn chuột dưới Canvas
        guide_lbl = tk.Label(
            left_frame, text="💡 Chuột trái: Vẽ nét sáng  |  Chuột phải: Tẩy nét  |  Kéo chuột: Vẽ nét liền",
            font=("Segoe UI", 9), fg="#9E9EAF", bg="#181822", pady=4
        )
        guide_lbl.pack()

        # --- CỘT PHẢI: BẢNG ĐIỀU KHIỂN & PREVIEW ---
        right_frame = tk.Frame(main_frame, bg="#121218", padx=10)
        right_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        # Khung Xem Trước (Preview OLED)
        prev_group = tk.LabelFrame(right_frame, text=" 👁️ Xem Trước Kích Thước Thật OLED (64x64 & 128x64) ", 
                                   bg="#181822", fg="#00E5FF", font=("Segoe UI", 9, "bold"), padx=10, pady=8)
        prev_group.pack(fill=tk.X, pady=(0, 8))

        prev_box = tk.Frame(prev_group, bg="#181822")
        prev_box.pack()

        # 64x64 True Size
        f1 = tk.Frame(prev_box, bg="#181822", padx=10)
        f1.pack(side=tk.LEFT)
        tk.Label(f1, text="Kích thước 1:1 (64x64)", font=("Segoe UI", 8), fg="#9E9EAF", bg="#181822").pack()
        self.preview_64 = tk.Label(f1, bg="#000000", relief=tk.SOLID, bd=1, width=64, height=64)
        self.preview_64.pack()

        # 128x64 OLED Simulator (Căn giữa giống màn hình 3)
        f2 = tk.Frame(prev_box, bg="#181822", padx=10)
        f2.pack(side=tk.LEFT)
        tk.Label(f2, text="Mô phỏng Màn OLED 3 (128x64)", font=("Segoe UI", 8), fg="#9E9EAF", bg="#181822").pack()
        self.preview_oled = tk.Label(f2, bg="#000000", relief=tk.SOLID, bd=1, width=128, height=64)
        self.preview_oled.pack()

        # Thanh công cụ bút & thao tác vẽ
        tools_group = tk.LabelFrame(right_frame, text=" 🛠️ Công Cụ Bút Vẽ ", bg="#181822", fg="#00E5FF", 
                                    font=("Segoe UI", 9, "bold"), padx=10, pady=6)
        tools_group.pack(fill=tk.X, pady=(0, 8))

        row_tool1 = tk.Frame(tools_group, bg="#181822")
        row_tool1.pack(fill=tk.X, pady=2)

        tk.Label(row_tool1, text="Cỡ bút:", font=("Segoe UI", 9), fg="#E0E0E0", bg="#181822").pack(side=tk.LEFT)
        self.brush_var = tk.IntVar(value=1)
        for size in [1, 2, 3, 4]:
            rb = tk.Radiobutton(
                row_tool1, text=f"{size}px", variable=self.brush_var, value=size,
                bg="#181822", fg="#E0E0E0", selectcolor="#2A2A3E", activebackground="#181822",
                command=self._on_brush_change
            )
            rb.pack(side=tk.LEFT, padx=4)

        cb_grid = tk.Checkbutton(
            row_tool1, text="Hiện lưới", variable=self.show_grid, bg="#181822", fg="#E0E0E0",
            selectcolor="#2A2A3E", activebackground="#181822", command=self._redraw_canvas
        )
        cb_grid.pack(side=tk.RIGHT, padx=6)

        cb_neon = tk.Checkbutton(
            row_tool1, text="Màu Neon", variable=self.neon_mode, bg="#181822", fg="#E0E0E0",
            selectcolor="#2A2A3E", activebackground="#181822", command=self._redraw_canvas
        )
        cb_neon.pack(side=tk.RIGHT, padx=4)

        # Các nút thao tác ảnh: Xóa, Đảo màu, Hoàn tác
        row_tool2 = tk.Frame(tools_group, bg="#181822")
        row_tool2.pack(fill=tk.X, pady=4)

        btn_clear = tk.Button(row_tool2, text="🧹 Xóa Hết", bg="#3A1D28", fg="#FF5252", relief=tk.FLAT,
                              font=("Segoe UI", 8, "bold"), padx=8, pady=3, command=self._clear_canvas)
        btn_clear.pack(side=tk.LEFT, padx=2)

        btn_invert = tk.Button(row_tool2, text="🔄 Đảo Màu", bg="#252538", fg="#E0E0E0", relief=tk.FLAT,
                               font=("Segoe UI", 8), padx=8, pady=3, command=self._invert_colors)
        btn_invert.pack(side=tk.LEFT, padx=2)

        btn_fill = tk.Button(row_tool2, text="⬛ Tràn Đen", bg="#252538", fg="#E0E0E0", relief=tk.FLAT,
                             font=("Segoe UI", 8), padx=8, pady=3, command=self._clear_canvas)
        btn_fill.pack(side=tk.LEFT, padx=2)

        btn_undo = tk.Button(row_tool2, text="↩ Undo", bg="#252538", fg="#E0E0E0", relief=tk.FLAT,
                             font=("Segoe UI", 8), padx=8, pady=3, command=self._undo)
        btn_undo.pack(side=tk.LEFT, padx=2)

        btn_redo = tk.Button(row_tool2, text="↪ Redo", bg="#252538", fg="#E0E0E0", relief=tk.FLAT,
                             font=("Segoe UI", 8), padx=8, pady=3, command=self._redo)
        btn_redo.pack(side=tk.LEFT, padx=2)

        # Dịch chuyển & Lật ảnh
        row_shift = tk.Frame(tools_group, bg="#181822")
        row_shift.pack(fill=tk.X, pady=2)
        tk.Label(row_shift, text="Dịch/Lật:", font=("Segoe UI", 8), fg="#9E9EAF", bg="#181822").pack(side=tk.LEFT)
        for arrow, (dx, dy) in [("⬅", (-1, 0)), ("➡", (1, 0)), ("⬆", (0, -1)), ("⬇", (0, 1))]:
            tk.Button(row_shift, text=arrow, bg="#252538", fg="#E0E0E0", font=("Segoe UI", 8), width=3,
                      relief=tk.FLAT, command=lambda x=dx, y=dy: self._shift_image(x, y)).pack(side=tk.LEFT, padx=1)
        tk.Button(row_shift, text="Lật Ngang", bg="#252538", fg="#E0E0E0", font=("Segoe UI", 8), relief=tk.FLAT,
                  command=self._flip_h).pack(side=tk.LEFT, padx=3)
        tk.Button(row_shift, text="Lật Dọc", bg="#252538", fg="#E0E0E0", font=("Segoe UI", 8), relief=tk.FLAT,
                  command=self._flip_v).pack(side=tk.LEFT, padx=1)

        # Khung Lưu Ảnh & Danh sách trong saved_images
        save_group = tk.LabelFrame(right_frame, text=" 💾 Lưu & Quản Lý Ảnh Trong saved_images/ ", 
                                   bg="#181822", fg="#00E5FF", font=("Segoe UI", 9, "bold"), padx=10, pady=8)
        save_group.pack(fill=tk.BOTH, expand=True, pady=(0, 4))

        # Nhập tên file để lưu
        row_save = tk.Frame(save_group, bg="#181822")
        row_save.pack(fill=tk.X, pady=4)
        tk.Label(row_save, text="Tên ảnh:", font=("Segoe UI", 9), fg="#E0E0E0", bg="#181822").pack(side=tk.LEFT)
        self.entry_name = tk.Entry(row_save, bg="#252538", fg="#00E5FF", insertbackground="#00E5FF", font=("Segoe UI", 9))
        self.entry_name.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=6)
        self.entry_name.insert(0, "anh_moi")

        btn_save = tk.Button(row_save, text="💾 LƯU ẢNH", bg="#00C853", fg="#FFFFFF", font=("Segoe UI", 9, "bold"),
                             relief=tk.FLAT, padx=10, pady=3, command=self._save_image)
        btn_save.pack(side=tk.RIGHT)

        # Danh sách các ảnh đã lưu
        tk.Label(save_group, text="Danh sách ảnh có sẵn (Nhấp đúp hoặc bấm Mở để tải lên bảng vẽ):",
                 font=("Segoe UI", 8), fg="#9E9EAF", bg="#181822").pack(anchor=tk.W, pady=(6, 2))

        list_frame = tk.Frame(save_group, bg="#181822")
        list_frame.pack(fill=tk.BOTH, expand=True)

        self.listbox = tk.Listbox(list_frame, bg="#151520", fg="#E0E0E0", selectbackground="#00E5FF",
                                  selectforeground="#000000", font=("Consolas", 9), relief=tk.FLAT, bd=0)
        self.listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.listbox.bind("<Double-Button-1>", lambda e: self._load_selected_image())

        scroll = tk.Scrollbar(list_frame, command=self.listbox.yview)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.listbox.config(yscrollcommand=scroll.set)

        row_list_btn = tk.Frame(save_group, bg="#181822")
        row_list_btn.pack(fill=tk.X, pady=4)

        btn_load = tk.Button(row_list_btn, text="📂 Mở Ảnh Chọn", bg="#252538", fg="#00E5FF",
                             font=("Segoe UI", 8, "bold"), relief=tk.FLAT, padx=8, pady=2,
                             command=self._load_selected_image)
        btn_load.pack(side=tk.LEFT, padx=2)

        btn_del = tk.Button(row_list_btn, text="🗑️ Xóa Ảnh Chọn", bg="#3A1D28", fg="#FF5252",
                            font=("Segoe UI", 8), relief=tk.FLAT, padx=8, pady=2,
                            command=self._delete_selected_image)
        btn_del.pack(side=tk.LEFT, padx=2)

        btn_refresh = tk.Button(row_list_btn, text="🔄 Làm Mới", bg="#252538", fg="#E0E0E0",
                                font=("Segoe UI", 8), relief=tk.FLAT, padx=8, pady=2,
                                command=self._refresh_saved_list)
        btn_refresh.pack(side=tk.RIGHT)

        # Trạng thái dưới cùng
        self.status_lbl = tk.Label(self.root, text=f"Thư mục lưu ảnh: {SAVED_DIR}", font=("Segoe UI", 8),
                                   fg="#75758A", bg="#121218", pady=4, anchor=tk.W, padx=16)
        self.status_lbl.pack(fill=tk.X)

        self._redraw_canvas()

    def _on_brush_change(self):
        self.brush_size = self.brush_var.get()

    def _on_mouse_down_draw(self, event):
        self.draw_color = 1
        self._apply_brush_at(event.x, event.y, 1)
        self.last_draw_pos = (event.x, event.y)

    def _on_mouse_down_erase(self, event):
        self.draw_color = 0
        self._apply_brush_at(event.x, event.y, 0)
        self.last_draw_pos = (event.x, event.y)

    def _on_mouse_drag_draw(self, event):
        if self.last_draw_pos:
            self._draw_line(self.last_draw_pos[0], self.last_draw_pos[1], event.x, event.y, 1)
        else:
            self._apply_brush_at(event.x, event.y, 1)
        self.last_draw_pos = (event.x, event.y)

    def _on_mouse_drag_erase(self, event):
        if self.last_draw_pos:
            self._draw_line(self.last_draw_pos[0], self.last_draw_pos[1], event.x, event.y, 0)
        else:
            self._apply_brush_at(event.x, event.y, 0)
        self.last_draw_pos = (event.x, event.y)

    def _on_mouse_up(self, event):
        self.last_draw_pos = None
        self._push_history()

    def _draw_line(self, x0, y0, x1, y1, color):
        """Vẽ đường liên tục theo thuật toán Bresenham để không bị đứt nét khi rê nhanh."""
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy

        while True:
            self._apply_brush_at(x0, y0, color)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def _apply_brush_at(self, pixel_x, pixel_y, color):
        """Tô các ô pixel tương ứng theo kích cỡ bút."""
        grid_x = pixel_x // SCALE
        grid_y = pixel_y // SCALE
        radius = self.brush_size - 1

        updated = False
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                gx = grid_x + dx
                gy = grid_y + dy
                if 0 <= gx < WIDTH and 0 <= gy < HEIGHT:
                    if self.grid_data[gy][gx] != color:
                        self.grid_data[gy][gx] = color
                        self._draw_cell(gx, gy, color)
                        updated = True
        if updated:
            self._update_previews()

    def _draw_cell(self, gx, gy, color):
        """Vẽ lại 1 ô pixel đơn lẻ trên Canvas."""
        x0 = gx * SCALE
        y0 = gy * SCALE
        x1 = x0 + SCALE
        y1 = y0 + SCALE

        tag = f"cell_{gx}_{gy}"
        self.canvas.delete(tag)

        fill_color = self._get_draw_color_hex() if color == 1 else "#000000"
        self.canvas.create_rectangle(x0, y0, x1, y1, fill=fill_color, outline="", tags=tag)

        # Lưới mờ
        if self.show_grid.get():
            self.canvas.create_rectangle(x0, y0, x1, y1, fill="", outline="#1E1E2A", tags=f"grid_{gx}_{gy}")

    def _redraw_canvas(self):
        """Vẽ lại toàn bộ Canvas 64x64."""
        self.canvas.delete("all")
        active_color = self._get_draw_color_hex()
        grid_on = self.show_grid.get()

        for gy in range(HEIGHT):
            for gx in range(WIDTH):
                color = self.grid_data[gy][gx]
                fill_color = active_color if color == 1 else "#000000"
                x0 = gx * SCALE
                y0 = gy * SCALE
                x1 = x0 + SCALE
                y1 = y0 + SCALE
                outline = "#1A1A24" if grid_on else ""
                self.canvas.create_rectangle(x0, y0, x1, y1, fill=fill_color, outline=outline, tags=f"cell_{gx}_{gy}")
        self._update_previews()

    def _update_previews(self):
        """Cập nhật 2 khung preview bên phải."""
        # 1. Preview 64x64 (True size)
        img_64 = Image.new("RGB", (WIDTH, HEIGHT), (0, 0, 0))
        pixels_64 = img_64.load()
        active_rgb = (0, 229, 255) if self.neon_mode.get() else (255, 255, 255)

        for y in range(HEIGHT):
            for x in range(WIDTH):
                if self.grid_data[y][x] == 1:
                    pixels_64[x, y] = active_rgb

        self.photo_64 = ImageTk.PhotoImage(img_64)
        self.preview_64.config(image=self.photo_64)

        # 2. Preview 128x64 OLED Simulator (Ảnh 64x64 căn giữa: offset_x = 32)
        img_oled = Image.new("RGB", (128, 64), (5, 5, 10))
        pixels_oled = img_oled.load()
        offset_x = 32
        for y in range(HEIGHT):
            for x in range(WIDTH):
                if self.grid_data[y][x] == 1:
                    pixels_oled[offset_x + x, y] = active_rgb

        # Khung viền mỏng giống OLED
        for x in range(31, 31 + 66):
            pixels_oled[x, 0] = (40, 40, 60)
            pixels_oled[x, 63] = (40, 40, 60)
        for y in range(0, 64):
            pixels_oled[31, y] = (40, 40, 60)
            pixels_oled[31 + 65, y] = (40, 40, 60)

        self.photo_oled = ImageTk.PhotoImage(img_oled)
        self.preview_oled.config(image=self.photo_oled)

    def _clear_canvas(self):
        for y in range(HEIGHT):
            for x in range(WIDTH):
                self.grid_data[y][x] = 0
        self._redraw_canvas()
        self._push_history()

    def _invert_colors(self):
        for y in range(HEIGHT):
            for x in range(WIDTH):
                self.grid_data[y][x] = 1 - self.grid_data[y][x]
        self._redraw_canvas()
        self._push_history()

    def _shift_image(self, dx, dy):
        new_grid = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]
        for y in range(HEIGHT):
            for x in range(WIDTH):
                nx = x + dx
                ny = y + dy
                if 0 <= nx < WIDTH and 0 <= ny < HEIGHT:
                    new_grid[ny][nx] = self.grid_data[y][x]
        self.grid_data = new_grid
        self._redraw_canvas()
        self._push_history()

    def _flip_h(self):
        for y in range(HEIGHT):
            self.grid_data[y] = self.grid_data[y][::-1]
        self._redraw_canvas()
        self._push_history()

    def _flip_v(self):
        self.grid_data = self.grid_data[::-1]
        self._redraw_canvas()
        self._push_history()

    def _push_history(self):
        # Lưu bản sao cho Undo
        snapshot = [row[:] for row in self.grid_data]
        if self.history_index < len(self.history) - 1:
            self.history = self.history[:self.history_index + 1]
        self.history.append(snapshot)
        if len(self.history) > 30:
            self.history.pop(0)
        self.history_index = len(self.history) - 1

    def _undo(self):
        if self.history_index > 0:
            self.history_index -= 1
            self.grid_data = [row[:] for row in self.history[self.history_index]]
            self._redraw_canvas()

    def _redo(self):
        if self.history_index < len(self.history) - 1:
            self.history_index += 1
            self.grid_data = [row[:] for row in self.history[self.history_index]]
            self._redraw_canvas()

    def _to_bytes(self) -> bytes:
        """Chuyển ma trận 64x64 bit thành 512 bytes chuẩn MSB-first."""
        data = bytearray(TOTAL_BYTES)
        for y in range(HEIGHT):
            for byte_col in range(WIDTH // 8):
                byte_val = 0
                for bit in range(8):
                    pixel = self.grid_data[y][byte_col * 8 + bit]
                    if pixel:
                        byte_val |= (0x80 >> bit)
                data[y * (WIDTH // 8) + byte_col] = byte_val
        return bytes(data)

    def _from_bytes(self, raw_bytes: bytes):
        """Khôi phục ma trận 64x64 từ 512 bytes."""
        new_grid = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]
        for y in range(HEIGHT):
            for byte_col in range(WIDTH // 8):
                idx = y * (WIDTH // 8) + byte_col
                b = raw_bytes[idx] if idx < len(raw_bytes) else 0
                for bit in range(8):
                    new_grid[y][byte_col * 8 + bit] = 1 if (b & (0x80 >> bit)) else 0
        self.grid_data = new_grid
        self._redraw_canvas()
        self._push_history()

    def _save_image(self):
        """Lưu đồng thời .png, .bin, .h vào thư mục saved_images/."""
        name = self.entry_name.get().strip()
        if not name:
            name = f"anh_{datetime.now().strftime('%Y%m%d_%H%M%S')}"

        # Làm sạch tên file an toàn
        safe_name = "".join(c for c in name if c.isalnum() or c in ('_', '-')).strip()
        if not safe_name:
            safe_name = f"anh_{datetime.now().strftime('%Y%m%d_%H%M%S')}"

        png_path = os.path.join(SAVED_DIR, f"{safe_name}.png")
        bin_path = os.path.join(SAVED_DIR, f"{safe_name}.bin")
        h_path = os.path.join(SAVED_DIR, f"{safe_name}.h")

        raw_bytes = self._to_bytes()

        # 1. Lưu file PNG 64x64 (chuẩn 1-bit đen trắng)
        img_bw = Image.new("1", (WIDTH, HEIGHT), 0)
        pixels = img_bw.load()
        for y in range(HEIGHT):
            for x in range(WIDTH):
                pixels[x, y] = self.grid_data[y][x]
        img_bw.save(png_path)

        # 2. Lưu file BIN 512 bytes
        with open(bin_path, "wb") as f:
            f.write(raw_bytes)

        # 3. Lưu file Header C (.h)
        c_array_str = ",\n    ".join(
            ", ".join(f"0x{b:02X}" for b in raw_bytes[i:i+16])
            for i in range(0, len(raw_bytes), 16)
        )
        c_content = f"""// Tự động tạo bởi Bảng Vẽ Pixel 64x64 - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// Kích thước: 64x64 pixels | 512 bytes | 1-bit Monochrome
#ifndef IMAGE_{safe_name.upper()}_H
#define IMAGE_{safe_name.upper()}_H

#include <stdint.h>

static const uint8_t PROGMEM g_image_{safe_name}[512] = {{
    {c_array_str}
}};

#endif // IMAGE_{safe_name.upper()}_H
"""
        with open(h_path, "w", encoding="utf-8") as f:
            f.write(c_content)

        self._refresh_saved_list()
        messagebox.showinfo(
            "Đã Lưu Thành Công",
            f"Ảnh đã được lưu vào thư mục saved_images:\n\n"
            f"✔ {safe_name}.png (Ảnh xem)\n"
            f"✔ {safe_name}.bin (512 bytes dữ liệu)\n"
            f"✔ {safe_name}.h   (Mảng mã C)"
        )

    def _refresh_saved_list(self):
        """Quét lại thư mục saved_images/ để hiển thị danh sách."""
        self.listbox.delete(0, tk.END)
        png_files = sorted(glob.glob(os.path.join(SAVED_DIR, "*.png")), key=os.path.getmtime, reverse=True)
        for p in png_files:
            fname = os.path.basename(p)
            base_name, _ = os.path.splitext(fname)
            mtime = datetime.fromtimestamp(os.path.getmtime(p)).strftime("%d/%m %H:%M")
            self.listbox.insert(tk.END, f"{base_name:<24} [{mtime}]")

    def _load_selected_image(self):
        """Mở ảnh đang chọn từ Listbox và vẽ lại lên Canvas."""
        sel = self.listbox.curselection()
        if not sel:
            return
        item_text = self.listbox.get(sel[0])
        base_name = item_text.split("[")[0].strip()

        bin_path = os.path.join(SAVED_DIR, f"{base_name}.bin")
        png_path = os.path.join(SAVED_DIR, f"{base_name}.png")

        if os.path.exists(bin_path):
            with open(bin_path, "rb") as f:
                data = f.read()
                if len(data) >= TOTAL_BYTES:
                    self._from_bytes(data[:TOTAL_BYTES])
                    self.entry_name.delete(0, tk.END)
                    self.entry_name.insert(0, base_name)
                    return

        if os.path.exists(png_path):
            img = Image.open(png_path).convert("L")
            if img.size != (WIDTH, HEIGHT):
                img = img.resize((WIDTH, HEIGHT))
            pixels = img.load()
            new_grid = [[1 if pixels[x, y] > 127 else 0 for x in range(WIDTH)] for y in range(HEIGHT)]
            self.grid_data = new_grid
            self._redraw_canvas()
            self._push_history()
            self.entry_name.delete(0, tk.END)
            self.entry_name.insert(0, base_name)

    def _delete_selected_image(self):
        sel = self.listbox.curselection()
        if not sel:
            return
        item_text = self.listbox.get(sel[0])
        base_name = item_text.split("[")[0].strip()

        if messagebox.askyesno("Xác nhận xóa", f"Bạn có chắc muốn xóa ảnh '{base_name}' khỏi saved_images không?"):
            for ext in [".png", ".bin", ".h"]:
                fpath = os.path.join(SAVED_DIR, f"{base_name}{ext}")
                if os.path.exists(fpath):
                    try:
                        os.remove(fpath)
                    except:
                        pass
            self._refresh_saved_list()

    def _open_saved_folder(self):
        """Mở thư mục saved_images trên Windows Explorer."""
        if os.path.exists(SAVED_DIR):
            os.startfile(SAVED_DIR)

def main():
    root = tk.Tk()
    app = PixelCanvasApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
