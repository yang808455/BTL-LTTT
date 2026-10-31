#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
HỆ THỐNG MÃ HÓA KÊNH TỰ THÍCH NGHI TRUYỀN ẢNH BITMAP TRÊN 3 BO ESP32-C3
Bảng điều khiển Desktop trực quan hóa đa luồng thời gian thực (PyQt6 Dashboard)
Giao diện Dark Theme độ tương phản cao, chữ trắng rõ nét 100%.
"""

import sys
import os
import json
import time
import csv
from datetime import datetime
from collections import deque

import serial
import serial.tools.list_ports
from PyQt6 import QtCore, QtGui, QtWidgets
from PyQt6.QtCore import Qt, pyqtSignal, pyqtSlot, QThread, QTimer

import matplotlib
matplotlib.use('QtAgg')
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

# Bitmap 64x64 mẫu mặc định (Ảnh 1 & Ảnh 2) để hiển thị máy phát
SAMPLE_IMAGE_SMILE = bytes([
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 
    0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x80, 0x00, 0xFE, 0x00, 0x00, 
    0x00, 0x00, 0xFE, 0x00, 0x00, 0x3F, 0x80, 0x00, 0x00, 0x01, 0xF0, 0x00, 0x00, 0x07, 0xC0, 0x00, 
    0x00, 0x03, 0xE0, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0xF8, 0x00, 
    0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 
    0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 
    0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 
    0x01, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC0, 
    0x03, 0x80, 0x02, 0x00, 0x00, 0x20, 0x00, 0xE0, 0x03, 0x80, 0x0F, 0x80, 0x00, 0xF8, 0x00, 0xE0, 
    0x07, 0x80, 0x1F, 0xC0, 0x01, 0xFC, 0x00, 0xF0, 0x07, 0x00, 0x1F, 0xC0, 0x01, 0xFC, 0x00, 0x70, 
    0x07, 0x00, 0x3F, 0xE0, 0x03, 0xFE, 0x00, 0x70, 0x0E, 0x00, 0x1F, 0xC0, 0x01, 0xFC, 0x00, 0x38, 
    0x0E, 0x00, 0x1F, 0xC0, 0x01, 0xFC, 0x00, 0x38, 0x0E, 0x00, 0x0F, 0x80, 0x00, 0xF8, 0x00, 0x38, 
    0x0E, 0x00, 0x02, 0x00, 0x00, 0x20, 0x00, 0x38, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 
    0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 
    0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 
    0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 
    0x0E, 0x00, 0xF0, 0x00, 0x00, 0x07, 0x80, 0x38, 0x0E, 0x00, 0xF0, 0x00, 0x00, 0x07, 0x80, 0x38, 
    0x0E, 0x00, 0xF8, 0x00, 0x00, 0x0F, 0x80, 0x38, 0x0E, 0x00, 0x7C, 0x00, 0x00, 0x1F, 0x00, 0x38, 
    0x07, 0x00, 0x3E, 0x00, 0x00, 0x3E, 0x00, 0x70, 0x07, 0x00, 0x3F, 0x00, 0x00, 0x7E, 0x00, 0x70, 
    0x07, 0x80, 0x1F, 0x80, 0x00, 0xFC, 0x00, 0xF0, 0x03, 0x80, 0x0F, 0xE0, 0x03, 0xF8, 0x00, 0xE0, 
    0x03, 0x80, 0x07, 0xFF, 0xFF, 0xF0, 0x00, 0xE0, 0x01, 0xC0, 0x01, 0xFF, 0xFF, 0xC0, 0x01, 0xC0, 
    0x01, 0xE0, 0x00, 0xFF, 0xFF, 0x80, 0x03, 0xC0, 0x00, 0xE0, 0x00, 0x1F, 0xFC, 0x00, 0x03, 0x80, 
    0x00, 0xF0, 0x00, 0x00, 0x80, 0x00, 0x07, 0x80, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 
    0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 
    0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0xF8, 0x00, 
    0x00, 0x03, 0xE0, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x01, 0xF0, 0x00, 0x00, 0x07, 0xC0, 0x00, 
    0x00, 0x00, 0xFE, 0x00, 0x00, 0x3F, 0x80, 0x00, 0x00, 0x00, 0x3F, 0x80, 0x00, 0xFE, 0x00, 0x00, 
    0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
])

SAMPLE_IMAGE_SCORE10 = bytes([
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x3E, 0x00, 0x00, 0xF8, 0x00, 0x0C, 0x30, 0x00, 0x3E, 0x00, 0x03, 0xFE, 0x00, 0x0C, 
    0x30, 0x00, 0x3E, 0x00, 0x0F, 0xFF, 0x80, 0x0C, 0x30, 0x00, 0x3E, 0x00, 0x1F, 0xFF, 0xC0, 0x0C, 
    0x30, 0x03, 0xFE, 0x00, 0x3F, 0x07, 0xE0, 0x0C, 0x30, 0x03, 0xFE, 0x00, 0x3C, 0x01, 0xE0, 0x0C, 
    0x30, 0x03, 0xFE, 0x00, 0x78, 0x00, 0xF0, 0x0C, 0x30, 0x03, 0xFE, 0x00, 0xF0, 0x00, 0x78, 0x0C, 
    0x30, 0x03, 0xFE, 0x00, 0xE0, 0x00, 0x38, 0x0C, 0x30, 0x00, 0x3E, 0x00, 0xE0, 0x00, 0x38, 0x0C, 
    0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 
    0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 
    0x30, 0x00, 0x3E, 0x01, 0x80, 0x00, 0x0C, 0x0C, 0x30, 0x00, 0x3E, 0x03, 0x80, 0x00, 0x0E, 0x0C, 
    0x30, 0x00, 0x3E, 0x03, 0x80, 0x00, 0x0E, 0x0C, 0x30, 0x00, 0x3E, 0x03, 0x80, 0x00, 0x0E, 0x0C, 
    0x30, 0x00, 0x3E, 0x01, 0x80, 0x00, 0x0C, 0x0C, 0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 
    0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 
    0x30, 0x00, 0x3E, 0x01, 0xC0, 0x00, 0x1C, 0x0C, 0x30, 0x00, 0x3E, 0x00, 0xE0, 0x00, 0x38, 0x0C, 
    0x30, 0x00, 0x3E, 0x00, 0xE0, 0x00, 0x38, 0x0C, 0x30, 0x00, 0x3E, 0x00, 0xF0, 0x00, 0x78, 0x0C, 
    0x30, 0x00, 0x3E, 0x00, 0x78, 0x00, 0xF0, 0x0C, 0x30, 0x00, 0x3E, 0x00, 0x3C, 0x01, 0xE0, 0x0C, 
    0x30, 0x00, 0x3E, 0x00, 0x3F, 0x07, 0xE0, 0x0C, 0x30, 0x0F, 0xFF, 0xE0, 0x1F, 0xFF, 0xC0, 0x0C, 
    0x30, 0x0F, 0xFF, 0xE0, 0x0F, 0xFF, 0x80, 0x0C, 0x30, 0x0F, 0xFF, 0xE0, 0x03, 0xFE, 0x00, 0x0C, 
    0x30, 0x0F, 0xFF, 0xE0, 0x00, 0xF8, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0C, 0x30, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0C, 
    0x30, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0C, 0x30, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 
])

def bytes_to_qimage(raw_bytes: bytes, width=64, height=64) -> QtGui.QImage:
    """Chuyển 512 bytes bitmap 1-bit thành QImage 64x64 thang độ sáng đen trắng."""
    img = QtGui.QImage(width, height, QtGui.QImage.Format.Format_Mono)
    img.setColorTable([0xFF000000, 0xFF00FFCC])  # 0: Đen, 1: Xanh Neon công nghệ
    for y in range(height):
        for byte_col in range(width // 8):
            idx = y * (width // 8) + byte_col
            b = raw_bytes[idx] if idx < len(raw_bytes) else 0
            for bit in range(8):
                val = 1 if (b & (0x80 >> bit)) else 0
                img.setPixel(byte_col * 8 + bit, y, val)
    return img

def show_custom_msg(parent, title: str, text: str, icon_type="info"):
    """Hiển thị hộp thoại thông báo với màu sắc tương phản cao, chữ trắng rõ nét 100%."""
    msg = QtWidgets.QMessageBox(parent)
    msg.setWindowTitle(title)
    msg.setText(text)
    if icon_type == "info":
        msg.setIcon(QtWidgets.QMessageBox.Icon.Information)
    elif icon_type == "warning":
        msg.setIcon(QtWidgets.QMessageBox.Icon.Warning)
    elif icon_type == "error":
        msg.setIcon(QtWidgets.QMessageBox.Icon.Critical)

    msg.setStyleSheet("""
        QDialog, QMessageBox {
            background-color: #1A1A26 !important;
            border: 2px solid #00E5FF;
            border-radius: 6px;
        }
        QLabel {
            color: #FFFFFF !important;
            font-size: 13px !important;
            font-weight: bold !important;
            padding: 10px;
            background: transparent !important;
        }
        QPushButton {
            background-color: #007ACC !important;
            color: #FFFFFF !important;
            font-weight: bold !important;
            font-size: 12px !important;
            border: 1px solid #0099FF;
            border-radius: 4px;
            padding: 8px 24px;
            min-width: 75px;
        }
        QPushButton:hover {
            background-color: #0099FF !important;
        }
    """)
    return msg.exec()

class SerialReaderThread(QThread):
    """Tiểu trình đọc cổng nối tiếp chuyên dụng, không làm đơ giao diện chính."""
    line_received = pyqtSignal(str, str)   # (node_type, line)
    status_changed = pyqtSignal(str, bool, str) # (node_type, is_connected, port_name)

    def __init__(self, node_type: str):
        super().__init__()
        self.node_type = node_type
        self.port_name = None
        self.baudrate = 115200
        self.serial_port = None
        self._is_running = False

    def connect_port(self, port_name: str, baudrate=115200):
        self.port_name = port_name
        self.baudrate = baudrate
        self._is_running = True
        self.start()

    def disconnect_port(self):
        self._is_running = False
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.close()
            except Exception:
                pass
        self.wait(1000)
        self.status_changed.emit(self.node_type, False, self.port_name or "")

    def send_command(self, cmd_str: str):
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.write((cmd_str + "\n").encode('utf-8'))
                self.serial_port.flush()
            except Exception as e:
                print(f"Error sending command to {self.node_type}: {e}")

    def run(self):
        try:
            self.serial_port = serial.Serial(self.port_name, self.baudrate, timeout=0.2)
            self.status_changed.emit(self.node_type, True, self.port_name)
        except Exception as e:
            self.status_changed.emit(self.node_type, False, f"Lỗi: {e}")
            return

        while self._is_running and self.serial_port and self.serial_port.is_open:
            try:
                if self.serial_port.in_waiting > 0:
                    raw_line = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                    if raw_line:
                        self.line_received.emit(self.node_type, raw_line)
                else:
                    self.msleep(10)
            except Exception as e:
                break

        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.close()
            except Exception:
                pass
        self.status_changed.emit(self.node_type, False, self.port_name or "")


class MplCanvas(FigureCanvas):
    """Khung vẽ biểu đồ matplotlib nhúng trong PyQt6."""
    def __init__(self, parent=None, width=5, height=3, dpi=100):
        self.fig = Figure(figsize=(width, height), dpi=dpi, facecolor='#16161E')
        self.axes = self.fig.add_subplot(111)
        self.axes.set_facecolor('#101016')
        self.axes.grid(True, linestyle='--', alpha=0.35, color='#777788')
        self.axes.tick_params(colors='#E0E0E0', labelsize=8)
        for spine in self.axes.spines.values():
            spine.set_color('#3E3E50')
        super().__init__(self.fig)


class ModernDashboardWindow(QtWidgets.QMainWindow):
    """Cửa sổ ứng dụng Dashboard quản lý 3 khối ESP32-C3 chuyên nghiệp."""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("HỆ THỐNG MÃ HÓA KÊNH TỰ THÍCH NGHI - ESP32-C3 ADAPTIVE IMAGE SYSTEM")
        self.resize(1380, 880)
        self.setMinimumSize(1100, 720)

        # Trạng thái dữ liệu
        self.tx_thread = SerialReaderThread("TX")
        self.ch_thread = SerialReaderThread("CH")
        self.rx_thread = SerialReaderThread("RX")

        self.rx_image_buffer = bytearray(512)
        self.ch_image_buffer = bytearray(512)
        self.current_tx_img_idx = 0
        self.ber_history = deque(maxlen=60)
        self.time_history = deque(maxlen=60)
        self.telemetry_logs = []

        self._init_ui()
        self._apply_dark_theme()
        self._wire_signals()

        # Quét cổng nối tiếp khi mở ứng dụng
        self.refresh_com_ports()

    def _init_ui(self):
        central_widget = QtWidgets.QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QtWidgets.QVBoxLayout(central_widget)
        main_layout.setContentsMargins(14, 10, 14, 10)
        main_layout.setSpacing(10)

        # 1. THANH CÔNG CỤ ĐỈNH (TOP TOOLBAR)
        top_bar = QtWidgets.QFrame()
        top_bar.setObjectName("topBar")
        top_layout = QtWidgets.QHBoxLayout(top_bar)
        top_layout.setContentsMargins(12, 8, 12, 8)

        title_label = QtWidgets.QLabel("🛰️ BẢNG ĐIỀU KHIỂN & ĐÁNH GIÁ THÍCH NGHI KÊNH (3x ESP32-C3)")
        title_label.setStyleSheet("font-size: 16px; font-weight: bold; color: #00FFCC;")
        top_layout.addWidget(title_label)

        top_layout.addStretch()

        self.btn_auto_scan = QtWidgets.QPushButton("⚡ Tự Động Quét & Nhận Diện 3 Bo")
        self.btn_auto_scan.setFixedHeight(34)
        self.btn_auto_scan.setStyleSheet("background-color: #007ACC; color: #FFFFFF; font-weight: bold; border-radius: 4px; padding: 0 14px;")
        top_layout.addWidget(self.btn_auto_scan)

        self.btn_refresh_ports = QtWidgets.QPushButton("🔄 Quét lại Cổng")
        self.btn_refresh_ports.setFixedHeight(34)
        top_layout.addWidget(self.btn_refresh_ports)

        self.btn_reset_all = QtWidgets.QPushButton("🧹 Đặt Lại Thống Kê")
        self.btn_reset_all.setFixedHeight(34)
        top_layout.addWidget(self.btn_reset_all)

        self.btn_export_csv = QtWidgets.QPushButton("📥 Xuất File CSV")
        self.btn_export_csv.setFixedHeight(34)
        self.btn_export_csv.setStyleSheet("background-color: #2E7D32; color: #FFFFFF; font-weight: bold; border-radius: 4px; padding: 0 14px;")
        top_layout.addWidget(self.btn_export_csv)

        main_layout.addWidget(top_bar)

        # 2. KHU VỰC 3 CỘT NÚT MẠNG CHÍNH (TX -> CHANNEL -> RX)
        columns_layout = QtWidgets.QHBoxLayout()
        columns_layout.setSpacing(12)

        # === CỘT 1: MÁY PHÁT (TX) ===
        col_tx = self._build_tx_column()
        columns_layout.addWidget(col_tx, 1)

        # === CỘT 2: KÊNH TRUYỀN (CHANNEL) ===
        col_ch = self._build_channel_column()
        columns_layout.addWidget(col_ch, 1)

        # === CỘT 3: MÁY THU (RX) ===
        col_rx = self._build_rx_column()
        columns_layout.addWidget(col_rx, 1)

        main_layout.addLayout(columns_layout, 7)

        # 3. KHU VỰC NHẬT KÝ THỜI GIAN THỰC (BOTTOM LOG CONSOLE)
        bottom_box = QtWidgets.QGroupBox("📋 Nhật Ký Vi Điều Khiển & Phản Hồi Telemetry")
        bottom_box.setStyleSheet("QGroupBox { font-weight: bold; color: #00FFCC; }")
        bottom_layout = QtWidgets.QVBoxLayout(bottom_box)
        bottom_layout.setContentsMargins(8, 8, 8, 8)

        self.log_table = QtWidgets.QTableWidget()
        self.log_table.setColumnCount(6)
        self.log_table.setHorizontalHeaderLabels(["Thời Gian", "Nút", "Sự Kiện", "SEQ / Khung", "Chế Độ (Mode)", "Chi Tiết Thông Số"])
        self.log_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeMode.ResizeToContents)
        self.log_table.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeMode.ResizeToContents)
        self.log_table.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeMode.ResizeToContents)
        self.log_table.horizontalHeader().setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeMode.ResizeToContents)
        self.log_table.horizontalHeader().setSectionResizeMode(4, QtWidgets.QHeaderView.ResizeMode.ResizeToContents)
        self.log_table.horizontalHeader().setSectionResizeMode(5, QtWidgets.QHeaderView.ResizeMode.Stretch)
        self.log_table.verticalHeader().setVisible(False)
        self.log_table.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectionBehavior.SelectRows)
        self.log_table.setStyleSheet("font-family: Consolas, monospace; font-size: 11px;")
        bottom_layout.addWidget(self.log_table)

        main_layout.addWidget(bottom_box, 3)

    def _build_tx_column(self) -> QtWidgets.QGroupBox:
        box = QtWidgets.QGroupBox("1. MÁY PHÁT (TRANSMITTER - TX)")
        layout = QtWidgets.QVBoxLayout(box)
        layout.setSpacing(8)

        # Header Cổng nối tiếp & Kết nối
        port_frame = QtWidgets.QHBoxLayout()
        self.cb_tx_port = QtWidgets.QComboBox()
        self.btn_tx_connect = QtWidgets.QPushButton("Kết Nối")
        self.lbl_tx_status = QtWidgets.QLabel("⚪ Ngắt kết nối")
        self.lbl_tx_status.setStyleSheet("color: #AAAAAA; font-weight: bold;")
        port_frame.addWidget(QtWidgets.QLabel("Cổng:"))
        port_frame.addWidget(self.cb_tx_port, 1)
        port_frame.addWidget(self.btn_tx_connect)
        layout.addLayout(port_frame)
        layout.addWidget(self.lbl_tx_status)

        # Khung hiển thị ảnh 64x64 gốc
        img_container = QtWidgets.QHBoxLayout()
        self.lbl_tx_img = QtWidgets.QLabel()
        self.lbl_tx_img.setFixedSize(160, 160)
        self.lbl_tx_img.setStyleSheet("background-color: #000000; border: 2px solid #00FFCC; border-radius: 4px;")
        self.lbl_tx_img.setAlignment(Qt.AlignmentFlag.AlignCenter)
        img_container.addStretch()
        img_container.addWidget(self.lbl_tx_img)
        img_container.addStretch()
        layout.addLayout(img_container)

        self._render_tx_image(0)

        # Nút chuyển ảnh
        btn_img_layout = QtWidgets.QHBoxLayout()
        self.btn_img_smile = QtWidgets.QPushButton("Ảnh 1: Mặt Cười")
        self.btn_img_score10 = QtWidgets.QPushButton("Ảnh 2: Điểm 10")
        btn_img_layout.addWidget(self.btn_img_smile)
        btn_img_layout.addWidget(self.btn_img_score10)
        layout.addLayout(btn_img_layout)

        # Bảng thông số phát
        form = QtWidgets.QFormLayout()
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)

        self.lbl_tx_mode = QtWidgets.QLabel("DIRECT (0.0% BER)")
        self.lbl_tx_mode.setStyleSheet("font-weight: bold; color: #00E676; font-size: 13px;")
        self.lbl_tx_seq = QtWidgets.QLabel("0")
        self.lbl_tx_seq.setStyleSheet("font-weight: bold; color: #FFFFFF;")
        self.lbl_tx_chunk = QtWidgets.QLabel("0 / 8")
        self.lbl_tx_chunk.setStyleSheet("font-weight: bold; color: #FFFFFF;")
        self.lbl_tx_cr = QtWidgets.QLabel("1.00x")
        self.lbl_tx_cr.setStyleSheet("font-weight: bold; color: #FFFFFF;")
        self.lbl_tx_speed = QtWidgets.QLabel("500 ms / khung")
        self.lbl_tx_speed.setStyleSheet("color: #FFFFFF;")

        form.addRow("Chế Độ Mã Kênh:", self.lbl_tx_mode)
        form.addRow("Bộ Đếm Khung (SEQ):", self.lbl_tx_seq)
        form.addRow("Mảnh Ảnh Đang Gửi:", self.lbl_tx_chunk)
        form.addRow("Hệ Số Nén (CR):", self.lbl_tx_cr)
        form.addRow("Chu Kỳ Phát:", self.lbl_tx_speed)
        layout.addLayout(form)

        # Điều khiển cưỡng bức chế độ
        layout.addWidget(QtWidgets.QLabel("Chế Độ Mã Kênh (Tự động / Thủ công):"))
        mode_box = QtWidgets.QHBoxLayout()
        self.btn_mode_auto = QtWidgets.QPushButton("🔄 Auto")
        self.btn_mode_auto.setStyleSheet("background: #00B0FF; color: black; font-weight: bold;")
        self.btn_mode_direct = QtWidgets.QPushButton("Direct (Tắt)")
        self.btn_mode_hamming = QtWidgets.QPushButton("Hamming")
        self.btn_mode_cyclic = QtWidgets.QPushButton("Cyclic")
        mode_box.addWidget(self.btn_mode_auto)
        mode_box.addWidget(self.btn_mode_direct)
        mode_box.addWidget(self.btn_mode_hamming)
        mode_box.addWidget(self.btn_mode_cyclic)
        layout.addLayout(mode_box)

        layout.addStretch()
        return box

    def _build_channel_column(self) -> QtWidgets.QGroupBox:
        box = QtWidgets.QGroupBox("2. KÊNH TRUYỀN (CHANNEL - BSC NOISE)")
        layout = QtWidgets.QVBoxLayout(box)
        layout.setSpacing(8)

        # Header Cổng nối tiếp & Kết nối
        port_frame = QtWidgets.QHBoxLayout()
        self.cb_ch_port = QtWidgets.QComboBox()
        self.btn_ch_connect = QtWidgets.QPushButton("Kết Nối")
        self.lbl_ch_status = QtWidgets.QLabel("⚪ Ngắt kết nối")
        self.lbl_ch_status.setStyleSheet("color: #AAAAAA; font-weight: bold;")
        port_frame.addWidget(QtWidgets.QLabel("Cổng:"))
        port_frame.addWidget(self.cb_ch_port, 1)
        port_frame.addWidget(self.btn_ch_connect)
        layout.addLayout(port_frame)
        layout.addWidget(self.lbl_ch_status)

        # Khung hiển thị ảnh 64x64 sau khi chịu tác động nhiễu Kênh
        img_container = QtWidgets.QHBoxLayout()
        self.lbl_ch_img = QtWidgets.QLabel()
        self.lbl_ch_img.setFixedSize(160, 160)
        self.lbl_ch_img.setStyleSheet("background-color: #000000; border: 2px solid #FFD600; border-radius: 4px;")
        self.lbl_ch_img.setAlignment(Qt.AlignmentFlag.AlignCenter)
        img_container.addStretch()
        img_container.addWidget(self.lbl_ch_img)
        img_container.addStretch()
        layout.addLayout(img_container)

        self._render_ch_image()

        # Đồng hồ đo BER hiện tại & Giá trị Biến trở ADC
        ber_display_box = QtWidgets.QVBoxLayout()
        self.lbl_ch_ber_val = QtWidgets.QLabel("0.0 %")
        self.lbl_ch_ber_val.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_ch_ber_val.setStyleSheet("font-size: 26px; font-weight: bold; color: #FFD600;")
        ber_display_box.addWidget(self.lbl_ch_ber_val)

        self.lbl_ch_pot_quick = QtWidgets.QLabel("Biến Trở: 0 / 4095  (0.00 V)")
        self.lbl_ch_pot_quick.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_ch_pot_quick.setStyleSheet("font-size: 13px; color: #00E5FF; font-weight: bold; padding: 2px 0;")
        ber_display_box.addWidget(self.lbl_ch_pot_quick)

        self.bar_ch_ber = QtWidgets.QProgressBar()
        self.bar_ch_ber.setRange(0, 120)  # 0.0% đến 12.0% (x10)
        self.bar_ch_ber.setValue(0)
        self.bar_ch_ber.setTextVisible(False)
        self.bar_ch_ber.setFixedHeight(14)
        ber_display_box.addWidget(self.bar_ch_ber)
        layout.addLayout(ber_display_box)

        # Biểu đồ Matplotlib động biểu diễn BER theo thời gian
        self.plot_canvas = MplCanvas(self, width=3.5, height=2.0, dpi=80)
        layout.addWidget(self.plot_canvas)

        # Thông số Kênh
        form = QtWidgets.QFormLayout()
        self.lbl_ch_frames = QtWidgets.QLabel("0")
        self.lbl_ch_frames.setStyleSheet("font-weight: bold; color: #FFFFFF;")
        self.lbl_ch_flipped = QtWidgets.QLabel("0 bit")
        self.lbl_ch_flipped.setStyleSheet("font-weight: bold; color: #FFFFFF;")
        self.lbl_ch_pot_adc = QtWidgets.QLabel("0 / 4095 (0.00 V)")
        self.lbl_ch_pot_adc.setStyleSheet("font-weight: bold; color: #00E5FF; font-size: 13px;")
        form.addRow("Tổng Khung Đã Forward:", self.lbl_ch_frames)
        form.addRow("Tổng Bit Nhiễu Đã Đảo:", self.lbl_ch_flipped)
        form.addRow("Biến Trở Vật Lý (ADC):", self.lbl_ch_pot_adc)
        layout.addLayout(form)

        # Điều khiển mức nhiễu ghi đè từ PC
        layout.addWidget(QtWidgets.QLabel("Điều Khiển Nhiễu (Override từ PC):"))
        slider_layout = QtWidgets.QHBoxLayout()
        self.slider_ber = QtWidgets.QSlider(Qt.Orientation.Horizontal)
        self.slider_ber.setRange(0, 120)
        self.slider_ber.setValue(0)
        slider_layout.addWidget(self.slider_ber)

        self.btn_ber_adc = QtWidgets.QPushButton("Trả Về Biến Trở ADC")
        self.btn_ber_adc.setStyleSheet("font-size: 11px;")
        slider_layout.addWidget(self.btn_ber_adc)
        layout.addLayout(slider_layout)

        layout.addStretch()
        return box

    def _build_rx_column(self) -> QtWidgets.QGroupBox:
        box = QtWidgets.QGroupBox("3. MÁY THU (RECEIVER - RX)")
        layout = QtWidgets.QVBoxLayout(box)
        layout.setSpacing(8)

        # Header Cổng nối tiếp & Kết nối
        port_frame = QtWidgets.QHBoxLayout()
        self.cb_rx_port = QtWidgets.QComboBox()
        self.btn_rx_connect = QtWidgets.QPushButton("Kết Nối")
        self.lbl_rx_status = QtWidgets.QLabel("⚪ Ngắt kết nối")
        self.lbl_rx_status.setStyleSheet("color: #AAAAAA; font-weight: bold;")
        port_frame.addWidget(QtWidgets.QLabel("Cổng:"))
        port_frame.addWidget(self.cb_rx_port, 1)
        port_frame.addWidget(self.btn_rx_connect)
        layout.addLayout(port_frame)
        layout.addWidget(self.lbl_rx_status)

        # Khung hiển thị ảnh 64x64 tái tạo
        img_container = QtWidgets.QHBoxLayout()
        self.lbl_rx_img = QtWidgets.QLabel()
        self.lbl_rx_img.setFixedSize(160, 160)
        self.lbl_rx_img.setStyleSheet("background-color: #000000; border: 2px solid #00FFCC; border-radius: 4px;")
        self.lbl_rx_img.setAlignment(Qt.AlignmentFlag.AlignCenter)
        img_container.addStretch()
        img_container.addWidget(self.lbl_rx_img)
        img_container.addStretch()
        layout.addLayout(img_container)

        self._render_rx_image()

        # Đèn LED chỉ thị Mode (Xanh lá / Vàng / Đỏ)
        led_container = QtWidgets.QHBoxLayout()
        self.lbl_led_direct = QtWidgets.QLabel("🟢 DIRECT")
        self.lbl_led_hamming = QtWidgets.QLabel("🟡 HAMMING")
        self.lbl_led_cyclic = QtWidgets.QLabel("🔴 CYCLIC")
        for lbl in (self.lbl_led_direct, self.lbl_led_hamming, self.lbl_led_cyclic):
            lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
            lbl.setStyleSheet("border-radius: 4px; padding: 4px 6px; background: #262632; color: #FFFFFF; font-weight: bold; font-size: 11px;")
            led_container.addWidget(lbl)
        layout.addLayout(led_container)

        # Bảng thông số thu và giải mã
        form = QtWidgets.QFormLayout()
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)

        self.lbl_rx_mode = QtWidgets.QLabel("DIRECT")
        self.lbl_rx_mode.setStyleSheet("font-weight: bold; color: #00E676; font-size: 13px;")
        self.lbl_rx_fixed_last = QtWidgets.QLabel("0 bit")
        self.lbl_rx_fixed_last.setStyleSheet("font-weight: bold; color: #FFFFFF;")
        self.lbl_rx_fixed_total = QtWidgets.QLabel("0 bit")
        self.lbl_rx_fixed_total.setStyleSheet("font-weight: bold; color: #FFFFFF;")
        self.lbl_rx_ber = QtWidgets.QLabel("0.0 %")
        self.lbl_rx_ber.setStyleSheet("font-weight: bold; color: #FFD600;")
        self.lbl_rx_rec_mode = QtWidgets.QLabel("DIRECT")
        self.lbl_rx_rec_mode.setStyleSheet("font-weight: bold; color: #FFFFFF;")

        form.addRow("Chế Độ Đang Giải Mã:", self.lbl_rx_mode)
        form.addRow("Bit Vừa Sửa (Khung gần nhất):", self.lbl_rx_fixed_last)
        form.addRow("Tổng Số Bit Đã Sửa:", self.lbl_rx_fixed_total)
        form.addRow("BER Đo Được:", self.lbl_rx_ber)
        form.addRow("Đề Xuất Phản Hồi Ngược:", self.lbl_rx_rec_mode)
        layout.addLayout(form)

        # Nút điều khiển CRC & Xóa màn hình thu
        rx_btn_layout = QtWidgets.QHBoxLayout()
        self.btn_toggle_crc = QtWidgets.QPushButton("🛡️ CRC: BẬT")
        self.btn_toggle_crc.setCheckable(True)
        self.btn_toggle_crc.setChecked(True)
        self.btn_toggle_crc.setStyleSheet("background: #00E676; color: #121218; font-weight: bold; border-radius: 4px; padding: 6px;")
        
        self.btn_clear_rx = QtWidgets.QPushButton("Xóa Bộ Đệm Ảnh")
        self.btn_clear_rx.setStyleSheet("background: #2D3748; color: #E2E8F0; border-radius: 4px; padding: 6px;")

        rx_btn_layout.addWidget(self.btn_toggle_crc)
        rx_btn_layout.addWidget(self.btn_clear_rx)
        layout.addLayout(rx_btn_layout)

        layout.addStretch()
        return box

    def _apply_dark_theme(self):
        style = """
        QMainWindow, QDialog, QMessageBox {
            background-color: #121218;
            color: #FFFFFF;
        }
        QWidget {
            color: #FFFFFF;
            font-family: 'Segoe UI', Tahoma, Arial, sans-serif;
            font-size: 12px;
        }
        QLabel {
            color: #E2E8F0;
            font-size: 12px;
        }
        QGroupBox {
            border: 1px solid #36364A;
            border-radius: 6px;
            margin-top: 14px;
            padding-top: 16px;
            font-size: 12px;
            font-weight: bold;
            color: #00E5FF;
            background-color: #1A1A24;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 8px;
            left: 10px;
        }
        QFrame#topBar {
            background-color: #181822;
            border-bottom: 2px solid #2B2B3C;
            border-radius: 6px;
        }
        QPushButton {
            background-color: #282838;
            color: #FFFFFF;
            border: 1px solid #44445A;
            border-radius: 4px;
            padding: 6px 12px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: #38384C;
            border-color: #00E5FF;
            color: #FFFFFF;
        }
        QPushButton:pressed {
            background-color: #1C1C28;
            color: #00E5FF;
        }
        QComboBox {
            background-color: #222230;
            color: #FFFFFF;
            border: 1px solid #44445A;
            border-radius: 4px;
            padding: 4px 8px;
            font-weight: 500;
        }
        QComboBox QAbstractItemView {
            background-color: #1A1A26;
            color: #FFFFFF;
            selection-background-color: #007ACC;
            selection-color: #FFFFFF;
        }
        QTableWidget {
            background-color: #121218;
            border: 1px solid #2B2B3C;
            gridline-color: #242434;
            color: #FFFFFF;
        }
        QHeaderView::section {
            background-color: #20202E;
            color: #00E5FF;
            padding: 5px;
            border: 1px solid #2B2B3C;
            font-weight: bold;
        }
        QProgressBar {
            background-color: #222230;
            border: 1px solid #3E3E54;
            border-radius: 4px;
        }
        QProgressBar::chunk {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00E5FF, stop:1 #FF3D00);
            border-radius: 3px;
        }
        QSlider::groove:horizontal {
            border: 1px solid #333348;
            height: 6px;
            background: #20202E;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #00E5FF;
            border: 1px solid #00B0FF;
            width: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QMessageBox {
            background-color: #1A1A26;
            border: 2px solid #00E5FF;
        }
        QMessageBox QLabel {
            color: #FFFFFF !important;
            font-size: 13px !important;
            font-weight: bold !important;
        }
        """
        # Áp dụng cho cả ứng dụng để mọi dialog và widget đều ăn giao diện
        app = QtWidgets.QApplication.instance()
        if app:
            app.setStyleSheet(style)
        else:
            self.setStyleSheet(style)

    def _wire_signals(self):
        # Nút top bar
        self.btn_refresh_ports.clicked.connect(self.refresh_com_ports)
        self.btn_auto_scan.clicked.connect(self.auto_scan_and_connect)
        self.btn_reset_all.clicked.connect(self.reset_all_stats)
        self.btn_export_csv.clicked.connect(self.export_csv_logs)

        # Cột Tx
        self.btn_tx_connect.clicked.connect(lambda: self.toggle_connect("TX"))
        self.btn_img_smile.clicked.connect(lambda: self.set_tx_image(0))
        self.btn_img_score10.clicked.connect(lambda: self.set_tx_image(1))
        self.btn_mode_auto.clicked.connect(self.set_tx_auto_adaptive)
        self.btn_mode_direct.clicked.connect(lambda: self.set_tx_mode(0))
        self.btn_mode_hamming.clicked.connect(lambda: self.set_tx_mode(1))
        self.btn_mode_cyclic.clicked.connect(lambda: self.set_tx_mode(2))

        # Cột Channel
        self.btn_ch_connect.clicked.connect(lambda: self.toggle_connect("CH"))
        self.slider_ber.valueChanged.connect(self.on_ber_slider_changed)
        self.btn_ber_adc.clicked.connect(self.release_ber_to_adc)

        # Cột Rx
        self.btn_rx_connect.clicked.connect(lambda: self.toggle_connect("RX"))
        self.btn_toggle_crc.clicked.connect(self.toggle_rx_crc)
        self.btn_clear_rx.clicked.connect(self.clear_rx_display)

        # Kết nối tín hiệu tiểu trình đọc Serial
        self.tx_thread.line_received.connect(self.handle_serial_line)
        self.tx_thread.status_changed.connect(self.handle_status_changed)

        self.ch_thread.line_received.connect(self.handle_serial_line)
        self.ch_thread.status_changed.connect(self.handle_status_changed)

        self.rx_thread.line_received.connect(self.handle_serial_line)
        self.rx_thread.status_changed.connect(self.handle_status_changed)

    def refresh_com_ports(self):
        """Liệt kê tất cả các cổng COM có trên máy."""
        ports = [p.device for p in serial.tools.list_ports.comports()]
        for cb in (self.cb_tx_port, self.cb_ch_port, self.cb_rx_port):
            cur = cb.currentText()
            cb.clear()
            cb.addItems(ports)
            if cur in ports:
                cb.setCurrentText(cur)

    def auto_scan_and_connect(self):
        """Gửi PING tự động nhận diện đúng bo TX, CH, RX vào từng cột."""
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if not ports:
            show_custom_msg(self, "Cảnh Báo", "Không tìm thấy bất kỳ cổng COM nào kết nối vào máy tính!", "warning")
            return

        self.btn_auto_scan.setEnabled(False)
        self.btn_auto_scan.setText("Đang dò tìm...")
        QtWidgets.QApplication.processEvents()

        identified = {}

        # 1. Kiểm tra các bo đã đang kết nối
        for node_type, th in [("TX", self.tx_thread), ("CH", self.ch_thread), ("RX", self.rx_thread)]:
            if th._is_running and th.port_name:
                identified[node_type] = th.port_name

        # 2. Quét các cổng chưa kết nối
        for port in ports:
            if port in identified.values():
                continue
            try:
                with serial.Serial(port, 115200, timeout=0.4, dsrdtr=False, rtscts=False) as s:
                    s.dtr = True
                    s.rts = True
                    time.sleep(0.15)
                    s.reset_input_buffer()
                    s.write(b'{"cmd":"PING"}\n')
                    s.flush()
                    
                    t_start = time.time()
                    while time.time() - t_start < 0.8:
                        if s.in_waiting > 0:
                            line = s.readline().decode('utf-8', errors='ignore').strip()
                            if '"node":"TX"' in line:
                                identified["TX"] = port
                                break
                            elif '"node":"CH"' in line:
                                identified["CH"] = port
                                break
                            elif '"node":"RX"' in line:
                                identified["RX"] = port
                                break
                        else:
                            time.sleep(0.02)
            except Exception:
                pass

        if "TX" in identified:
            self.cb_tx_port.setCurrentText(identified["TX"])
            if not self.tx_thread._is_running:
                self.tx_thread.connect_port(identified["TX"])

        if "CH" in identified:
            self.cb_ch_port.setCurrentText(identified["CH"])
            if not self.ch_thread._is_running:
                self.ch_thread.connect_port(identified["CH"])

        if "RX" in identified:
            self.cb_rx_port.setCurrentText(identified["RX"])
            if not self.rx_thread._is_running:
                self.rx_thread.connect_port(identified["RX"])

        self.btn_auto_scan.setEnabled(True)
        self.btn_auto_scan.setText("⚡ Tự Động Quét & Nhận Diện 3 Bo")

        tx_txt = identified.get('TX', 'Chưa tìm thấy')
        ch_txt = identified.get('CH', 'Chưa tìm thấy')
        rx_txt = identified.get('RX', 'Chưa tìm thấy')
        res_msg = f"ĐÃ HOÀN TẤT QUÉT CỔNG COM:\n\n• Máy Phát (TX): {tx_txt}\n• Kênh Truyền (CH): {ch_txt}\n• Máy Thu (RX): {rx_txt}"
        show_custom_msg(self, "Kết Quả Quét", res_msg, "info")

    def toggle_connect(self, node_type: str):
        thread_map = {"TX": self.tx_thread, "CH": self.ch_thread, "RX": self.rx_thread}
        cb_map = {"TX": self.cb_tx_port, "CH": self.cb_ch_port, "RX": self.cb_rx_port}
        th = thread_map[node_type]
        if th._is_running:
            th.disconnect_port()
        else:
            port = cb_map[node_type].currentText()
            if port:
                th.connect_port(port)

    @pyqtSlot(str, bool, str)
    def handle_status_changed(self, node_type: str, is_connected: bool, info: str):
        status_lbl = {"TX": self.lbl_tx_status, "CH": self.lbl_ch_status, "RX": self.lbl_rx_status}[node_type]
        btn = {"TX": self.btn_tx_connect, "CH": self.btn_ch_connect, "RX": self.btn_rx_connect}[node_type]

        if is_connected:
            status_lbl.setText(f"🟢 Đã kết nối ({info})")
            status_lbl.setStyleSheet("color: #00E676; font-weight: bold; font-size: 12px;")
            btn.setText("Ngắt")
        else:
            status_lbl.setText("⚪ Ngắt kết nối")
            status_lbl.setStyleSheet("color: #AAAAAA; font-weight: bold; font-size: 12px;")
            btn.setText("Kết Nối")

    @pyqtSlot(str, str)
    def handle_serial_line(self, node_type: str, line: str):
        """Xử lý dòng telemetry JSON từ từng vi điều khiển ESP32-C3."""
        if not line.startswith("{"):
            return

        try:
            data = json.loads(line)
        except Exception:
            return

        now_str = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        event = data.get("event", data.get("status", "TELEMETRY"))

        # Ghi log bộ nhớ
        self.telemetry_logs.append({
            "time": now_str,
            "node": node_type,
            "event": event,
            "raw": line
        })

        # Cập nhật hiển thị giao diện theo từng loại Nút
        if node_type == "TX":
            self._handle_tx_data(data)
        elif node_type == "CH":
            self._handle_ch_data(data)
        elif node_type == "RX":
            self._handle_rx_data(data)

        # Thêm vào bảng Log bên dưới
        self._append_log_table(now_str, node_type, event, data)

    def _handle_tx_data(self, d: dict):
        if "img_idx" in d:
            self._render_tx_image(d["img_idx"])
        if "chn_alg" in d:
            modes = {0: ("DIRECT", "#00E676"), 1: ("HAMMING (7,4)", "#FFD600"), 2: ("CYCLIC (7,4)", "#FF1744")}
            name, color = modes.get(d["chn_alg"], ("UNKNOWN", "#FFFFFF"))
            self.lbl_tx_mode.setText(name)
            self.lbl_tx_mode.setStyleSheet(f"font-weight: bold; color: {color}; font-size: 13px;")
        if "seq" in d:
            self.lbl_tx_seq.setText(str(d["seq"]))
        if "chunk" in d:
            self.lbl_tx_chunk.setText(f"{d['chunk']} / 8")
        if "cr" in d:
            self.lbl_tx_cr.setText(f"{d['cr']:.2f}x")

    def _handle_ch_data(self, d: dict):
        event = d.get("event", "")
        # Bỏ qua gói FB_FORWARD (phản hồi ngược từ Rx) để không đè lẫn với số liệu Kênh phát
        if event == "FB_FORWARD":
            return

        # 1. Đo lường tỷ lệ lỗi BER thực tế trên Kênh truyền
        ber_pct = None
        if "ber" in d:
            b = float(d["ber"])
            ber_pct = b if b > 1.0 else (b * 100.0)
            if ber_pct > 15.0: ber_pct = 15.0
            if ber_pct < 0.0: ber_pct = 0.0
            self.lbl_ch_ber_val.setText(f"{ber_pct:.1f} %")
            self.bar_ch_ber.setValue(int(ber_pct * 10))

        # 2. Cập nhật giá trị đọc trực tiếp từ biến trở vật lý (GPIO 0 ADC1 12-bit)
        pot_raw = None
        if "pot" in d:
            pot_raw = int(d["pot"])
        elif "raw" in d:
            pot_raw = int(d["raw"])
        elif "A0" in d:
            pot_raw = int(d["A0"])
        elif "A1" in d:
            pot_raw = int(d["A1"])
        elif "A3" in d:
            pot_raw = int(d["A3"])
        elif ber_pct is not None:
            pot_raw = int((ber_pct / 12.0) * 4095)

        if pot_raw is not None:
            pot_raw = max(0, min(4095, pot_raw))
            volt = (pot_raw / 4095.0) * 3.3
            self.lbl_ch_pot_adc.setText(f"{pot_raw} / 4095  ({volt:.2f} V)")
            self.lbl_ch_pot_quick.setText(f"Biến Trở (GPIO 0): {pot_raw} / 4095  ({volt:.2f} V)")

        # 3. Thông số khung và bit lỗi
        if "frames" in d:
            self.lbl_ch_frames.setText(str(d["frames"]))
        if "flipped" in d or "flipped_bits" in d:
            val = d.get("flipped", d.get("flipped_bits", 0))
            self.lbl_ch_flipped.setText(f"{val} bit")

        # 4. Cập nhật ảnh sau khi chịu tác động của nhiễu Kênh
        if "hex" in d and "chunk" in d:
            hex_str = d["hex"]
            chunk_idx = d["chunk"] - 1
            chunk_len = len(hex_str) // 2
            if 0 <= chunk_idx < 8 and chunk_len > 0:
                chunk_bytes = bytes.fromhex(hex_str)
                offset = chunk_idx * chunk_len
                self.ch_image_buffer[offset:offset+chunk_len] = chunk_bytes
                self._render_ch_image()
                if ber_pct is not None:
                    self.ber_history.append(ber_pct)
                    self.time_history.append(len(self.time_history) + 1)
                    self._update_plot()

    def _handle_rx_data(self, d: dict):
        if "mode" in d:
            mode_val = d["mode"]
            modes = {0: "DIRECT", 1: "HAMMING (7,4)", 2: "CYCLIC (7,4)"}
            self.lbl_rx_mode.setText(modes.get(mode_val, "DIRECT"))
            self._update_rx_leds(mode_val)

        if "fixed" in d:
            self.lbl_rx_fixed_last.setText(f"{d['fixed']} bit")
        if "total_fixed" in d:
            self.lbl_rx_fixed_total.setText(f"{d['total_fixed']} bit")
        if "ber" in d:
            ber_val = float(d["ber"])
            self.lbl_rx_ber.setText(f"{ber_val:.1f} %")
            # Khi bo Kênh chưa cắm trực tiếp vào PC, cập nhật gợi ý thay vì gán đè số liệu giả định
            if not self.ch_thread._is_running:
                self.lbl_ch_pot_quick.setText("Biến Trở: (Cắm USB bo Kênh để đọc trực tiếp)")
        if "rec_mode" in d:
            modes = {0: "DIRECT", 1: "HAMMING", 2: "CYCLIC"}
            self.lbl_rx_rec_mode.setText(modes.get(d["rec_mode"], "DIRECT"))

        # Khôi phục từng mảnh ảnh nhận được từ hex string
        if "hex" in d and "chunk" in d:
            hex_str = d["hex"]
            chunk_idx = d["chunk"] - 1
            chunk_len = len(hex_str) // 2
            if 0 <= chunk_idx < 8 and chunk_len > 0:
                chunk_bytes = bytes.fromhex(hex_str)
                offset = chunk_idx * chunk_len
                self.rx_image_buffer[offset:offset+chunk_len] = chunk_bytes
                self._render_rx_image()

    def _render_tx_image(self, idx: int):
        self.current_tx_img_idx = idx
        raw = SAMPLE_IMAGE_SMILE if idx == 0 else SAMPLE_IMAGE_SCORE10
        qimg = bytes_to_qimage(raw)
        pix = QtGui.QPixmap.fromImage(qimg).scaled(160, 160, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.FastTransformation)
        self.lbl_tx_img.setPixmap(pix)

    def _render_rx_image(self):
        qimg = bytes_to_qimage(self.rx_image_buffer)
        pix = QtGui.QPixmap.fromImage(qimg).scaled(160, 160, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.FastTransformation)
        self.lbl_rx_img.setPixmap(pix)

    def _render_ch_image(self):
        # 0: Đen, 1: Vàng Cam (#FFD600) thể hiện ảnh chịu nhiễu Kênh
        img = QtGui.QImage(64, 64, QtGui.QImage.Format.Format_Mono)
        img.setColorTable([0xFF000000, 0xFFFFD600])
        for y in range(64):
            for byte_col in range(8):
                idx = y * 8 + byte_col
                b = self.ch_image_buffer[idx] if idx < len(self.ch_image_buffer) else 0
                for bit in range(8):
                    val = 1 if (b & (0x80 >> bit)) else 0
                    img.setPixel(byte_col * 8 + bit, y, val)
        pix = QtGui.QPixmap.fromImage(img).scaled(160, 160, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.FastTransformation)
        self.lbl_ch_img.setPixmap(pix)

    def _update_rx_leds(self, active_mode: int):
        # 0: Direct (Green), 1: Hamming (Yellow), 2: Cyclic (Red)
        self.lbl_led_direct.setStyleSheet("border-radius: 4px; padding: 4px 6px; font-weight: bold; font-size: 11px; " +
            ("background: #00E676; color: #000000;" if active_mode == 0 else "background: #20202A; color: #666677;"))
        self.lbl_led_hamming.setStyleSheet("border-radius: 4px; padding: 4px 6px; font-weight: bold; font-size: 11px; " +
            ("background: #FFD600; color: #000000;" if active_mode == 1 else "background: #20202A; color: #666677;"))
        self.lbl_led_cyclic.setStyleSheet("border-radius: 4px; padding: 4px 6px; font-weight: bold; font-size: 11px; " +
            ("background: #FF1744; color: #FFFFFF;" if active_mode == 2 else "background: #20202A; color: #666677;"))

    def _update_plot(self):
        if not self.ber_history:
            return
        ax = self.plot_canvas.axes
        ax.clear()
        ax.set_facecolor('#101016')
        ax.grid(True, linestyle='--', alpha=0.35, color='#777788')
        ax.plot(list(self.time_history), list(self.ber_history), color='#00E5FF', linewidth=1.8, marker='.', markersize=4)
        ax.set_ylabel("BER (%)", color='#E0E0E0', fontsize=8, fontweight='bold')
        ax.set_ylim(-0.5, 13.0)
        ax.tick_params(colors='#E0E0E0', labelsize=8)
        self.plot_canvas.draw_idle()

    def _append_log_table(self, t_str: str, node: str, event: str, d: dict):
        row = self.log_table.rowCount()
        if row > 200:
            self.log_table.removeRow(0)
            row = self.log_table.rowCount()

        self.log_table.insertRow(row)
        
        t_item = QtWidgets.QTableWidgetItem(t_str)
        t_item.setForeground(QtGui.QColor("#FFFFFF"))
        self.log_table.setItem(row, 0, t_item)
        
        node_item = QtWidgets.QTableWidgetItem(node)
        if node == "TX": node_item.setForeground(QtGui.QColor("#00E5FF"))
        elif node == "CH": node_item.setForeground(QtGui.QColor("#FFD600"))
        elif node == "RX": node_item.setForeground(QtGui.QColor("#00E676"))
        self.log_table.setItem(row, 1, node_item)

        ev_item = QtWidgets.QTableWidgetItem(event)
        ev_item.setForeground(QtGui.QColor("#FFFFFF"))
        self.log_table.setItem(row, 2, ev_item)

        seq = str(d.get("seq", d.get("frames", "-")))
        seq_item = QtWidgets.QTableWidgetItem(seq)
        seq_item.setForeground(QtGui.QColor("#FFFFFF"))
        self.log_table.setItem(row, 3, seq_item)
        
        mode_str = "-"
        if "chn_alg" in d:
            mode_str = ["DIRECT", "HAMMING", "CYCLIC"][d["chn_alg"]] if d["chn_alg"] <= 2 else str(d["chn_alg"])
        elif "mode" in d:
            mode_str = ["DIRECT", "HAMMING", "CYCLIC"][d["mode"]] if d["mode"] <= 2 else str(d["mode"])
        mode_item = QtWidgets.QTableWidgetItem(mode_str)
        mode_item.setForeground(QtGui.QColor("#00FFCC"))
        self.log_table.setItem(row, 4, mode_item)

        # Rút gọn payload json
        clean_d = {k: v for k, v in d.items() if k not in ("node", "event", "hex")}
        detail = json.dumps(clean_d, ensure_ascii=False)
        det_item = QtWidgets.QTableWidgetItem(detail)
        det_item.setForeground(QtGui.QColor("#CBD5E1"))
        self.log_table.setItem(row, 5, det_item)

        self.log_table.scrollToBottom()

    def set_tx_image(self, idx: int):
        self.tx_thread.send_command(f'{{"cmd":"SET_IMAGE","idx":{idx}}}')

    def set_tx_auto_adaptive(self):
        self.tx_thread.send_command('{"cmd":"SET_ADAPTIVE","val":1}')

    def set_tx_mode(self, mode: int):
        self.tx_thread.send_command(f'{{"cmd":"SET_MODE","mode":{mode}}}')

    def on_ber_slider_changed(self, val: int):
        ber_val = (val / 10.0) / 100.0  # e.g. 50 -> 5.0% -> 0.05
        self.ch_thread.send_command(f'{{"cmd":"SET_BER","val":{ber_val:.4f}}}')

    def release_ber_to_adc(self):
        self.ch_thread.send_command('{"cmd":"SET_BER","val":-1}')
        self.slider_ber.setValue(0)

    def clear_rx_display(self):
        self.rx_image_buffer = bytearray(512)
        self._render_rx_image()
        self.rx_thread.send_command('{"cmd":"RESET_STATS"}')

    def reset_all_stats(self):
        self.tx_thread.send_command('{"cmd":"RESET_STATS"}')
        self.ch_thread.send_command('{"cmd":"RESET_STATS"}')
        self.rx_thread.send_command('{"cmd":"RESET_STATS"}')
        self.ber_history.clear()
        self.time_history.clear()
        self.rx_image_buffer = bytearray(512)
        self._render_rx_image()
        self.log_table.setRowCount(0)
        self._update_plot()
        show_custom_msg(self, "Thông Báo", "Đã gửi lệnh đặt lại toàn bộ thống kê trên cả 3 nút mạng!", "info")

    def toggle_rx_crc(self):
        """Bật hoặc Tắt kiểm tra CRC16 trên Máy thu (Rx)."""
        is_checked = self.btn_toggle_crc.isChecked()
        if is_checked:
            self.btn_toggle_crc.setText("🛡️ CRC: BẬT")
            self.btn_toggle_crc.setStyleSheet("background: #00E676; color: #121218; font-weight: bold; border-radius: 4px; padding: 6px;")
            self.rx_thread.send_command('{"cmd":"SET_CRC_CHECK","enabled":true}')
        else:
            self.btn_toggle_crc.setText("⚠️ CRC: TẮT (Vẽ ảnh tự do)")
            self.btn_toggle_crc.setStyleSheet("background: #FF5252; color: #FFFFFF; font-weight: bold; border-radius: 4px; padding: 6px;")
            self.rx_thread.send_command('{"cmd":"SET_CRC_CHECK","enabled":false}')

    def export_csv_logs(self):
        if not self.telemetry_logs:
            show_custom_msg(self, "Cảnh Báo", "Chưa có dữ liệu nhật ký để xuất!", "warning")
            return
        
        filename, _ = QtWidgets.QFileDialog.getSaveFileName(
            self, "Xuất Nhật Ký Đo Đạc Ra File CSV", 
            f"telemetry_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv", 
            "CSV Files (*.csv)"
        )
        if filename:
            try:
                with open(filename, 'w', newline='', encoding='utf-8') as f:
                    writer = csv.writer(f)
                    writer.writerow(["Timestamp", "Node", "Event", "Raw_JSON"])
                    for entry in self.telemetry_logs:
                        writer.writerow([entry["time"], entry["node"], entry["event"], entry["raw"]])
                show_custom_msg(self, "Thành Công", f"Đã lưu thành công file:\n{filename}", "info")
            except Exception as e:
                show_custom_msg(self, "Lỗi", f"Không thể lưu file CSV: {e}", "error")

    def closeEvent(self, event):
        self.tx_thread.disconnect_port()
        self.ch_thread.disconnect_port()
        self.rx_thread.disconnect_port()
        event.accept()


def main():
    app = QtWidgets.QApplication(sys.argv)
    app.setStyle("Fusion")

    # Đặt Dark Palette đồng nhất cho toàn hệ điều hành
    palette = QtGui.QPalette()
    palette.setColor(QtGui.QPalette.ColorRole.Window, QtGui.QColor("#121218"))
    palette.setColor(QtGui.QPalette.ColorRole.WindowText, QtGui.QColor("#FFFFFF"))
    palette.setColor(QtGui.QPalette.ColorRole.Base, QtGui.QColor("#181822"))
    palette.setColor(QtGui.QPalette.ColorRole.AlternateBase, QtGui.QColor("#20202E"))
    palette.setColor(QtGui.QPalette.ColorRole.ToolTipBase, QtGui.QColor("#1A1A26"))
    palette.setColor(QtGui.QPalette.ColorRole.ToolTipText, QtGui.QColor("#FFFFFF"))
    palette.setColor(QtGui.QPalette.ColorRole.Text, QtGui.QColor("#FFFFFF"))
    palette.setColor(QtGui.QPalette.ColorRole.Button, QtGui.QColor("#252536"))
    palette.setColor(QtGui.QPalette.ColorRole.ButtonText, QtGui.QColor("#FFFFFF"))
    palette.setColor(QtGui.QPalette.ColorRole.BrightText, QtGui.QColor("#00E5FF"))
    palette.setColor(QtGui.QPalette.ColorRole.Highlight, QtGui.QColor("#007ACC"))
    palette.setColor(QtGui.QPalette.ColorRole.HighlightedText, QtGui.QColor("#FFFFFF"))
    app.setPalette(palette)

    window = ModernDashboardWindow()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
