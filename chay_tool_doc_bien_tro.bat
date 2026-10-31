@echo off
chcp 65001 >nul
title GIÁM SÁT BIẾN TRỞ & BER (NODE 2 - KÊNH TRUYỀN)
cd /d "%~dp0"
echo ===================================================
echo     KHỞI ĐỘNG CÔNG CỤ ĐỌC BIẾN TRỞ THỜI GIAN THỰC
echo ===================================================
python tool_doc_bien_tro.py
pause
