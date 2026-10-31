@echo off
chcp 65001 >nul
title BẢNG VẼ PIXEL 64x64 - HỆ THỐNG TRUYỀN TIN ESP32-C3
cd /d "%~dp0"
echo ================================================================
echo   KHỞI ĐỘNG BẢNG VẼ PIXEL 64x64 (CHUẨN 512 BYTES CHO OLED 3)
echo   Thư mục lưu ảnh tự động: tools\saved_images
echo ================================================================
echo.
python tool_ve_anh_64x64.py
echo.
