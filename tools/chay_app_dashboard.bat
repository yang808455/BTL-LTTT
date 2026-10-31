@echo off
chcp 65001 > nul
cd /d "%~dp0"
echo =====================================================================
echo  KHỞI ĐỘNG BẢNG ĐIỀU KHIỂN HỆ THỐNG MÃ HÓA KÊNH THÍCH NGHI (3x ESP32-C3)
echo =====================================================================
python app_dashboard.py
pause
