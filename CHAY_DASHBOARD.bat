@echo off
chcp 65001 >nul
title BẢNG ĐIỀU KHIỂN HỆ THỐNG ESP32-C3
cd /d "%~dp0tools"
call chay_app_dashboard.bat
