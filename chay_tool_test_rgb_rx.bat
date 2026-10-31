@echo off
chcp 65001 >nul
title KIỂM TRA ĐÈN LED RGB BO THU (RX - COM11)
cd /d "%~dp0"
echo ================================================================
echo   CÔNG CỤ TEST LED RGB RX (GPIO 5, 6, 7 - ĐỘ SÁNG 1%%)
echo ================================================================
echo.
python tool_test_rgb_rx.py
echo.
pause
